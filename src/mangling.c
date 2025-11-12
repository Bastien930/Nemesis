
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <crypt.h>
#include <openssl/evp.h>
#include "Hash_Engine.h"
#include "Mangling.h"
#include "Utils.h"


// Niveaux de priorité
#define PRIORITY_HIGH 3    // Règles très probables
#define PRIORITY_MEDIUM 2  // Règles probables
#define PRIORITY_LOW 1     // Règles moins probables

// Variants de leetspeak
#define LEET_NONE 0
#define LEET_BASIC 1       // @, 3, 1, 0, $
#define LEET_EXTENDED 2    // + 5, 7, 8, 9
#define LEET_ALL 3         // Tous les variants

// Variants de capitalisation
#define CAP_NONE 0
#define CAP_FIRST 1        // Password
#define CAP_ALL 2          // PASSWORD
#define CAP_ALTERNATE 3    // pAsSwOrD
#define CAP_INVERSE_ALT 4  // PaSsWoRd
#define CAP_LAST 5         // passworD
#define CAP_ALL_VARIANTS 6 // Tous

// Options globales
#define MANGLE_NONE 0
#define MANGLE_ALL 1




// Suffixes numériques populaires
static const char *numeric_suffixes[] = {
    "1", "123", "1234", "11", "12", "13", "21", "69", "007", "777", "01"
};
static const int num_numeric_suffixes = 11;

// Années populaires (observées fréquemment : années récentes + 1990-2000)
static const char *year_suffixes[] = {
    "2025","2024","2023","2022","2021","2020",
    "2019","2018","2015","2010",
    "2005","2001","2000",
    "1999","1998","1997","1996","1995","1994","1993","1992","1991","1990"
};
static const int num_year_suffixes = 23;

// Suffixes symboles (les symboles réellement rencontrés dans les fuites)
static const char *symbol_suffixes[] = {
    "!", "!!", "?", "$", "#", "@", "*", ".", "!?","!1"
};
static const int num_symbol_suffixes = 10;

// Préfixes observés (moins fréquents que les suffixes mais présents)
static const char *common_prefixes[] = {
    "1", "12", "123", "!", "@", "#", "$", "_"
};
static const int num_common_prefixes = 8;

// Mots / tokens fréquemment combinés (observés dans les fuites francophones/internationales)
static const char *common_words[] = {
    "admin", "welcome", "master", "super", "motdepasse", "mdp",
    "password", "azerty", "qwerty", "love", "bonjour", "salut"
};
static const int num_common_words = 12;

// === STRUCTURES ===



typedef struct {
    int count;
    char words[256][MAX_WORD_LEN];  // tableau statique
} WordList;

// === FONCTIONS UTILITAIRES ===

void wordlist_init(WordList *wl) {
    if (!wl) return;
    wl->count = 0;
    for (size_t i = 0; i < 256; i++) {
        wl->words[i][0] = '\0';
    }
}

static inline void wordlist_add(WordList *wl, const char *word) {
    if (wl->count >= 256) {
        // liste pleine, on ne fait rien ou on peut signaler une erreur
        return;
    }
    size_t len = strlen(word);
    if (len >= MAX_WORD_LEN) {
        len = MAX_WORD_LEN - 1; // tronquer si trop long
    }
    // copie rapide avec memcpy + terminaison manuelle
    memcpy(wl->words[wl->count], word, MAX_WORD_LEN);
    wl->words[wl->count][len] = '\0';  // sécuriser la terminaison
    wl->count++;
}



/* === MODULE 1: LEETSPEAK (avec remplacements partiels) === */

// Remplacement complet (tous les caractères)
static inline void apply_leet_full(const char *input, char *output, int variant) {
    strcpy(output, input);
    for (int i = 0; output[i]; i++) {
        char c = tolower(output[i]);
        if (variant == 0) {  // Basic
            if (c == 'a') output[i] = '@';
            else if (c == 'e') output[i] = '3';
            else if (c == 'i') output[i] = '1';
            else if (c == 'o') output[i] = '0';
            else if (c == 's') output[i] = '$';
        } else {  // Extended
            if (c == 'a') output[i] = '4';
            else if (c == 'e') output[i] = '3';
            else if (c == 'i') output[i] = '!';
            else if (c == 'o') output[i] = '0';
            else if (c == 's') output[i] = '5';
            else if (c == 't') output[i] = '7';
            else if (c == 'b') output[i] = '8';
            else if (c == 'g') output[i] = '9';
        }
    }
}

// Remplacement partiel (un seul caractère à la fois)
static inline void apply_leet_single(const char *input, char target_char, char replacement, char *output) {
    strcpy(output, input);
    int replaced = 0;
    for (int i = 0; output[i] && !replaced; i++) {
        if (tolower(output[i]) == target_char) {
            output[i] = replacement;
            replaced = 1;  // Remplace seulement le premier
        }
    }
}

// Remplacement de caractères spécifiques (pas tous)
static inline void apply_leet_selective(const char *input, char *output, int pattern) {
    strcpy(output, input);
    for (int i = 0; output[i]; i++) {
        char c = tolower(output[i]);
        // Pattern 0: a et o seulement
        if (pattern == 0) {
            if (c == 'a') output[i] = '@';
            else if (c == 'o') output[i] = '0';
        }
        // Pattern 1: e et s seulement
        else if (pattern == 1) {
            if (c == 'e') output[i] = '3';
            else if (c == 's') output[i] = '$';
        }
        // Pattern 2: i et o seulement
        else if (pattern == 2) {
            if (c == 'i') output[i] = '1';
            else if (c == 'o') output[i] = '0';
        }
    }
}

static inline void reverse_str(const char *src, char *dest) {
    int len = strlen(src);
    for (int i = 0; i < len; ++i) dest[i] = src[len - 1 - i];
    dest[len] = '\0';
}

void module_leetspeak(const char *base, int mode, WordList *tmp) {
    char buffer[MAX_WORD_LEN];

    if (mode == LEET_NONE) return;

    // Variantes de remplacement PARTIEL (plus réalistes)
    // Remplace juste 'a' → '@'
    apply_leet_single(base, 'a', '@', buffer);
    da_hash_compare(buffer);
    wordlist_add(tmp,buffer);

    // Remplace juste 'e' → '3'
    apply_leet_single(base, 'e', '3', buffer);
    da_hash_compare(buffer);
    wordlist_add(tmp,buffer);

    // Remplace juste 'o' → '0'
    apply_leet_single(base, 'o', '0', buffer);
    da_hash_compare(buffer);
    wordlist_add(tmp,buffer);

    // Remplace juste 's' → '$'
    apply_leet_single(base, 's', '$', buffer);
    da_hash_compare(buffer);
    wordlist_add(tmp,buffer);

    // Remplacements sélectifs (2 caractères)
    apply_leet_selective(base, buffer, 0);  // a→@ et o→0
    da_hash_compare(buffer);
    wordlist_add(tmp,buffer);

    apply_leet_selective(base, buffer, 1);  // e→3 et s→$
    da_hash_compare(buffer);
    wordlist_add(tmp,buffer);

    if (mode == LEET_BASIC || mode == LEET_ALL) {
        // Remplacement complet basic
        apply_leet_full(base, buffer, 0);
        da_hash_compare(buffer);
        wordlist_add(tmp,buffer);
    }

    if (mode == LEET_EXTENDED || mode == LEET_ALL) {
        // Remplacement complet extended
        apply_leet_full(base, buffer, 1);
        da_hash_compare(buffer);
        wordlist_add(tmp,buffer);

        // Quelques variantes extended partielles
        apply_leet_single(base, 't', '7', buffer);
        da_hash_compare(buffer);
        wordlist_add(tmp,buffer);

        apply_leet_single(base, 'i', '!', buffer);
        da_hash_compare(buffer);
        wordlist_add(tmp,buffer);
    }
}

// === MODULE 2: CAPITALISATION ===

static inline void apply_cap_first(const char *input, char *output) {
    strcpy(output, input);
    if (output[0]) output[0] = toupper(output[0]);
}

static inline void apply_cap_all(const char *input, char *output) {
    strcpy(output, input);
    for (int i = 0; output[i]; i++) output[i] = toupper(output[i]);
}

static inline void apply_cap_alternate(const char *input, char *output) {
    strcpy(output, input);
    for (int i = 0; output[i]; i++) {
        output[i] = (i % 2 == 0) ? toupper(output[i]) : tolower(output[i]);
    }
}

static inline void apply_cap_last(const char *input, char *output) {
    strcpy(output, input);
    int len = strlen(output);
    if (len > 0) output[len - 1] = toupper(output[len - 1]);
}

void module_capitalization(const char *base, int mode) {
    char buffer[MAX_WORD_LEN];

    if (mode == CAP_FIRST) {
        apply_cap_first(base, buffer);
        da_hash_compare(buffer);
    } else if (mode == CAP_ALL) {
        apply_cap_all(base, buffer);
        da_hash_compare(buffer);
    } else if (mode == CAP_ALTERNATE) {
        apply_cap_alternate(base, buffer);
        da_hash_compare(buffer);
    } else if (mode == CAP_LAST) {
        apply_cap_last(base, buffer);
        da_hash_compare(buffer);
    } else if (mode == CAP_ALL_VARIANTS) {
        apply_cap_first(base, buffer);
        da_hash_compare(buffer);
        apply_cap_all(base, buffer);
        da_hash_compare(buffer);
        apply_cap_alternate(base, buffer);
        da_hash_compare(buffer);
        apply_cap_last(base, buffer);
        da_hash_compare(buffer);
    }
}

// === MODULE 3: SUFFIXES NUMÉRIQUES ===

void module_numeric_suffixes(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_numeric_suffixes; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, numeric_suffixes[i]);
        da_hash_compare(buffer);
    }
}

// === MODULE 4: SUFFIXES ANNÉES ===

void module_year_suffixes(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_year_suffixes; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, year_suffixes[i]);
        da_hash_compare(buffer);
    }
}

// === MODULE 5: SUFFIXES SYMBOLES ===

void module_symbol_suffixes(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_symbol_suffixes; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, symbol_suffixes[i]);
        da_hash_compare(buffer);
    }
}

// === MODULE 6: PRÉFIXES ===

void module_prefixes(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_common_prefixes; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", common_prefixes[i], base);
        da_hash_compare(buffer);
    }
}

// === MODULE 7: RÉPÉTITION ===

void module_repetition(const char *base) {
    char buffer[MAX_WORD_LEN];
    int len = strlen(base);

    // Répétition dernière lettre
    if (len > 0 && len < MAX_WORD_LEN - 2) {
        snprintf(buffer, MAX_WORD_LEN, "%s%c", base, base[len - 1]);
        da_hash_compare(buffer);
        snprintf(buffer, MAX_WORD_LEN, "%s%c%c", base, base[len - 1], base[len - 1]);
        da_hash_compare(buffer);
    }

    // Ajout de répétitions numériques
    const char *repeats[] = {"11", "111", "00"};
    for (int i = 0; i < 3; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, repeats[i]);
        da_hash_compare(buffer);
    }
}

// === MODULE 8: INVERSION ===

void module_reverse(const char *base) {
    char buffer[MAX_WORD_LEN];
    int len = strlen(base);

    for (int i = 0; i < len; i++) {
        buffer[i] = base[len - 1 - i];
    }
    buffer[len] = '\0';
    da_hash_compare(buffer);
}

// === MODULE 9: MOTS COMMUNS ===

void module_common_words(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_common_words; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, common_words[i]);
        da_hash_compare(buffer);
    }
}

static inline void combine_variant_with_suffixes(const char *variant, ManglingConfig *config) {
    char buf[MAX_WORD_LEN];

    // add base variant
    da_hash_compare(variant);

    // numeric suffixes
    if (config->use_numeric_suffixes) {
        for (int j = 0;j < num_numeric_suffixes; ++j) {
            snprintf(buf, MAX_WORD_LEN, "%s%s", variant, numeric_suffixes[j]);
            da_hash_compare(buf);
        }
    }

    // year suffixes
    if (config->use_year_suffixes) {
        for (int j = 0;j < num_year_suffixes; ++j) {
            snprintf(buf, MAX_WORD_LEN, "%s%s", variant, year_suffixes[j]);
            da_hash_compare(buf);
        }
    }

    // symbol suffixes
    if (config->use_symbol_suffixes) {
        for (int s = 0; s < num_symbol_suffixes; ++s) {
            snprintf(buf, MAX_WORD_LEN, "%s%s", variant, symbol_suffixes[s]);
            da_hash_compare(buf);
        }
    }

    // small combo numeric+symbol (keep very limited)
    if (config->use_numeric_suffixes && config->use_symbol_suffixes) {
            snprintf(buf, MAX_WORD_LEN, "%s%s%s", variant, numeric_suffixes[0], symbol_suffixes[0]);
            da_hash_compare(buf);

    }
}

// combine leet variant with capitalization + suffix combos (used to centralize all leet handling)
static inline void combine_leet_variant_full(const char *leet_variant, ManglingConfig *config) {
    char capbuf[MAX_WORD_LEN];

    // 1) leet itself + limited suffixes (numeric/year/symbol)
    combine_variant_with_suffixes(leet_variant, config);

    // 2) leet + first capital + limited suffixes (P@ssw0rd1)
    if (config->use_capitalization) {
        apply_cap_first(leet_variant, capbuf);
        combine_variant_with_suffixes(capbuf, config);
    }
}

// --- Nouvelle generate_mangled_words factorisée (garde logique priorités) ---

void generate_mangled_words(const char *base_word, ManglingConfig *config) {

    // Mot original d'abord
    if (!base_word)exit(1);
    da_hash_compare(base_word);

    char temp[MAX_WORD_LEN];
    char buffer[MAX_WORD_LEN];

    // === PRIORITÉ 1: Règles très probables ===

    // 1. Capitalisation simple
    if (config->use_capitalization && config->priority_level <= PRIORITY_HIGH) {
        module_capitalization(base_word, config->use_capitalization);
    }

    // 2. Suffixes numériques
    if (config->use_numeric_suffixes && config->priority_level <= PRIORITY_HIGH) {
        module_numeric_suffixes(base_word);
    }

    // 3. Suffixes symboles
    if (config->use_symbol_suffixes && config->priority_level <= PRIORITY_HIGH) {
        module_symbol_suffixes(base_word);
    }

    // 4. Capital + numeric (combo TRÈS courant)
    if (config->use_capitalization && config->use_numeric_suffixes &&
        config->priority_level <= PRIORITY_HIGH) {
        apply_cap_first(base_word, temp);
        module_numeric_suffixes(temp);
    }

    // 5. Leetspeak (centralisé une seule fois) :
    //    on génère temp_list avec module_leetspeak puis on applique toutes les combinaisons
    if (config->use_leetspeak && config->priority_level <= PRIORITY_HIGH) {
        WordList temp_list;
        wordlist_init(&temp_list);
        module_leetspeak(base_word, config->use_leetspeak, &temp_list);

        for (int i = 0; i < temp_list.count; i++) {
            // central function handles:
            //   - leet base + numeric/year/symbol
            //   - leet + capital first + numeric/year/symbol
            combine_leet_variant_full(temp_list.words[i], config);
        }
    }

    // === PRIORITÉ 2: Règles probables ===

    if (config->priority_level <= PRIORITY_MEDIUM) {

        // 6. Années + Capitalisation (Password2024)
        if (config->use_year_suffixes) {
            module_year_suffixes(base_word);

            if (config->use_capitalization) {
                apply_cap_first(base_word, temp);
                module_year_suffixes(temp);
            }
        }

        // 7. Capitalisation + symbole (ex: Password!)
        if (config->use_capitalization && config->use_symbol_suffixes) {
            apply_cap_first(base_word, temp);
            for (int s = 0; s < 3 && s < num_symbol_suffixes; s++) {
                snprintf(buffer, MAX_WORD_LEN, "%s%s", temp, symbol_suffixes[s]);
                da_hash_compare(buffer);
            }
        }

        // 8. Préfixes courants (!, 1, @)
        if (config->use_prefixes) {
            module_prefixes(base_word);
        }

        // 9. Répétition (ex: motdepassemotdepasse)
        if (config->use_repetition) {
            module_repetition(base_word);
        }

        // 10. Leet + capitalisation (déjà couvert dans combine_leet_variant_full)
        //     => plus besoin d'appeler module_leetspeak une seconde fois ici.

        // 11. Combinaison prénom/mot commun + num/symbole
        if (config->use_common_words) {
            for (int c = 0; c < num_common_words; c++) {
                snprintf(buffer, MAX_WORD_LEN, "%s%s", base_word, common_words[c]);
                da_hash_compare(buffer);

                if (config->use_numeric_suffixes) {
                    snprintf(temp, MAX_WORD_LEN+1, "%s%s", buffer, numeric_suffixes[0]); // add '1' or first numeric
                    da_hash_compare(temp);
                }

                if (config->use_symbol_suffixes) {
                    snprintf(temp, MAX_WORD_LEN+1, "%s%s", buffer, symbol_suffixes[0]); // '!' or first symbol
                    da_hash_compare(temp);
                }
            }
        }
    }

    // === PRIORITÉ 3: Règles moins probables ===

    if (config->priority_level <= PRIORITY_LOW) {

        // 12. Inversion
        if (config->use_reverse) {
            module_reverse(base_word);
        }

        // 13. Combinaisons année + symbole (ex: motdepasse2024!)
        if (config->use_year_suffixes && config->use_symbol_suffixes) {
            for (int y = 0; y < 3 && y < num_year_suffixes; y++) {
                for (int s = 0; s < 2 && s < num_symbol_suffixes; s++) {
                    snprintf(buffer, MAX_WORD_LEN, "%s%s%s", base_word, year_suffixes[y], symbol_suffixes[s]);
                    da_hash_compare(buffer);
                }
            }
        }

        // 14. Mots communs
        if (config->use_common_words) {
            module_common_words(base_word);
        }

        // 15. Reverse + numeric (ex: drowssap1)
        if (config->use_reverse && config->use_numeric_suffixes) {
            reverse_str(base_word, temp); // utilitaire au-dessus
            for (int j = 0; j < 2 && j < num_numeric_suffixes; j++) {
                snprintf(buffer, MAX_WORD_LEN, "%s%s", temp, numeric_suffixes[j]);
                da_hash_compare(buffer);
            }
        }
    }
}

// === CONFIGURATIONS PRÉDÉFINIES ===

ManglingConfig get_config_aggressive() {
    return (ManglingConfig) {
        .priority_level = PRIORITY_LOW,
        .use_leetspeak = LEET_ALL,
        .use_capitalization = CAP_ALL_VARIANTS,
        .use_numeric_suffixes = MANGLE_ALL,
        .use_year_suffixes = MANGLE_ALL,
        .use_symbol_suffixes = MANGLE_ALL,
        .use_prefixes = MANGLE_ALL,
        .use_repetition = MANGLE_ALL,
        .use_reverse = MANGLE_ALL,
        .use_common_words = MANGLE_ALL
    };
}

ManglingConfig get_config_balanced() {
    return (ManglingConfig) {
        .priority_level = PRIORITY_MEDIUM,
        .use_leetspeak = LEET_BASIC,
        .use_capitalization = CAP_FIRST,
        .use_numeric_suffixes = MANGLE_ALL,
        .use_year_suffixes = MANGLE_ALL,
        .use_symbol_suffixes = MANGLE_ALL,
        .use_prefixes = MANGLE_ALL,
        .use_repetition = MANGLE_ALL,
        .use_reverse = MANGLE_NONE,
        .use_common_words = MANGLE_NONE
    };
}

ManglingConfig get_config_fast() {
    return (ManglingConfig) {
        .priority_level = PRIORITY_HIGH,
        .use_leetspeak = LEET_BASIC,
        .use_capitalization = CAP_FIRST,
        .use_numeric_suffixes = MANGLE_ALL,
        .use_year_suffixes = MANGLE_NONE,
        .use_symbol_suffixes = MANGLE_ALL,
        .use_prefixes = MANGLE_NONE,
        .use_repetition = MANGLE_NONE,
        .use_reverse = MANGLE_NONE,
        .use_common_words = MANGLE_NONE
    };
}

// === EXEMPLE D'UTILISATION ===

void print_usage_example(int nb) {
    //printf("=== MODULE DE MANGLING AVANCÉ ===\n\n");

    const char *test_word = "password";


    //printf("Mot de base: %s\n\n", test_word);
    /*
    // Test config FAST
    //printf("--- CONFIG FAST (priorité haute uniquement) ---\n");
    ManglingConfig config_fast = get_config_fast();
    generate_mangled_words(test_word, &config_fast);
    printf("Variations: %d\n", variations.count);
    for (int i = 0; i < variations.count; i++) {
        printf("%3d. %s\n", i+1, variations.words[i]);
    }

    // Test config BALANCED
    printf("\n--- CONFIG BALANCED (priorité haute + moyenne) ---\n");
    ManglingConfig config_balanced = get_config_balanced();
    generate_mangled_words(test_word, &config_balanced, &variations);
    printf("Variations: %d\n", variations.count);
    for (int i = 0; i < variations.count; i++) {
        printf("%3d. %s\n", i+1, variations.words[i]);
    }
    */
    // Test config AGGRESSIVE
    //printf("\n--- CONFIG AGGRESSIVE (toutes priorités) ---\n");
    ManglingConfig conf = get_config_fast();
    //ManglingConfig conf = get_config_aggressive();
    //ManglingConfig conf = get_config_aggressive();


    init_global_sha();
    for (int i=0;i<nb;i++)generate_mangled_words(test_word, &conf);
    free_global_sha();
    printf("Variations: %lu\n", da_hash_get_count());
    //for (int i = 0; i < variations.count; i++) {
      //  printf("%3d. %s\n", i+1, variations.words[i]);
    //}
}

/*
int main(int agrc,char *argv[]) {
    if (agrc>1){
        for (int i = 1; i < atoi(argv[1]); i++) {
            print_usage_example();
        }
    }else
        for (int i = 1; i < 100000; i++) {
            print_usage_example();
        }
    return 0;
}
*/