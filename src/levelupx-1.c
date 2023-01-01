#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

static uid_t euid, ruid;
static gid_t egid, rgid;
static char *logfile = "levelupx-1";

void log_message(const char *format, ...) {
    va_list args;
    va_start (args, format);
    FILE *f;
    char *filename = (char *)malloc(strlen(logfile) + 10);

    sprintf(filename, "/var/log/%s", logfile);
    f = fopen(filename, "a+");
    if (f == NULL) {
        perror("Could not open log");
        abort();
    }
    vfprintf(f, format, args);
    fclose(f);
    va_end (args);
    free(filename);
}

void elevate_permissions_until_end(char *reason) {
    log_message("Elevating permissions until end for '%s'\n", reason);
    setuid(euid);
    setgid(egid);
}

void lower_permissions(char *reason) {
    setreuid(euid, ruid);
    setregid(egid, rgid);
    log_message("Lowering permissions after '%s'\n", reason);
}

void save_permissions() {
    euid = geteuid();
    ruid = getuid();
    egid = getegid();
    rgid = getgid();
}

void shell_exec(char *command, int elevated) {
    pid_t pid;
    int *status;

    if (elevated) {
        pid = fork(); 
        if (!pid) {
            elevate_permissions_until_end(command);
            fclose(stdin);
            system(command);
            exit(0);
        } 
        wait(status);
    } else {
        system(command);
    }
}

int handle_ping(char *line) {
    char destination[128];
    char *cmd;

    if (sscanf(line, "ping %127s", destination) > 0) {
        if (isalnum(destination[0])) {
            printf("Pinging %s\n", destination);
            cmd = (char*)malloc(14 + strlen(destination));
            sprintf(cmd, "ping -c1 -W1 %s", destination);
            shell_exec(cmd, 1);
            free(cmd);
            return 0;
        }
    }
    return -1;
}

int handle_whoami(char *line) {
    shell_exec("whoami", 0);
    return 0;
}

int handle_ps(char *line) {
    shell_exec("ps", 0);
    return 0;
}

int handle_shell(char *line) {
    char *args[] = {
        "/bin/sh",
        NULL
    };
    pid_t pid;
    int *status;
    pid = fork(); 
    if (!pid) {
        elevate_permissions_until_end(line);
        execvp(args[0], args);
        exit(0);
    } 
    wait(status);
    return 0;
}

int handle_exit(char *line) {
    return -2;
}

typedef int command_function(char *);
typedef struct command_item {
    char *prefix;
    char *description;
    command_function *function;
    int hidden;
} command_item;

static command_item commands[] = {
    {"ping", "send ping", handle_ping, 0},
    {"whoami", "display username", handle_whoami, 0},
    {"ps", "display running processes", handle_ps, 0},
    {"_shell", "", handle_shell, 1},
    {"exit", "exit shell", handle_exit, 0}
};

int handle_input(char *line) {
    char command[128];
    int i;

    if (sscanf(line, "%127s ", command) > 0) {
        if (command) {
            if (!strcmp(command, "help")) {
                puts("Commands:");
                for (i = 0; i < sizeof(commands)/sizeof(command_item); i++) {
                    if (commands[i].hidden == 0) {
                        printf("%s - %s\n", commands[i].prefix, commands[i].description);
                    }
                }
                return 0;
            } else {
                for (i = 0; i < sizeof(commands)/sizeof(command_item); i++) {
                    if (!strcmp(command, commands[i].prefix)) {
                        return commands[i].function(line);
                    }
                }
            }
        }
    }
    return -1;
}

int main(int argc, char **argv) {
    char *line = NULL;
    int ret;
    size_t len = 0;
    ssize_t nread;

    puts("Welcome to the LevelUpX Swagricator shell!");
    save_permissions();
    lower_permissions("startup");
    printf("> ");
    while ((nread = getline(&line, &len, stdin)) != -1) {
        line[nread-1] = 0x00;
        ret = handle_input(line);
        log_message("Executed: '%s', ret: %d\n", line, ret);
        switch (ret) {
            case -1:
            printf("Invalid input '%s'\n", line);
            break;
            case -2:
            printf("Logoff\n");
            return 0;
            break;
        }
        free(line);
        line = NULL;
        printf("> ");
    }
    return 0;
}