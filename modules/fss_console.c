#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_CMD_LEN 500


char* log_date() {
    static char buffer[30];  // static: ζει και μετά το τέλος της συνάρτησης
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    snprintf(buffer, sizeof(buffer), "[%04d-%02d-%02d %02d:%02d:%02d]", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

    return buffer;
}

int console_in(char* command, char* fss_in, char* console_log) {
    int fd_in = open(fss_in, O_WRONLY);
    if (fd_in == -1) {
        perror("Error opening the pipe_in for reading\n");
        return -1;
    }

    int fd_console_log = open(console_log, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_console_log == -1) {
        perror("Error opening the log-file\n");
        close(fd_console_log);
        return -1;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s Command %s", log_date(), command);

    ssize_t bytes_cmd = write(fd_console_log, cmd, strlen(cmd));
    if (bytes_cmd == -1) {
        perror("Error writing in log-file\n");
    }

    ssize_t a = write(fd_in, command, strlen(command));
    if (a == -1) {
        perror("Error sending the command to fss_manager\n");
        return -1;
    }
    
    if (close(fd_in) == -1) {
        perror("Error closing logfile");
        return -1;
    }

    return 0;
}

int console_out(char* fss_out, char* console_log) {
    int fd_out = open(fss_out, O_RDONLY);
    if (fd_out == -1) {
        perror("Error opening the pipe_in for writing\n");
        return -1;
    }

    char buffer[1024];
    ssize_t bytes_read;
    
    int fd_console_log = open(console_log, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_console_log == -1) {
        perror("Error opening the log-file\n");
        close(fd_console_log);
        return -1;
    }

    bytes_read = read(fd_out, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        printf("Manager replied: %s\n", buffer);

        ssize_t bytes_wrote = write(fd_console_log, buffer, bytes_read);
        if (bytes_wrote == -1) {
            perror("Error writing in log-file\n");
        } else if (bytes_wrote != bytes_read) {
            fprintf(stderr, "Warning: Partially wrote to log-file...\n");
        }

    } else {
        perror("Error reading from pipe_out\n");
        return -1;
    }
    
    if (close(fd_console_log) == -1) {
        perror("Error closing logfile\n");
        return -1;
    }
    if (close(fd_out) == -1) {
        perror("Error closing pipe_out\n");
        return -1;
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Error not correct arguments\n");
        return 1;
    }

    int opt;
    char *console_log = argv[2];

    while ((opt = getopt(argc, argv, "l:")) != -1) {
        switch (opt) {
            case 'l':
                // console_log = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -l <logfile>\n", argv[0]);
                return 1;
        }
    }

    if (console_log == NULL) {
        fprintf(stderr, "Log file not specified. Use -l <logfile>\n");
        return 1;
    }

    char* pipe_in = "fss_in";
    char* pipe_out = "fss_out";

    char command[MAX_CMD_LEN];

    while (1) {
        if (fgets(command, MAX_CMD_LEN, stdin) == NULL) {
            break;
        }

        command[strcspn(command, "\n")] = '\0';

        log_date();

        printf("Command ");

        char *token = strtok(command, " ");
        printf("%s ", token);
        int cmd_add = 0;
        if (strcmp(token, "add") == 0) {
            cmd_add = 1;
        }
        while (token != NULL) {
            printf("%s \n", token);
            if (cmd_add == 1) {
                printf("-> ");
                cmd_add = 0;
            }
            token = strtok(NULL, " ");
        }

        printf("\n");

        if (console_in(command, (char*)pipe_in, console_log) == -1){
            continue;
        }

        if (console_out((char*)pipe_out, console_log) == -1) {
            continue;
        }

        if (strcmp(command, "shutdown") == 0) {
            break;
        }
    }

    return 0;
}