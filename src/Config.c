//
// Created by bastien on 10/24/25.
//

#include "../Include/Config.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <zlib.h>
#include <stdlib.h>

#include "brute_force.h"
#include "Dictionnary.h"
#include "log.h"
#include "Utils.h"


void NEMESIS_config_init_default(NEMESIS_config_t *cfg){
    if (!cfg) return;
    memset(cfg, 0, sizeof(*cfg));
    cfg->input.shadow_file[0] = '\0';
    cfg->input.wordlist_file[0] = '\0';
    cfg->input.save= 0;

    cfg->attack.enable_dictionary = false;
    cfg->attack.enable_bruteforce = false;
    cfg->attack.enable_mangling = false;
    cfg->attack.mangling_config = NEMESIS_MANGLING_FAST;
    cfg->attack.charset_preset = NEMESIS_CHARSET_PRESET_DEFAULT;
    cfg->attack.charset_custom[0] = '\0';
    cfg->attack.min_len = NEMESIS_MIN_LEN ;
    cfg->attack.max_len = NEMESIS_MAX_LEN ;
    cfg->attack.max_attempts = NEMESIS_MAX_ATTEMPTS ;

    strcpy(cfg->output.output_file, "output.txt");
    cfg->output.format = NEMESIS_OUT_TXT ;
    cfg->output.enable_logging = true ;
    strcpy(cfg->output.log_file, "log.da");

    cfg->system.threads = 16 ;
    cfg->system.enable_gpu = false ;

    cfg->meta.version = NEMESIS_VERSION ;
    cfg->meta.build_date = NEMESIS_BUILD_DATE ;
    cfg->meta.author = NEMESIS_AUTHOR ;

    //cfg->show_help = false ;


}
/* Retour codes :
   0   = OK
  -1   = cfg NULL
  -2   = shadow file missing / not readable
  -3   = neither dictionary nor bruteforce enabled
  -4   = dictionary enabled but no wordlist
  -5   = invalid min/max lengths
  -6   = invalid threads value
  -7   = logging enabled but no log file
*/
int NEMESIS_config_validate(const NEMESIS_config_t *cfg, char *errbuf, size_t errlen) {
    if (!cfg) {
        if (errbuf && errlen) snprintf(errbuf, errlen, "config is NULL");
        return -1;
    }

    /* shadow file obligatoire */
    if (cfg->input.shadow_file[0] == '\0') {
        if (errbuf && errlen) snprintf(errbuf, errlen, "shadow file not specified");
        return -2;
    }

    /* si on accepte "-" pour stdin, on skip access check */
    if (!(cfg->input.shadow_file[0] == '-' && cfg->input.shadow_file[1] == '\0')) {
        if (access(cfg->input.shadow_file, R_OK) != 0) {
            if (errbuf && errlen) snprintf(errbuf, errlen,
                                           "shadow file '%s' not readable: %s",
                                           cfg->input.shadow_file, strerror(errno));
            return -2;
        }
    }

    /* au moins un mode actif */
    if (!cfg->attack.enable_dictionary && !cfg->attack.enable_bruteforce) {
        if (errbuf && errlen) snprintf(errbuf, errlen, "no attack mode enabled (dictionary or bruteforce)");
        return -3;
    }
    if (cfg->attack.enable_dictionary && cfg->attack.enable_bruteforce) {
        if (errbuf && errlen) snprintf(errbuf, errlen, "2 icomptible mode enabled (dictionary and bruteforce)");
        return -3;
    }

    /* si dictionnaire activé, wordlist requis (sauf si bruteforce seul) */
    if (cfg->attack.enable_dictionary) {
        if (cfg->input.wordlist_file[0] == '\0') {
            if (errbuf && errlen) snprintf(errbuf, errlen, "dictionary enabled but no wordlist specified");
            return -4;
        }
        if (!(cfg->input.wordlist_file[0] == '-' && cfg->input.wordlist_file[1] == '\0')) {
            if (access(cfg->input.wordlist_file, R_OK) != 0) {
                if (errbuf && errlen) snprintf(errbuf, errlen,
                                               "wordlist '%s' not readable: %s",
                                               cfg->input.wordlist_file, strerror(errno));
                return -4;
            }
        }
    }

    /* min/max length checks */
    if (cfg->attack.min_len < NEMESIS_MIN_LEN || cfg->attack.max_len < NEMESIS_MIN_LEN) {
        if (errbuf && errlen) snprintf(errbuf, errlen, "min/max must be >= %d", NEMESIS_MIN_LEN);
        return -5;
    }
    if (cfg->attack.min_len > cfg->attack.max_len) {
        if (errbuf && errlen) snprintf(errbuf, errlen, "min_len (%d) > max_len (%d)",
                                        cfg->attack.min_len, cfg->attack.max_len);
        return -5;
    }
    if (cfg->attack.max_len > NEMESIS_MAX_LEN) {
        if (errbuf && errlen) snprintf(errbuf, errlen,
                                        "max_len (%d) too large (limit %d)",
                                        cfg->attack.max_len, NEMESIS_MAX_LEN);
        return -5;
    }

    /* attempts bound */
    if (cfg->attack.max_attempts < 0) {
        if (errbuf && errlen) snprintf(errbuf, errlen, "max_attempts must be >= 0");
        return -8;
    }

    /* threads checks */
    if (cfg->system.threads == 0) {
        if (errbuf && errlen) snprintf(errbuf, errlen, "threads must be >= 1");
        return -6;
    }
    if (cfg->system.threads > NEMESIS_MAX_THREADS) {
        if (errbuf && errlen) snprintf(errbuf, errlen, "threads (%u) exceeds compile-time limit (%d)",
                                        cfg->system.threads, NEMESIS_MAX_THREADS);
        return -6;
    }

    /* logging */
    if (cfg->output.enable_logging && cfg->output.log_file[0] == '\0') {
        if (errbuf && errlen) snprintf(errbuf, errlen, "logging enabled but no log file specified");
        return -7;
    }

    return 0;


}

/*
 * progname = argv[0].
 * cette fonction est appeler si on detecte l'option -h ou --help peut import les autres options.
*/
void NEMESIS_print_usage(const char *progname) {
    if (!progname) progname = "NEMESIS_tool";

    printf("%s %d — utilitaire d'analyse de shadow / dictionary attacks (usage sécurisé)\n\n", progname, NEMESIS_VERSION);
    printf("Usage: %s [options]\n\n", progname);

    puts("Options (principales) :");
    puts("  -s, --shadow <file>        Fichier shadow (obligatoire).");
    puts("                             Utilisez '-' pour lire depuis stdin.");
    puts("  -w, --wordlist <file>      Fichier wordlist (pour attaque par dictionnaire).");
    puts("      --resume               Active la reprise du programme a partir de la sauvegarde.");
    puts("  -m, --mangling <config>    Config vaut fast, balanced, agressive (défaut : fast).");
    puts("  -d, --dictionary           Activer le mode dictionnaire.");
    puts("  -b, --bruteforce           Activer le mode bruteforce.");
    puts("  -c, --charset <preset|chars> Charset prédéfini ou liste explicite de caractères.");
    puts("                             presets: default, alphanum, numeric, <custom>");
    puts("      --min <n>              Longueur minimale pour génération brutforce (défaut : " STR(NEMESIS_MIN_LEN) ").");
    puts("      --max <n>              Longueur maximale pour génération brutforce (défaut : " STR(NEMESIS_MAX_LEN) ").");
    puts("  -t, --threads <n>          Nombre de threads (défaut : 1).");
    puts("  -o, --output <file>        Fichier de sortie (JSON/CSV/TXT selon extension).");
    puts("      --format <json|csv|txt> Forcer le format de sortie (TXT si omis).");
    puts("  --log <file>               Activer logging vers fichier (doit être écrit).");
    puts("  --enable-gpu               Activer backend GPU (si disponible).");
    puts("  -h, --help                 Afficher cette aide et quitter.");
    puts("  --version                  Afficher la version et quitter.");
    puts("");

    printf("Limites / valeurs par défaut compilées :\n");
    printf("  NEMESIS_MAX_PATH = %d, NEMESIS_MAX_THREADS = %d, NEMESIS_MIN_LEN = %d, NEMESIS_MAX_LEN = %d\n",
           NEMESIS_MAX_PATH, NEMESIS_MAX_THREADS, NEMESIS_MIN_LEN, NEMESIS_MAX_LEN);
    printf("  NEMESIS_MAX_ALLOWED_LEN = %d, NEMESIS_MAX_ATTEMPTS = %d\n", NEMESIS_MAX_LEN, NEMESIS_MAX_ATTEMPTS);
    puts("");

    puts("Exemples :");
    printf("  %s -s shadow.dump -w words.txt -d -o results.json\n", progname);
    printf("  %s -s /etc/shadow -w rockyou.txt -m 'leet,append:123' --anonymize --log run.log\n", progname);
    puts("");

    puts("Notes de sécurité :");
    puts("  - N'utilisez cet outil que sur des systèmes / dumps pour lesquels vous avez l'autorisation.");
    puts("  - Par défaut, l'export complet des hashes est désactivé ; activez-le explicitement si nécessaire.");
    puts("  - Préférez le mode simulation avant de lancer des opérations réelles.");
    puts("");
    fflush(stdout);
}


int NEMESIS_save_config(FILE *file, NEMESIS_config_t *config) {
    if (!file || !config) return -1;

    struct NEMESIS_config_file config_file;

    // Initialiser à zéro pour éviter les données non initialisées
    memset(&config_file, 0, sizeof(config_file));

    // En-tête
    config_file.magic = NEMESIS_CONFIG_MAGIC;
    config_file.version = NEMESIS_VERSION;

    // Copier les structures (qui ne contiennent QUE des tableaux, pas de pointeurs)
    config_file.input = config->input;
    config_file.attack = config->attack;
    config_file.output = config->output;
    config_file.system = config->system;
    config_file.meta_version = config->meta.version;

    // Calculer le checksum sur toutes les données SAUF le champ checksum lui-même
    size_t data_size = sizeof(config_file) - sizeof(config_file.checksum);
    uLong checksum = crc32(0uL, Z_NULL, 0);
    checksum = crc32(checksum, (const Bytef *)&config_file, data_size);

    config_file.checksum = checksum;

    // Écrire la structure complète
    if (fwrite(&config_file, sizeof(config_file), 1, file) != 1) {
        return -1;
    }

    fflush(file);
    fclose(file);
    return 0;
}

int NEMESIS_load_config(NEMESIS_config_t *config) {
    char full_path[NEMESIS_MAX_PATH];
    PATH_JOIN(full_path,NEMESIS_MAX_PATH,NEMESIS_config.output.config_dir,NEMESIS_STOPPED_FILE);
    FILE *file = fopen(full_path, "rb");
    if (!file || !config) return -1;

    struct NEMESIS_config_file config_file;

    // Lire la structure complète
    if (fread(&config_file, sizeof(config_file), 1, file) != 1) {
        fclose(file);
        return -1;
    }

    fclose(file);

    // Vérifier le magic number
    if (config_file.magic != NEMESIS_CONFIG_MAGIC) {
        fprintf(stderr, "Erreur: Magic number invalide (0x%X au lieu de 0x%X)\n",
                config_file.magic, NEMESIS_CONFIG_MAGIC);
        return -1;
    }

    // Vérifier la version
    if (config_file.version != NEMESIS_VERSION) {
        fprintf(stderr, "Erreur: Version incompatible (%u au lieu de %u)\n",
                config_file.version, NEMESIS_VERSION);
        return -1;
    }

    // Vérifier le checksum
    __uint32_t saved_checksum = config_file.checksum;
    config_file.checksum = 0;  // Mettre à zéro pour recalculer

    size_t data_size = sizeof(config_file) - sizeof(config_file.checksum);
    uLong calculated_checksum = crc32(0uL, Z_NULL, 0);
    calculated_checksum = crc32(calculated_checksum, (const Bytef *)&config_file, data_size);

    if (saved_checksum != calculated_checksum) {
        fprintf(stderr, "Erreur: Checksum invalide (0x%X au lieu de 0x%X)\n",
                calculated_checksum, saved_checksum);
        return -1;
    }

    // Restaurer les données dans la config
    config->input = config_file.input;
    config->attack = config_file.attack;
    config->output = config_file.output;
    config->system = config_file.system;
    config->meta.version = config_file.meta_version;
    config->input.save = 1;



    // Restaurer les pointeurs constants de meta (pas sauvegardés)
    config->meta.build_date = NEMESIS_BUILD_DATE;
    config->meta.author = NEMESIS_AUTHOR;

    return 0;
}



void NEMESIS_safe_save_config(void) {
    // Vérifier si un fichier existe déjà
    char full_path[NEMESIS_MAX_PATH];
    PATH_JOIN(full_path,NEMESIS_MAX_PATH,NEMESIS_config.output.config_dir,NEMESIS_STOPPED_FILE);
    if (File_exist(full_path)) {
        char res[64];
        print_slow("Une configuration existe déjà, l'écraser ? (Y/N) : ",SPEED_PRINT);
        fflush(stdout);

        if (fgets(res, sizeof(res), stdin) == NULL) return;

        if (res[0] != 'Y' && res[0] != 'y') {
            print_slow("Sauvegarde annulée.\n",SPEED_PRINT);

            return;
        }
    }

    if (NEMESIS_config.attack.enable_bruteforce) {save_brute_thread_states();delete_dict_thread_states();}
    else {save_dict_thread_states();delete_brut_thread_states();}
    FILE *f = fopen(full_path, "wb");
    if (!f) {
        perror("Impossible d'ouvrir le fichier de sauvegarde");
        return;
    }

    if (NEMESIS_save_config(f, &NEMESIS_config) != 0) {
        fprintf(stderr, "Erreur lors de la sauvegarde de la configuration\n");
        return;
    }

    PRINT_SLOW_MACRO(SPEED_PRINT,"Configuration sauvegardée dans '%s'\n", NEMESIS_STOPPED_FILE);
}

/*int NEMESIS_save_config(FILE *file,NEMESIS_config_t *config) {
    if (!file || !config) return -1;

    struct NEMESIS_config_file config_file;

    config_file.magic   = NEMESIS_CONFIG_MAGIC;
    config_file.version = NEMESIS_VERSION;
    config_file.NEMESIS_config = *config;

    printf("NEMESIS_config : %s\n",config_file.NEMESIS_config.input.shadow_file);
    uLong checksum = crc32(0uL, Z_NULL, 0);
    checksum = crc32(checksum,(const Bytef *)&config_file.NEMESIS_config,sizeof(NEMESIS_config_t));

    config_file.checksum = checksum;

    if (fwrite(&config_file, sizeof(config_file), 1, file) != 1)
        return -1;

    fclose(file);
    return 0;
}*/


/*int NEMESIS_load_config(FILE *file,NEMESIS_config_t *config) { // la fonctrion appelante devra ouvrir le fichier

    if (!file || !config) return -1;

    struct NEMESIS_config_file config_file;
    fread(&config_file, sizeof(config_file), 1, file);
    fclose(file);

    if (config_file.magic!=NEMESIS_CONFIG_MAGIC)return -1;
    if (config_file.version!=NEMESIS_VERSION)return -1;

    uLong checksum = crc32(0uL,Z_NULL,0);
    checksum = crc32(checksum,(const Bytef *)&config_file.NEMESIS_config,sizeof(NEMESIS_config_t));

    if (checksum!=config_file.checksum)return -1;

    *config = config_file.NEMESIS_config;
    return 0;

}*/

