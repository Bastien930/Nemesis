//
// Created by bastien on 10/24/25.
//

#include "../Include/brute_force.h"


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../Include/brute_force.h"

void attaque_bruteforce(char* machaine,int sizemax){
    int boucle=1;
    int sauvegarde=0;
    int i=0;
    int pointage=0;
    char caracteres[127 - 32 + 1];  // 95 caracteres + '\0'
    /*for (int i = 32; i <= 126; i++) {
        caracteres[i - 32] = (char)i;
    }
    caracteres[126 - 32 + 1] = '\0'; // terminaison*/
    for (int i = 97; i <= 100; i++) {
        caracteres[i - 97] = (char)i;
    }
    caracteres[100 - 97 + 1] = '\0';
    printf("%s\n", caracteres);
    printf("\n\n----------------------\n\n");
    while(boucle <= sizemax){
        if(boucle-1 == pointage){
            if(caracteres[i] != '\0'){ //Si on n'a pas fini tous les caracteres
                machaine[pointage] = caracteres[i]; //On met le caractere suivant
                i++; //On prendra le prochain caractere
            }
            else{ //si on a fini tous les caracteres
                if(pointage != 0){ //on regarde si le pointage est le dernier   faire
                    machaine[pointage] = caracteres[0];
                    pointage--;
                    if(pointage<=boucle-1){
                        char *ptr = strchr(caracteres, machaine[pointage]); //a continuer (bug)
                        sauvegarde = ptr - caracteres;
                    }
                }
                else{
                    machaine[pointage] = caracteres[0];
                    pointage = pointage+boucle; //si c' tait le dernier, on le d fini au prochain nouveau caractere
                    boucle++;
                }
                i=0;
            }
            printf("%s\n", machaine);
        }
        else{
            /*if(pointage==0){
                pointage = boucle;
                boucle++;
                sauvegarde = 0;
            }
            else{*/
            if(caracteres[sauvegarde+1]=='\0'){
                sauvegarde = 0;
                machaine[pointage] = caracteres[sauvegarde];
                pointage = boucle;
                boucle++;
            }
            else{
                machaine[pointage] = caracteres[sauvegarde+1];
                pointage = strlen(machaine)-1;
            }
        }
    }
}

void init_bruteforce(){
    char unechaine[3];
    attaque_bruteforce(unechaine,3);
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
