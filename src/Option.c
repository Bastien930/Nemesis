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

#include "Utils.h"

int parse_args(int argc, char *argv[], NEMESIS_config_t *cfg) {
    int option;
    int long_index = 0;
    int return_value = 0;

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
                cfg->attack.mangling_config = NEMESIS_MANGLING_BALANCED;
            } else if (optarg && strcmp(optarg, "agressive") == 0) {
                cfg->attack.mangling_config = NEMESIS_MANGLING_AGGRESSIVE;
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
                cfg->attack.charset_preset = NEMESIS_CHARSET_PRESET_DEFAULT;
            } else if (strcmp(optarg, "alphanum") == 0) {
                cfg->attack.charset_preset = NEMESIS_CHARSET_PRESET_ALPHANUM;
            } else if (strcmp(optarg, "numeric") == 0) {
                cfg->attack.charset_preset = NEMESIS_CHARSET_PRESET_NUMERIC;
            } else {
                cfg->attack.charset_preset = NEMESIS_CHARSET_PRESET_CUSTOM;
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
            printf("avant print\n");
            NEMESIS_print_usage(argv[0]);
            return_value = 2;
            break;
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
                cfg->output.format = NEMESIS_OUT_CSV;
            else if (strcmp(optarg, "json") == 0)
                cfg->output.format = NEMESIS_OUT_JSON;
            else if (strcmp(optarg, "xml") == 0)
                cfg->output.format = NEMESIS_OUT_XML;
            else
                cfg->output.format = NEMESIS_OUT_TXT;
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
            puts(STR(NEMESIS_VERSION));
            exit(EXIT_SUCCESS);
        }
        case OPT_RESUME: {
            cfg->input.save = 1;
            if (return_value<2)
                return_value = 1;
            break;
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

    return return_value;
}
