//
// Created by bastien on 10/21/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
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
#include "Shdow_io.h"

#define RED(string) "\x1b[31m" string "\x1b[0m"

da_config_t da_config = {0};
//static struct da_shadow_entry *da_entry = NULL;
struct da_shadow_entry_list da_shadow_entry_liste = {0};
volatile sig_atomic_t interrupt_requested = 0;
char SavePath[128] = {STR(DIR_OF_EXE)};


static void run_attack(void);

static void global_cleanup(void) {
    da_free_shadow_entry_list(&da_shadow_entry_liste);
    close_log();
}


void crash_handler(int sig) {
    write_log(LOG_ERROR, "Crash détecté, nettoyage...", "signal");
    global_cleanup();
    puts("\n");
    _exit(128 + sig); // sortie immédiate et sûre
}

void save_handler(int sig) {
    interrupt_requested = 1;
    signal(SIGINT, SIG_DFL);
}

/*static void save_handler(int sig) {
    printf("dans le handler\n");
    atomic_store_explicit(&need_save,1, memory_order_relaxed);
    atomic_store_explicit(&last_sig,sig, memory_order_relaxed);
}*/

/*static void combine_handler(int sig) {
    save_handler(sig);
    sleep(5);
    crash_handler(sig);
}*/

int main(int argc, char* argv[]){

    char errbuff[DA_ERRLEN];

    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGINT,  crash_handler );
    signal(SIGTERM, crash_handler);

    atexit(global_cleanup);


    //print_banner();
    //sleep(10);

    da_config_init_default(&da_config);




    int retour_parse_args = parse_args(argc, argv, &da_config);
    printf("retour parse args : %d\n",retour_parse_args);
    if (retour_parse_args == 2) {
        return 0;
    }
    if (retour_parse_args==1) {
        int retourn = da_load_config(&da_config);
    }

    if (retour_parse_args < 0 ) {
        return EXIT_FAILURE;
    }
    if (da_config_validate(&da_config,errbuff,DA_ERRLEN) != 0) {
        puts(errbuff);
        return EXIT_FAILURE;
    }

    run_attack();


    return 0;
}

static void run_attack(void) {
    // boucler sur le nombre de shadow entry.

    // faire une fnct si brutforce et mangling alors lancer bruteforce mangling
    // si dictionnaire et mangling
    // si dictionnaire
    // si brutforce.
    // si mangling echec.
    printf("Flag run_attack\n");
    if (da_config.output.enable_logging) {if (!init_log(da_config.output.log_file,LOG_DEBUG)) perror("log init");}

    da_init_shadow_entry_list(&da_shadow_entry_liste);

    if (da_load_shadow_file(da_config.input.shadow_file,&da_shadow_entry_liste)<=0) {write_log(LOG_WARNING,"Aucune entrée shadow à été chargée","run_attack()");return;}



    // ===== temporaire ====== //
    /*const char *password = "Password123";
    char *hashed = "zJq7O87dzcdJ0wonNAzYlD1YxJJ.suzRv3QJAhmllSmnClaRM085JqTuawh.Y9ePO2o5gafSJJBQBRst0SVd11"; // commande pour hash openssl passwd -6 -salt abcd1234 '$y(4'
    //char *hashed = "uPC3x7H6BVL5jdWZGWSqAZuS7F2f2qNXwtLqocfjC50"; // commande pour hash openssl passwd -5 -salt abcd1234 Password123

    if (da_config.output.enable_logging) {if (!init_log(da_config.output.log_file,LOG_DEBUG)) perror("log init");}

    da_entry = malloc(sizeof(*da_entry));
    da_entry->username = strdup("user");
    da_entry->hash     = strdup(hashed); // $5 sha 256
    //e->salt     = strdup("$y$j9T$abcd1234$");
    da_entry->salt     = strdup("$6$abcd1234$");
    da_entry->algo     = DA_HASH_SHA512;
    printf("%s\n%s\n%s\n",da_entry->username,da_entry->hash,da_entry->salt);
    */

    omp_set_num_threads(da_config.system.threads); // par defaut 16.
    signal(SIGINT,  save_handler );
    for (int i=0;i<da_shadow_entry_liste.count;i++) {
        printf("il y a : %d shadow entry",da_shadow_entry_liste.count);
        printf("%s\n%s\n%s\n",da_shadow_entry_liste.entries[i]->username,da_shadow_entry_liste.entries[i]->hash,da_shadow_entry_liste.entries[i]->salt);

        init_time(); // le end time ce fait par les fnct de brut force.
        reset_found_password();

        if (!da_hash_engine_init(da_shadow_entry_liste.entries[i])) {write_log(LOG_ERROR,"erreur lors de l'initialisation du moteur de hash.","run_attack");perror("engine init");}



        if (da_config.attack.enable_dictionary && !da_config.attack.enable_bruteforce){;}

        if (da_config.attack.enable_bruteforce && !da_config.attack.enable_dictionary) {
            //if (da_config.attack.enable_mangling) da_bruteforce_mangling(da_getConfigMangling(da_config.attack.mangling_config));
            //else da_bruteforce();
            da_brute_status_t st = da_bruteforce();
            if (st == DA_BRUTE_INTERRUPTED)da_safe_save_config();
        }

        //#pragma omp parallel for schedule(dynamic) // pbar de progression
        //for (int i=0;i<100;i++) {
        //    generate_mangled_words("hello la team",get_config_fast());
        //}


        show_result(da_shadow_entry_liste.entries[i]); // rajouter le usernname oour chaque mdp
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
