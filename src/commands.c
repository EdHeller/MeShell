#include "commands.h"

// Commands return true if everything goes well
typedef bool (*CmdFunc)(Token* args, Shell* sh, char** err, char** output);

typedef struct cmd_item {
    char* name;
    CmdFunc func;
} CmdItem;

static bool cd(Token* args, Shell* sh, char** err, char** output) {
    Token* t = args;
    char* dir = NULL;

    // There's no arg token or maybe there's a redirect
    if (!t || t->type != ARG) {
        // We always use heap for cwd
        sh->cwd = (char*)realloc(sh->cwd, sizeof(char) * 2);
        strcpy(sh->cwd, "/");
        return true;
    }

    if (t->type == ARG) {
        dir = t->text;
    }

    if (t->next != NULL && t->next->type == ARG) {
        *err = "Too many args for cd";
        return false;
    }

    if (dir != NULL) {
        char invalid_chars[] = {'\\', '|', '<', '>'};
        for (int i = 0; i < 4; i++) {
            if (strchr(dir, invalid_chars[i]) != NULL) {
                *err = "Invalid character found in directory name";
                return false;
            }
        }

        int dir_len = strlen(dir);
        // Remove trailing '/' if exists
        if (dir_len > 1 && *(dir + dir_len - 1) == '/') {
            *(dir + dir_len - 1) = '\0';
            dir_len--;
        }

        if (!check_dir_exists(sh->sqldb->db, dir)) {
            *err = "No such file or directory";
            return false;
        }

        // If dir doesn't start with '/' then it must be a relative path
        if (dir[0] != '/') {
            int cwd_len = strlen(sh->cwd);
            // length + 2 for null terminated string and the '/'
            sh->cwd = (char*)realloc(sh->cwd, sizeof(char) * dir_len + cwd_len + 2);
            // if cwd is root, no '/' is needed
            if (cwd_len != 1)
                strcat(sh->cwd, "/");
            strcat(sh->cwd, dir);
        } else {
            // length + 1 for null terminated string
            sh->cwd = (char*)realloc(sh->cwd, sizeof(char) * dir_len + 1);
            strcpy(sh->cwd, dir);
        }
    } else {
        *err = "Unknown error happend.";
        return false;
    }

    return true;
}

static bool pwd(Token* args, Shell* sh, char** err, char** output) {
    if (args != NULL && args[0].type == ARG) {
        *err = "pwd does not accept arguments";
        return false;
    }
    *output = (char*)malloc(sizeof(char) * strlen(sh->cwd) + 1);
    strcpy(*output, sh->cwd);
    return true;
}

static bool exit_shell(Token* args, Shell* sh, char** err, char** output) {
    if (args != NULL && args[0].type == ARG) {
        sh->exit_code = atoi(args[0].text);
    }
    sh->closed = true;
    return true;
}

static bool echo(Token* args, Shell* sh, char** err, char** output) {
    if (args != NULL && args[0].type == ARG) {
        *output = (char*)malloc(sizeof(char) * strlen(args[0].text) + 2);
        strcpy(*output, args[0].text);
        strcat(*output, " ");
    }
    Token* tok = args[0].next;
    while (tok && tok->type == ARG) {
        *output = (char*)realloc(*output, sizeof(char) * strlen(*output) + strlen(tok->text) + 2);
        strcat(*output, tok->text);
        strcat(*output, " ");
        tok = tok->next;
    }

    return true;
}

#define TOTAL_COMMANDS 4
static const CmdItem cmd_lookup_table[] = {{"cd", cd}, {"pwd", pwd}, {"exit", exit_shell}, {"echo", echo}};

static CmdFunc find_command(Token cmd) {
    for (int i = 0; i < TOTAL_COMMANDS; ++i) {
        CmdItem ci = cmd_lookup_table[i];
        if (strcmp(cmd.text, ci.name) == 0) {
            return ci.func;
        }
    }
    return NULL;
}

void run_command(Token* tokens, Shell* sh, char** err) {
    if (tokens == NULL || tokens[0].type != COMMAND) {
        *err = "Error parsing the command";
        return;
    }

    CmdFunc func = find_command(tokens[0]);
    if (func == NULL) {
        *err = "Command not found";
        return;
    }
    char* out = NULL;
    char* error = NULL;
    bool result = func(tokens->next, sh, &error, &out);

    if (!result && error) {
        *err = error;
        return;
    }

    if (out) {
        printf("%s\n", out);
        free(out);
    }
}