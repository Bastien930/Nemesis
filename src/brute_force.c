//
// Created by bastien on 10/24/25.
//

#include "brute_force.h"
#include "Config.h"
#include "Utils.h"
#include "log.h"


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../Include/brute_force.h"

#include "Hash_Engine.h"
#include "Mangling.h"

static inline void construire_mot_depuis_index(long long n,char *mot,const char *charset,int b, int max_len);

void da_bruteforce(void) {

    char caracteres[256];

    int len = build_charset(caracteres, sizeof(caracteres),
                            da_config.attack.charset_preset,
                            da_config.attack.charset_custom);

    if (len < 0) { write_log(LOG_ERROR,"Erreur lors de l'initialisation du charset","brutforce");return;    }

    const int b = len;
    const int length = DA_MAX_LEN;
    const long long total = puissance(len,DA_MAX_LEN);

    #pragma omp parallel
    {
        char word[DA_MAX_LEN+1];
        word[length] = '\0';

    #pragma omp for schedule(static)
        for (long long n = 0; n < total; n++) {

            if (is_password_found()) continue;

            construire_mot_depuis_index(n, word,caracteres,b,length);

            da_hash_compare(word,NULL);
            //if (n==5100000) printf("%d : %s\n",n,word);

            long long count = da_hash_get_count();
            if (count%10000==0) {
            #pragma omp critical
              {
                  print_progress_bar(count,total,word,false);
               }
             }
        }
    }
    char last_word[DA_MAX_LEN+1] = {'\0'};
    if (!is_password_found()) construire_mot_depuis_index(total,last_word,caracteres,b,length);
    else get_found_password(last_word,DA_MAX_LEN+1);
    print_progress_bar(total,total,last_word,true);
}

/*void da_bruteforce_mangling(ManglingConfig *config) {

    char caracteres[256];

    int len = build_charset(caracteres, sizeof(caracteres),
                            da_config.attack.charset_preset,
                            da_config.attack.charset_custom);

    if (len < 0) { write_log(LOG_ERROR,"Erreur lors de l'initialisation du charset","brutforce");return;    }

    const int b = len;
    const int length = DA_MAX_LEN;
    const long long total = puissance(len,DA_MAX_LEN);
    const int mangling_itération = DA_GET_ITERATION_OF_MANGLING(da_config.attack.mangling_config);

    #pragma omp parallel
    {
        char word[DA_MAX_LEN+1];
        word[length] = '\0';

    #pragma omp for schedule(static)
        for (long long n = 0; n < total; n++) {

            if (is_password_found()) continue;

            construire_mot_depuis_index(n, word, caracteres, b, length);

            generate_mangled_words(word,config);// le mangling verifie le mot de base...
            //if (n==5100000) printf("%d : %s\n",n,word);

            long long count = da_hash_get_count();
            if (count%10000==0) {
                #pragma omp critical
                {
                    print_progress_bar(count*mangling_itération,total*mangling_itération,word,false);
                }
            }
        }
    }
    char last_word[DA_MAX_LEN+1] = {'\0'};
    if (!is_password_found()) construire_mot_depuis_index(total,last_word,caracteres,b,length);
    else get_found_password(last_word,DA_MAX_LEN+1);
    print_progress_bar(total, total, last_word, true);

    printf("total : %llu\n",total);
}*/


int build_charset(char *out, size_t out_size,
                  da_charset_preset_t preset,
                  const char *custom_charset)
{
    size_t pos = 0;

    switch (preset) {

        case DA_CHARSET_PRESET_DEFAULT:
            // ASCII imprimable (32 à 126)
            for (int c = 32; c <= 126; c++) {
                if (pos + 1 >= out_size) return -1;
                out[pos++] = (char)c;
            }
            break;

        case DA_CHARSET_PRESET_ALPHANUM:
            // A-Z
            for (char c = 'A'; c <= 'Z'; c++) out[pos++] = c;
            // a-z
            for (char c = 'a'; c <= 'z'; c++) out[pos++] = c;
            // 0-9
            for (char c = '0'; c <= '9'; c++) out[pos++] = c;
            break;

        case DA_CHARSET_PRESET_NUMERIC:
            for (char c = '0'; c <= '9'; c++) out[pos++] = c;
            break;

        case DA_CHARSET_PRESET_CUSTOM:
            if (!custom_charset) return -2;
            pos = strlen(custom_charset);
            if (pos + 1 >= out_size) return -3;
            memcpy(out, custom_charset, pos);
            break;

        default:
            return -4;
    }

    out[pos] = '\0';  // terminaison string C
    return pos;       // retourne la taille
}


static inline void construire_mot_depuis_index(long long n,char *mot,const char *charset,int b, int max_len)
{
    int longueur = 1;
    long long plage = b;
    long long idx = n;

    // Calcul de la longueur du mot
    while (idx >= plage && longueur < max_len) {
        idx -= plage;
        longueur++;
        plage *= b;
    }

    mot[longueur] = '\0';

    // Construction du mot (base b)
    for (int i = longueur - 1; i >= 0; i--) {
        mot[i] = charset[idx % b];
        idx /= b;
    }
}



//a pointage = 0
//b pointage = 0
//c pointage = 0
//aa pointage = 1
//ab pointage = 1
//ac pointage = 1
//ba pointage = 1 Retour arriere
//bb pointage = 1
//bc pointage = 1
//ca pointage = 1
//cb pointage = 1
//cc pointage = 1
//aaa pointage = 2
//aab pointage = 2
//aac pointage = 2
//aba pointage = 2 Retour arriere
//abb pointage = 2
//abc pointage = 2
//aca pointage = 2 Retour arriere
//acb pointage = 2
//acc pointage = 2
//baa pointage = 2 Retour arriere *2
