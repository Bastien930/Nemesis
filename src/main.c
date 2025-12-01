//
// Created by bastien on 10/21/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include "../Include/main.h"

#include "../Include/Config.h"
#include "../Include/Option.h"
#include "../Include/Utils.h"
#include "../Include/Hash_Engine.h"
#include "../Include/Mangling.h"
#include "../Include/log.h"
#include <unistd.h>
#include <crypt.h>
#include <omp.h>

#include "brute_force.h"

#define RED(string) "\x1b[31m" string "\x1b[0m"

struct da_config_t da_config = {0};

int main(int argc, char* argv[]){

    char errbuff[DA_ERRLEN];

    print_banner();
    //sleep(10);
    init_time();
    da_config_init_default(&da_config);





    if (parse_args(argc, argv, &da_config) == 2) {
        return 0;
    }

    if (parse_args(argc, argv, &da_config) != 0) {
        return EXIT_FAILURE;
    }
    //if (da_config_validate(&da_config,errbuff,DA_ERRLEN) != 0) {
    //    puts(errbuff);
    //    return EXIT_FAILURE;
    //}

    // ===== temporaire ====== //
    const char *password = "Password123";
    char *hashed = "zJq7O87dzcdJ0wonNAzYlD1YxJJ.suzRv3QJAhmllSmnClaRM085JqTuawh.Y9ePO2o5gafSJJBQBRst0SVd11"; // commande pour hash openssl passwd -6 -salt abcd1234 '$y(4'
    //char *hashed = "uPC3x7H6BVL5jdWZGWSqAZuS7F2f2qNXwtLqocfjC50"; // commande pour hash openssl passwd -5 -salt abcd1234 Password123
    if (!init_log("../log.txt",LOG_DEBUG)) perror("log init");
    struct da_shadow_entry *e = malloc(sizeof(*e));
    e->username = strdup("user");
    e->hash     = strdup(hashed); // $5 sha 256
    //e->salt     = strdup("$y$j9T$abcd1234$");
    e->salt     = strdup("$6$abcd1234$");
    e->algo     = DA_HASH_SHA512;
    printf("%s\n%s\n%s\n",e->username,e->hash,e->salt);
    if (!da_hash_engine_init(e)) perror("engine init");

    omp_set_num_threads(16);

    bruteforce();
    //#pragma omp parallel for schedule(dynamic) // pbar de progression
    //for (int i=0;i<100;i++) {
    //    generate_mangled_words("hello la team",get_config_fast());
    //}

    free(e->username);
    free(e->hash);
    free(e->salt);
    free(e);

    show_result();


    /*
    int opt;
    static struct option long_options[] = {
        {"bruteforce", no_argument, 0, 'b'}, //bruteforce "naif"
        {"mangling", required_argument, 0, 'm'}, //le mangling obligatoirement : un argument qui est un entier (pour le nombre de fois...) A DEFINIR
        {"help", no_argument, 0, 'h'}, //le help : juste de l'aide
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "bm:h", long_options, NULL)) != -1) {
        switch(opt) {
        case 'b':
            printf("Generation du bruteforce dans 3sec...\n");
            sleep(3);
            init_bruteforce();
            break;
        case 'm':
            printf("Le mangling va commencer pour %s iterations de caracteres dans 3sec...\n", optarg); //optarg est une variable globale : si on met un chiffre ("3"), il va le stocker : important pour la suite
            sleep(3);
            break;
        case 'h':
            printf("Pour utiliser : ./programme [--bruteforce] OU [--mangling CHIFFRE] OU [--help]\n");
            exit(0);
        default:
            fprintf(stderr, RED("L'option n'est pas connue !\n"));
            exit(1);
        }
    }
    */
    return 0;
}
/*int main(int argc, char **argv) {
    const char *pwd = (argc > 1) ? argv[1] : "motdepasse_test";


    const char *salt = "$y$j9T$abcd1234$";

    char *res = crypt(pwd, salt);

    if (res == NULL) {
        perror("crypt");
        return 2;
    }

    printf("Password : %s\n", pwd);
    printf("Salt     : %s\n", salt);
    printf("Hash     : %s\n", res);

    if (strncmp(res, "$y$", 3) == 0) {
        puts("=> yescrypt DETECTÉ (hash commence par $y$)");
    } else {
        puts("=> yescrypt NON détecté (hash n'utilise pas le préfixe $y$)");
    }

    return 0;
}*/
