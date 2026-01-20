//
// Created by bastien on 10/21/25.
//


#include "main.h"
#include "Config.h"
#include "Option.h"
#include "Utils.h"
#include "Hash_Engine.h"
#include "Mangling.h"
#include "log.h"
#include "brute_force.h"
#include "Dictionnary.h"
#include "Shdow_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <crypt.h>
#include <omp.h>

#define RED(string) "\x1b[31m" string "\x1b[0m"

NEMESIS_config_t NEMESIS_config = {0};
struct NEMESIS_shadow_entry_list NEMESIS_shadow_entry_liste = {0};
volatile sig_atomic_t interrupt_requested = 0;
char SavePath[128] = {STR(DIR_OF_EXE)};


static void run_attack(void);

/**
 * Libérer les ressources allouées par le programme
 */
static void global_cleanup(void) {
    NEMESIS_free_shadow_entry_list(&NEMESIS_shadow_entry_liste);
    close_log();
}


/**
 * Gérer les signaux de crash du programme
 * 
 * @param sig Signal reçu
 */
void crash_handler(int sig) {
    write_log(LOG_ERROR, "Crash détecté, nettoyage...", "signal");
    global_cleanup();
    puts("\n");
    _exit(128 + sig); // sortie immédiate et sûre
}

/**
 * Gérer l'interruption utilisateur pour sauvegarder l'état
 * 
 * @param sig Signal reçu
 */
void save_handler(int sig) {
    interrupt_requested = 1;
    signal(SIGINT, SIG_DFL);
}

/**
 * Point d'entrée principal du programme
 * 
 * @param argc Nombre d'arguments
 * @param argv Tableau des arguments
 * @return 0 en cas de succès, code d'erreur sinon
 */
int main(int argc, char* argv[]){

    char errbuff[NEMESIS_ERRLEN];

    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGINT,  crash_handler );
    signal(SIGTERM, crash_handler);

    atexit(global_cleanup);


    //print_banner();
    //sleep(2);

    NEMESIS_config_init_default(&NEMESIS_config);
    nemesis_init_paths();



    int retour_parse_args = parse_args(argc, argv, &NEMESIS_config);
    printf("retour arg : %d\n",retour_parse_args);
    if (retour_parse_args == 2) {
        return 0;
    }
    if (retour_parse_args==1) {
        int retourn = NEMESIS_load_config(&NEMESIS_config);
        printf("retour de load config : %d\n",retourn);
    }

    if (retour_parse_args < 0 ) {
        return EXIT_FAILURE;
    }
    if (NEMESIS_config_validate(&NEMESIS_config,errbuff,NEMESIS_ERRLEN) != 0) {
        puts(errbuff);
        return EXIT_FAILURE;
    }

    run_attack();


    return 0;
}

/**
 * Exécuter l'attaque configurée sur les entrées shadow
 * 
 * @return void
 */
static void run_attack(void) {

    if (NEMESIS_config.output.enable_logging) {if (init_log(NEMESIS_config.output.log_file,LOG_DEBUG) < 0) print_slow("erreur lors de l'initialisation des logs\n",SPEED_PRINT); else print_slow("Init log : Sucess\n",SPEED_PRINT);}

    NEMESIS_init_shadow_entry_list(&NEMESIS_shadow_entry_liste);

    if (NEMESIS_load_shadow_file(NEMESIS_config.input.shadow_file,&NEMESIS_shadow_entry_liste)<=0) {write_log(LOG_WARNING,"Aucune entrée shadow à été chargée","run_attack()");return;}



    // ===== temporaire ====== //
    /*const char *password = "Password123";
    char *hashed = "zJq7O87dzcdJ0wonNAzYlD1YxJJ.suzRv3QJAhmllSmnClaRM085JqTuawh.Y9ePO2o5gafSJJBQBRst0SVd11"; // commande pour hash openssl passwd -6 -salt abcd1234 '$y(4'
    //char *hashed = "uPC3x7H6BVL5jdWZGWSqAZuS7F2f2qNXwtLqocfjC50"; // commande pour hash openssl passwd -5 -salt abcd1234 Password123

    if (NEMESIS_config.output.enable_logging) {if (!init_log(NEMESIS_config.output.log_file,LOG_DEBUG)) perror("log init");}

    NEMESIS_entry = malloc(sizeof(*NEMESIS_entry));
    NEMESIS_entry->username = strdup("user");
    NEMESIS_entry->hash     = strdup(hashed); // $5 sha 256
    //e->salt     = strdup("$y$j9T$abcd1234$");
    NEMESIS_entry->salt     = strdup("$6$abcd1234$");
    NEMESIS_entry->algo     = NEMESIS_HASH_SHA512;
    printf("%s\n%s\n%s\n",NEMESIS_entry->username,NEMESIS_entry->hash,NEMESIS_entry->salt);
    */

    omp_set_num_threads(NEMESIS_config.system.threads); // par defaut 16.
    signal(SIGINT,  save_handler );
    for (int i=0;i<NEMESIS_shadow_entry_liste.count;i++) {
        //printf("il y a : %d shadow entry",NEMESIS_shadow_entry_liste.count);
        //printf("%s\n%s\n%s\n",NEMESIS_shadow_entry_liste.entries[i]->username,NEMESIS_shadow_entry_liste.entries[i]->hash,NEMESIS_shadow_entry_liste.entries[i]->salt);
        if (NEMESIS_shadow_entry_liste.count>1 && NEMESIS_config.input.save) {
            print_slow("L'option '--resume' va s'appliquer uniquement à la premiere entrée...\n",SPEED_PRINT);
        }

        init_time(); // le end time ce fait par les fnct de brut force.
        reset_found_password();
        printf("test1");
        if (!NEMESIS_hash_engine_init(NEMESIS_shadow_entry_liste.entries[i])) {write_log(LOG_ERROR,"erreur lors de l'initialisation du moteur de hash.","run_attack");perror("engine init");}
        printf("test");
        if (NEMESIS_config.attack.enable_dictionary && !NEMESIS_config.attack.enable_bruteforce) {
            NEMESIS_brute_status_t st;
            if (NEMESIS_config.attack.enable_mangling)
               st = NEMESIS_dictionary_attack_mangling(NEMESIS_config.input.wordlist_file,NEMESIS_getConfigMangling(NEMESIS_config.attack.mangling_config));
            else
                st = NEMESIS_dictionary_attack(NEMESIS_config.input.wordlist_file);
            if (st == NEMESIS_BRUTE_INTERRUPTED)NEMESIS_safe_save_config();
            if (st == NEMESIS_BRUTE_ERROR);
        }

        if (NEMESIS_config.attack.enable_bruteforce && !NEMESIS_config.attack.enable_dictionary) {
            printf("brute force");
            NEMESIS_brute_status_t st;
            if (NEMESIS_config.attack.enable_mangling)
                st = NEMESIS_bruteforce_mangling(NEMESIS_getConfigMangling(NEMESIS_config.attack.mangling_config));
            else
                st = NEMESIS_bruteforce();
            if (st == NEMESIS_BRUTE_INTERRUPTED)NEMESIS_safe_save_config();
            if (st == NEMESIS_BRUTE_ERROR);

        }

        //#pragma omp parallel for schedule(dynamic) // pbar de progression
        //for (int i=0;i<100;i++) {
        //    generate_mangled_words("hello la team",get_config_fast());
        //}


        show_and_write_result(NEMESIS_shadow_entry_liste.entries[i],NEMESIS_config.output.output_file,NEMESIS_config.output.format); // rajouter le usernname oour chaque mdp
    }

    // rajouter le faite de print dans la sortie en fnct du type de sortie
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
