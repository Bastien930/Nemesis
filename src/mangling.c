
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <crypt.h>
#include <openssl/evp.h>

#include "Config.h"
#include "Hash_Engine.h"
#include "Mangling.h"
#include "Utils.h"
#include "hashSet.h"


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


static __thread HashSet hs;


static const char *numeric_suffixes[] = {
    "1", "123", "1234", "69", "01"
};
static const int num_numeric_suffixes = 5;

static const char *year_suffixes[] = {
    "2025","2024","2023","2022","2021","2020",
    "2019","2018","2015","2010",
    "2000",
};
static const int num_year_suffixes = 11;

static const char *symbol_suffixes[] = {
    "!", "?", "$", "#", "@", "*", "."//, "!?","!1"
};
static const int num_symbol_suffixes = 7;

static const char *common_prefixes[] = {
    "123", "!", "@", "#", "$", "_"
};
static const int num_common_prefixes = 6;

static const char *common_words[] = {
    "admin", "welcome", "master", "super", "motdepasse", "mdp",
    "password", "azerty", "qwerty"
};
static const int num_common_words = 9;

typedef struct {
    int count;
    char words[256][MAX_WORD_LEN];  // tableau statique
} WordList;


/**
 * Initialiser une nouvelle liste de mots vide
 * 
 * @param wl Pointeur vers la structure WordList à initialiser
 */
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
        len = MAX_WORD_LEN - 1;
    }
    memcpy(wl->words[wl->count], word, MAX_WORD_LEN);
    wl->words[wl->count][len] = '\0';
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

/**
 * Générer des variations leetspeak d'un mot
 * 
 * @param base Mot de base à transformer
 * @param mode Mode de transformation leetspeak à appliquer
 * @param tmp Liste temporaire pour stocker les variations
 */
void module_leetspeak(const char *base, int mode, WordList *tmp) {
    char buffer[MAX_WORD_LEN];

    if (mode == LEET_NONE) return;

    // Variantes de remplacement PARTIEL (plus réalistes)
    // Remplace juste 'a' → '@'
    apply_leet_single(base, 'a', '@', buffer);
    NEMESIS_hash_compare(buffer,&hs);
    wordlist_add(tmp,buffer);

    // Remplace juste 'e' → '3'
    apply_leet_single(base, 'e', '3', buffer);
    NEMESIS_hash_compare(buffer,&hs);
    wordlist_add(tmp,buffer);

    // Remplace juste 'o' → '0'
    apply_leet_single(base, 'o', '0', buffer);
    NEMESIS_hash_compare(buffer,&hs);
    wordlist_add(tmp,buffer);

    // Remplace juste 's' → '$'
    apply_leet_single(base, 's', '$', buffer);
    NEMESIS_hash_compare(buffer,&hs);
    wordlist_add(tmp,buffer);

    // Remplacements sélectifs (2 caractères)
    apply_leet_selective(base, buffer, 0);  // a→@ et o→0
    NEMESIS_hash_compare(buffer,&hs);
    wordlist_add(tmp,buffer);

    apply_leet_selective(base, buffer, 1);  // e→3 et s→$
    NEMESIS_hash_compare(buffer,&hs);
    wordlist_add(tmp,buffer);

    if (mode == LEET_BASIC || mode == LEET_ALL) {
        // Remplacement complet basic
        apply_leet_full(base, buffer, 0);
        NEMESIS_hash_compare(buffer,&hs);
        wordlist_add(tmp,buffer);
    }

    if (mode == LEET_EXTENDED || mode == LEET_ALL) {
        // Remplacement complet extended
        apply_leet_full(base, buffer, 1);
        NEMESIS_hash_compare(buffer,&hs);
        wordlist_add(tmp,buffer);

        // Quelques variantes extended partielles
        apply_leet_single(base, 't', '7', buffer);
        NEMESIS_hash_compare(buffer,&hs);
        wordlist_add(tmp,buffer);

        apply_leet_single(base, 'i', '!', buffer);
        NEMESIS_hash_compare(buffer,&hs);
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

/**
 * Générer des variations de capitalisation d'un mot
 * 
 * @param base Mot de base à transformer
 * @param mode Mode de capitalisation à appliquer
 */
void module_capitalization(const char *base, int mode) {
    char buffer[MAX_WORD_LEN];

    if (mode == CAP_FIRST) {
        apply_cap_first(base, buffer);
        NEMESIS_hash_compare(buffer,&hs);
    } else if (mode == CAP_ALL) {
        apply_cap_all(base, buffer);
        NEMESIS_hash_compare(buffer,&hs);
    } else if (mode == CAP_ALTERNATE) {
        apply_cap_alternate(base, buffer);
        NEMESIS_hash_compare(buffer,&hs);
    } else if (mode == CAP_LAST) {
        apply_cap_last(base, buffer);
        NEMESIS_hash_compare(buffer,&hs);
    } else if (mode == CAP_ALL_VARIANTS) {
        apply_cap_first(base, buffer);
        NEMESIS_hash_compare(buffer,&hs);
        apply_cap_all(base, buffer);
        NEMESIS_hash_compare(buffer,&hs);
        apply_cap_alternate(base, buffer);
        NEMESIS_hash_compare(buffer,&hs);
        apply_cap_last(base, buffer);
        NEMESIS_hash_compare(buffer,&hs);
    }
}

// === MODULE 3: SUFFIXES NUMÉRIQUES ===

/**
 * Ajouter des suffixes numériques à un mot
 * 
 * @param base Mot de base auquel ajouter les suffixes
 */
void module_numeric_suffixes(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_numeric_suffixes; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, numeric_suffixes[i]);
        NEMESIS_hash_compare(buffer,&hs);
    }
}

// === MODULE 4: SUFFIXES ANNÉES ===

/**
 * Ajouter des années en suffixe à un mot
 * 
 * @param base Mot de base auquel ajouter les années
 */
void module_year_suffixes(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_year_suffixes; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, year_suffixes[i]);
        NEMESIS_hash_compare(buffer,&hs);
    }
}

// === MODULE 5: SUFFIXES SYMBOLES ===

/**
 * Ajouter des symboles en suffixe à un mot
 * 
 * @param base Mot de base auquel ajouter les symboles
 */
void module_symbol_suffixes(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_symbol_suffixes; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, symbol_suffixes[i]);
        NEMESIS_hash_compare(buffer,&hs);
    }
}

// === MODULE 6: PRÉFIXES ===

/**
 * Ajouter des préfixes communs à un mot
 * 
 * @param base Mot de base auquel ajouter les préfixes
 */
void module_prefixes(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_common_prefixes; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", common_prefixes[i], base);
        NEMESIS_hash_compare(buffer,&hs);
    }
}

// === MODULE 7: RÉPÉTITION ===

/**
 * Générer des variations avec répétition de caractères
 * 
 * @param base Mot de base à modifier
 */
void module_repetition(const char *base) {
    char buffer[MAX_WORD_LEN];
    int len = strlen(base);

    // Répétition dernière lettre
    if (len > 0 && len < MAX_WORD_LEN - 2) {
        snprintf(buffer, MAX_WORD_LEN, "%s%c", base, base[len - 1]);
        NEMESIS_hash_compare(buffer,&hs);
        snprintf(buffer, MAX_WORD_LEN, "%s%c%c", base, base[len - 1], base[len - 1]);
        NEMESIS_hash_compare(buffer,&hs);
    }

    // Ajout de répétitions numériques
    const char *repeats[] = {"11", "111", "00"};
    for (int i = 0; i < 3; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, repeats[i]);
        NEMESIS_hash_compare(buffer,&hs);
    }
}

// === MODULE 8: INVERSION ===

/**
 * Inverser un mot
 * 
 * @param base Mot à inverser
 */
void module_reverse(const char *base) {
    char buffer[MAX_WORD_LEN];
    int len = strlen(base);

    for (int i = 0; i < len; i++) {
        buffer[i] = base[len - 1 - i];
    }
    buffer[len] = '\0';
    NEMESIS_hash_compare(buffer,&hs);
}

// === MODULE 9: MOTS COMMUNS ===

/**
 * Combiner le mot avec des mots communs
 * 
 * @param base Mot de base à combiner
 */
void module_common_words(const char *base) {
    char buffer[MAX_WORD_LEN];
    for (int i = 0; i < num_common_words; i++) {
        snprintf(buffer, MAX_WORD_LEN, "%s%s", base, common_words[i]);
        NEMESIS_hash_compare(buffer,&hs);
    }
}

static inline void combine_variant_with_suffixes(const char *variant, ManglingConfig *config) {
    char buf[MAX_WORD_LEN];

    // add base variant
    NEMESIS_hash_compare(variant,&hs);

    // numeric suffixes
    if (config->use_numeric_suffixes) {
        for (int j = 0;j < num_numeric_suffixes; ++j) {
            snprintf(buf, MAX_WORD_LEN, "%s%s", variant, numeric_suffixes[j]);
            NEMESIS_hash_compare(buf,&hs);
        }
    }

    // year suffixes
    if (config->use_year_suffixes) {
        for (int j = 0;j < num_year_suffixes; ++j) {
            snprintf(buf, MAX_WORD_LEN, "%s%s", variant, year_suffixes[j]);
            NEMESIS_hash_compare(buf,&hs);
        }
    }

    // symbol suffixes
    if (config->use_symbol_suffixes) {
        for (int s = 0; s < num_symbol_suffixes; ++s) {
            snprintf(buf, MAX_WORD_LEN, "%s%s", variant, symbol_suffixes[s]);
            NEMESIS_hash_compare(buf,&hs);
        }
    }

    // small combo numeric+symbol (keep very limited)
    if (config->use_numeric_suffixes && config->use_symbol_suffixes) {
            snprintf(buf, MAX_WORD_LEN, "%s%s%s", variant, numeric_suffixes[0], symbol_suffixes[0]);
            NEMESIS_hash_compare(buf,&hs);

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
// Attention le mangling test le mot de passe de départ.
/**
 * Générer toutes les variations possibles d'un mot selon la configuration
 * 
 * @param base_word Mot de base à transformer
 * @param config Configuration des règles de transformation à appliquer
 */
void generate_mangled_words(const char *base_word, ManglingConfig *config) {

    if (is_password_found()) return;
    // Mot original d'abord
    if (!base_word)exit(1);

    hashset_init(&hs);

    NEMESIS_hash_compare(base_word,&hs);

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
                NEMESIS_hash_compare(buffer,&hs);
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
                NEMESIS_hash_compare(buffer,&hs);

                if (config->use_numeric_suffixes) {
                    snprintf(temp, MAX_WORD_LEN+1, "%s%s", buffer, numeric_suffixes[0]); // add '1' or first numeric
                    NEMESIS_hash_compare(temp,&hs);
                }

                if (config->use_symbol_suffixes) {
                    snprintf(temp, MAX_WORD_LEN+1, "%s%s", buffer, symbol_suffixes[0]); // '!' or first symbol
                    NEMESIS_hash_compare(temp,&hs);
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
                    NEMESIS_hash_compare(buffer,&hs);
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
                NEMESIS_hash_compare(buffer,&hs);
            }
        }
    }
    hashset_free(&hs);
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

ManglingConfig NEMESIS_getConfigMangling(int config) {
    switch (config) {
        case NEMESIS_MANGLING_BALANCED : {return get_config_balanced(); break;}
        case NEMESIS_MANGLING_AGGRESSIVE : {return get_config_aggressive(); break;}
        default : {return get_config_fast(); break;}

    }
}
int NEMESIS_get_iteration_of_mangling(int config) {
    switch (config) {
        case NEMESIS_MANGLING_FAST : {return NEMESIS_MANGLING_FAST; break;}
        case NEMESIS_MANGLING_BALANCED : {return NEMESIS_MANGLING_BALANCED; break;}
        default : {return NEMESIS_MANGLING_AGGRESSIVE; break;}

    }
}

// === EXEMPLE D'UTILISATION ===

void print_usage_example() {

    const char *test_word = "password";

    ManglingConfig config_fast = get_config_fast();
    generate_mangled_words(test_word, &config_fast);
    printf("Variations: %lu\n", NEMESIS_hash_get_count());

    ManglingConfig config_balanced = get_config_balanced();
    generate_mangled_words(test_word, &config_balanced);
    printf("Variations: %lu\n", NEMESIS_hash_get_count());

    ManglingConfig conf_agress = get_config_aggressive();



    generate_mangled_words(test_word, &conf_agress);
    printf("Variations: %lu\n", NEMESIS_hash_get_count());
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