//
// Created by Basti on 26/11/2025.
//

#ifndef NEMESIS_LOG_H
#define NEMESIS_LOG_H


#include <stdio.h>


typedef enum {
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
} LogLevel;

typedef struct {
    FILE *file;
    char filename[256];
    int log_level;
} LogConfig;

extern LogConfig* NEMESIS_log_config;


void write_log(LogLevel level, const char *message,const char *location) ;

int init_log(const char *filename, int level) ;

void close_log(void) ;


/*
int main() {
    LogConfig logger;
    if (init_log(&logger, "application.log", LOG_DEBUG) != 0) {
        return EXIT_FAILURE;
    }

    write_log(&logger, LOG_INFO, "Démarrage de l'application");
    write_log(&logger, LOG_WARNING, "Ceci est un avertissement");
    write_log(&logger, LOG_ERROR, "Une erreur est survenue");
    write_log(&logger, LOG_DEBUG, "Message de debug");

    close_log(&logger);
    return EXIT_SUCCESS;
}*/


#endif //NEMESIS_LOG_H