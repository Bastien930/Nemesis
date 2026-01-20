//
// Created by Basti on 26/11/2025.
//
#include "log.h"
#include "Config.h"
#include "Utils.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>



LogConfig * log_config = NULL;

/**
 * Initialiser le système de journalisation
 *
 * @param filename Nom du fichier de log à créer/ouvrir
 * @param level Niveau de log initial
 * @return 1 si succès, -1 si erreur
 */
int init_log(const char *filename, int level) { // le level correspond au level initial.
    log_config = malloc(sizeof(LogConfig));
    if (!log_config) { perror("allocation dynamique de init_log"); return -1; }
    char fullpath[NEMESIS_MAX_PATH];
    PATH_JOIN(fullpath,NEMESIS_MAX_PATH,NEMESIS_config.output.log_dir,filename);
    printf("log enregistrer à l'emplacement : %s\n",fullpath);
    log_config->file = fopen(fullpath, "a");
    if (!log_config->file) { perror("Erreur lors de l'ouverture du fichier de log"); return -1; }
    strncpy(log_config->filename, filename, sizeof(log_config->filename) - 1);
    log_config->filename[sizeof(log_config->filename) - 1] = '\0';
    log_config->log_level = level;
    return 1;
}

/**
 * Écrire un message dans le fichier de log
 *
 * @param level Niveau de priorité du message
 * @param message Contenu du message à journaliser
 * @param location Emplacement d'où provient le message
 */
void write_log(LogLevel level, const char *message, const char *location) {
    if (!log_config || !log_config->file) return;

    if (level > LOG_ERROR || level < LOG_DEBUG) level = log_config->log_level;

    const char *level_str[] = {"ERROR", "WARNING", "INFO", "DEBUG"};
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log_config->file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] at %s %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            level_str[level],
            location,
            message);
    fflush(log_config->file);
}

/**
 * Fermer et libérer les ressources du système de journalisation
 */
void close_log(void) {
    if (!log_config)return;
    if (log_config->file) {
        fclose(log_config->file);
        log_config->file = NULL;
    }
    free(log_config);
}