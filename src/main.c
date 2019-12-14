#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "shell.h"

#define VERSION 0.1

int main(int argc, char const *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("MeShell version %0.1f\n", VERSION);
            exit(0);
        }
    }
    database meshell_db = {
        .name = ":memory:",
        .db = NULL};
    init_db(&meshell_db);

    Shell shell = {.closed = false, .exit_code = 0, .sqldb = &meshell_db};
    start_shell(&shell);
    free(shell.cwd);

    close_db(&meshell_db);
    return 0;
}