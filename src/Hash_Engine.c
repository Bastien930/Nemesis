


#include "Shadow.h"



#include "Hash_Engine.h"

#include <crypt.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#include "log.h"

/* === Définition des globals === */
const char *da_hash = NULL;
const char *da_salt = NULL;
long da_hash_count = 0;
//da_hash_compare_fn da_compare_fn = NULL;

/* stockage binaire du hash cible / salt */
//static unsigned char *da_hash_bin = NULL;
static size_t da_hash_len = 0;
//static unsigned char *da_salt_bin = NULL;
static size_t da_salt_len = 0;

static inline int fast_eq_memcmp(const void *a, const void *b, size_t len) {
    if (len == 0) return 1;
    return memcmp(a, b, len) == 0;
}


/* helper : convertit une chaîne hex -> binaire
static int hex_to_bin(const char *hex, unsigned char **out, size_t *out_len) {
    if (!hex) return -1;
    size_t len = strlen(hex);
    if (len % 2 != 0) return -1;
    size_t blen = len / 2;
    unsigned char *buf = malloc(blen);
    if (!buf) return -1;
    for (size_t i = 0; i < blen; ++i) {
        char h = hex[2*i], l = hex[2*i+1];
        unsigned v = ((h >= 'a' ? h - 'a' + 10 : h - '0') << 4)
                   |  (l >= 'a' ? l - 'a' + 10 : l - '0');
        buf[i] = (unsigned char)v;
    }
    *out = buf;
    *out_len = blen;
    return 0;
}


static int base64_to_bin(const char *base64, unsigned char **out, size_t *out_len) {
    if (!base64 || !out || !out_len) return -1;

    size_t b64_len = strlen(base64);
    if (b64_len == 0) return -1;

    // Ajout du padding si nécessaire
    size_t padding = (4 - (b64_len % 4)) % 4;
    size_t padded_len = b64_len + padding;
    char *padded = malloc(padded_len + 1);
    if (!padded) return -1;

    strcpy(padded, base64);
    for (size_t i = 0; i < padding; i++) {
        padded[b64_len + i] = '=';
    }
    padded[padded_len] = '\0';

    // Allocation
    size_t max_len = ((padded_len * 3) / 4) + 1;
    unsigned char *buf = malloc(max_len);
    if (!buf) {
        free(padded);
        return -1;
    }

    // Décodage
    int decoded_len = EVP_DecodeBlock(buf, (const unsigned char*)padded, padded_len);
    free(padded);

    if (decoded_len < 0) {
        free(buf);
        return -1;
    }

    // Retire le padding ajouté
    decoded_len -= padding;

    *out = buf;
    *out_len = (size_t)decoded_len;
    return 0;
}





 helper générique pour hasher un buffer avec OpenSSL EVP
static EVP_MD_CTX *global_sha_ctx = NULL;

// Initialisation (à appeler une fois au début du programme)
void init_global_sha() {
    if (!global_sha_ctx) {
        global_sha_ctx = EVP_MD_CTX_new();
        if (global_sha_ctx)
            EVP_DigestInit_ex(global_sha_ctx, EVP_sha256(), NULL); // md par défaut
    }
}

// Libération (à appeler à la fin)
void free_global_sha() {
    if (global_sha_ctx) {
        EVP_MD_CTX_free(global_sha_ctx);
        global_sha_ctx = NULL;
    }
}

// Version optimisée de evp_hash
static int evp_hash(const EVP_MD *md,
                           const unsigned char *in, size_t in_len,
                           unsigned char *out, size_t *out_len)
{
    if (!global_sha_ctx) return -1; // contexte non initialisé

    // Réinitialisation du contexte
    if (EVP_MD_CTX_reset(global_sha_ctx) != 1) return -2;
    if (EVP_DigestInit_ex(global_sha_ctx, md, NULL) != 1) return -3;

    // Calcul du hash
    if (EVP_DigestUpdate(global_sha_ctx, in, in_len) != 1) return -4;

    unsigned int olen = 0;
    if (EVP_DigestFinal_ex(global_sha_ctx, out, &olen) != 1) return -5;

    *out_len = olen;
    return 0;
}*/

/* === Fonctions de hash concrètes (une par algo) === */

bool da_crypt(const char *password) {
    struct crypt_data data;
    data.initialized = 0;
    char *crypt_res = crypt_r(password, da_salt,&data);
    if (!crypt_res) {char buffer[512];safe_concat(buffer,512,"Erreur lors de crypt pour le mot de passe : ",password);write_log(LOG_WARNING,buffer,"da_crypt"); return false;}
    /* 2) extraire la partie hash encodée (dernier champ après $) */
    const char *enc_hash = extract_crypt_hash_part(crypt_res);

    if (strlen(enc_hash) != da_hash_len) return false;
    return fast_eq_memcmp(enc_hash,da_hash,da_hash_len);
}

/* SHA256 */
/*
static bool sha256_compare_fn(const char *password) {
    unsigned char buf[256];
    size_t pwlen = strlen(password);
    size_t total = da_salt_len + pwlen;
    unsigned char *input = buf;

    if (total > sizeof(buf)) {
        input = malloc(total);
        if (!input) return false;
    }

    // Copie salt+password
    if (da_salt_len) memcpy(input, da_salt_bin, da_salt_len);
    memcpy(input + da_salt_len, password, pwlen);

    // Au lieu de : salt + password

    //if (da_salt_len) memcpy(input, password, pwlen);
    //memcpy(input + pwlen, da_salt_bin, da_salt_len);

    // Hash
    unsigned char digest[EVP_MAX_MD_SIZE];
    size_t dlen = 0;
    int ret = evp_hash(EVP_sha256(), input, total, digest, &dlen);  // ← Vérifie le retour !

    if (input != buf) free(input);

    // Vérifications
    if (ret != 0) return false;          // ← IMPORTANT : vérifie que evp_hash a réussi
    if (dlen != da_hash_len) return false;

    // Debug (à retirer en prod)
    if (strcmp(password, "Password123") == 0) {
        printf("[DEBUG] Match test:\n");
        printf("  Computed: ");
        for (size_t i = 0; i < dlen; i++) printf("%02x", digest[i]);
        printf("\n  Expected: ");
        for (size_t i = 0; i < da_hash_len; i++) printf("%02x", da_hash[i]);
        printf("\n  Result: %d\n", fast_eq_memcmp(digest, da_hash_bin, dlen));
    }

    return fast_eq_memcmp(digest, da_hash_bin, dlen);
}
*/
/* SHA512 */
/*
static bool sha512_compare_fn(const char *password) {
    unsigned char buf[512];
    size_t pwlen = strlen(password);
    size_t total = da_salt_len + pwlen;
    unsigned char *input = buf;

    if (total > sizeof(buf)) {
        input = malloc(total);
        if (!input) return false;
    }
    if (da_salt_len) memcpy(input, da_salt_bin, da_salt_len);
    memcpy(input + da_salt_len, password, pwlen);

    unsigned char digest[EVP_MAX_MD_SIZE];
    size_t dlen = 0;
    evp_hash(EVP_sha512(), input, total, digest, &dlen);

    if (input != buf) free(input);
    if (dlen != da_hash_len) return false;

    return fast_eq_memcmp(digest,da_hash_bin,dlen);
}

 MD5 (exemple)

static bool md5_compare_fn(const char *password) {
    unsigned char buf[128];
    size_t pwlen = strlen(password);
    size_t total = da_salt_len + pwlen;
    unsigned char *input = buf;
    if (total > sizeof(buf)) {
        input = malloc(total);
        if (!input) return false;
    }
    if (da_salt_len) memcpy(input, da_salt_bin, da_salt_len);
    memcpy(input + da_salt_len, password, pwlen);

    unsigned char digest[EVP_MAX_MD_SIZE];
    size_t dlen = 0;
    evp_hash(EVP_md5(), input, total, digest, &dlen);

    if (input != buf) free(input);
    if (dlen != da_hash_len) return false;

    return fast_eq_memcmp(digest,da_hash_bin,dlen);
}
 fallback
static bool unknown_compare_fn(const char *password) {
    (void)password;
    return false;
}*/

/* === API === */
const char *extract_crypt_hash_part(const char *crypt_res) {
    if (!crypt_res) return NULL;
    /* find last '$' */
    const char *last = strrchr(crypt_res, '$');
    if (!last) return NULL;
    return last + 1;
}


bool da_hash_engine_init(struct da_shadow_entry *entry) {
    if (!entry) {write_log(LOG_ERROR,"Erreur aucune shadow entry impossible d'initilialiser le hash engine","da_hash_engine_init"); return false;}

    da_hash_engine_reset();

    //const da_hash_algo_t da_algo_hash = entry->algo;
    da_hash = entry->hash;
    da_salt = entry->salt;
    da_hash_len = strlen(entry->hash);
    da_salt_len = strlen(entry->salt);

    struct crypt_data warmup;
    memset(&warmup, 0, sizeof(warmup));
    crypt_r("warmup", "$5$abcd1234$", &warmup);


    /*if (base64_to_bin(da_hash, &da_hash_bin, &da_hash_len) != 0) {
        printf("erreur de conversion base64\n");
        return false;
    }

    if (da_salt && hex_to_bin(da_salt, &da_salt_bin, &da_salt_len) != 0) {
        free(da_hash_bin); da_hash_bin = NULL;
        printf("erreur lros de la conversion du salt \n");
        return false;
    }*/

    /*switch (da_algo_hash) {
        case DA_HASH_MD5:
            da_compare_fn = md5_compare_fn;
            break;
        case DA_HASH_SHA256:
            da_compare_fn = sha256_compare_fn;
            break;
        case DA_HASH_SHA512:
            da_compare_fn = sha512_compare_fn;
            break;
        default:
            da_compare_fn = unknown_compare_fn;
            break;
    }*/

    return true;
}

void da_hash_engine_reset(void) {
    //if (da_hash_bin) { free(da_hash_bin); da_hash_bin = NULL; }
    //if (da_salt_bin) { free(da_salt_bin); da_salt_bin = NULL; }
    da_hash_len = da_salt_len = 0;
    da_hash = NULL;
    da_salt = NULL;
    //da_compare_fn = NULL;
    da_hash_count = 0;
}
