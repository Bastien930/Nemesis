//
// Created by bastien on 10/24/25.
//

#include "../Include/Option.h"
#include "../Include/Config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>

int parse_args(int argc, char *argv[], da_config_t *cfg) {
    int option;
    int long_index = 0;

    while ((option = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
        //printf("parse args : %d : %s: %d\n",option,optarg,optind);
        switch (option) {
        case 's': {
            strncpy(cfg->input.shadow_file, optarg, sizeof(cfg->input.shadow_file) - 1);
            cfg->input.shadow_file[sizeof(cfg->input.shadow_file)-1] = '\0';
            break;
        }

        case 'w': {
            strncpy(cfg->input.wordlist_file, optarg, sizeof(cfg->input.wordlist_file) - 1);
            cfg->input.wordlist_file[sizeof(cfg->input.wordlist_file)-1] = '\0';
            break;
        }

        case 'm': {
            cfg->attack.enable_mangling = true;
            if (optarg && strcmp(optarg, "balanced") == 0) {
                cfg->attack.mangling_config = DA_MANGLING_BALANCED;
            } else if (optarg && strcmp(optarg, "alphanum") == 0) {
                cfg->attack.mangling_config = DA_MANGLING_AGGRESSIVE;
                // sinon deja Fast par defaut.
            }
            break;
        }

        case 'd': {
            cfg->attack.enable_dictionary = true;
            break;
        }

        case 'b': {
            cfg->attack.enable_bruteforce = true;
            break;
        }

        case 'c': {
            if (strcmp(optarg, "default") == 0) {
                cfg->attack.charset_preset = DA_CHARSET_PRESET_DEFAULT;
            } else if (strcmp(optarg, "alphanum") == 0) {
                cfg->attack.charset_preset = DA_CHARSET_PRESET_ALPHANUM;
            } else if (strcmp(optarg, "numeric") == 0) {
                cfg->attack.charset_preset = DA_CHARSET_PRESET_NUMERIC;
            } else {
                cfg->attack.charset_preset = DA_CHARSET_PRESET_CUSTOM;
                strncpy(cfg->attack.charset_custom, optarg, sizeof(cfg->attack.charset_custom)-1);
                cfg->attack.charset_custom[sizeof(cfg->attack.charset_custom)-1] = '\0';
            }
            break;
        }

        case 't': {
            cfg->system.threads = (unsigned)atoi(optarg);
            break;
        }

        case 'o': {
            strncpy(cfg->output.output_file, optarg, sizeof(cfg->output.output_file) - 1);
            cfg->output.output_file[sizeof(cfg->output.output_file)-1] = '\0';
            break;
        }

        case 'h': {
            da_print_usage(argv[0]);
            return 2;
        }

        case OPT_MIN: {
            cfg->attack.min_len = atoi(optarg);
            break;
        }

        case OPT_MAX: {
            cfg->attack.max_len = atoi(optarg);
            break;
        }

        case OPT_FORMAT: {
            if (strcmp(optarg, "csv") == 0)
                cfg->output.format = DA_OUT_CSV;
            else
                cfg->output.format = DA_OUT_TXT;
            break;
        }

        case OPT_LOG: {
            cfg->output.enable_logging = true;
            strncpy(cfg->output.log_file, optarg, sizeof(cfg->output.log_file) - 1);
            cfg->output.log_file[sizeof(cfg->output.log_file)-1] = '\0';
            break;
        }

        case OPT_ENABLE_GPU: {
            cfg->system.enable_gpu = true;
            break;
        }

        case OPT_MAX_ATTEMPTS: {
            cfg->attack.max_attempts = atoi(optarg);
            break;
        }

        case OPT_VERSION: {
            puts(STR(DA_VERSION));
            exit(EXIT_SUCCESS);
        }

        default: {
            return -1;
        }
        }

    }
    if (optind < argc) {
        fprintf(stderr, "[Erreur] Argument(s) non reconnu(s) :\n");
        for (int i = optind; i < argc; i++) {
            fprintf(stderr, "  %s\n", argv[i]);
        }
        return -1;
    }


    return 0;
}