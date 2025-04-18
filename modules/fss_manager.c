#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ADTQueue.h"
#include "ADTHash.h"

#define MAX_WORKERS 5


void log_date(int fd_log) {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    printf("[%04d-%02d-%02d %02d:%02d:%02d]\n", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
}

int init_system(char* fss_in, char* fss_out, char* configfile, char* managerlog) {
    int check, fd_in, fd_out, fd_log, fd_config;

    if (access(fss_in, F_OK) == 0) {
        check = unlink(fss_in);
        if (check == -1) {
            perror("Error deleting fss_in\n");
        }

        if (mkfifo(fss_in, 0644) == -1) {
            perror("Error creating the fss_in\n");
        }

        fd_in = open(fss_in, O_RDONLY | O_NONBLOCK);
        if (fd_in == -1) {
            perror("Eror opening the fss_in\n");
        }
    } else {
        fd_in = open(fss_in, O_RDONLY | O_NONBLOCK);
        if (fd_in == -1) {
            perror("Eror opening the fss_in\n");
        }
    }

    if (access(fss_out, F_OK) == 0) {
        check = unlink(fss_out);
        if (check == -1) {
            perror("Error deleting fss_out\n");
        }

        if (mkfifo(fss_out, 0644) == -1) {
            perror("Error creating the fss_out\n");
        }

        fd_out = open(fss_out, O_WRONLY | O_NONBLOCK);
        if (fd_out == -1) {
            perror("Eror opening the fss_out\n");
        }
    } else {
        fd_out = open(fss_out, O_WRONLY | O_NONBLOCK);
        if (fd_out == -1) {
            perror("Eror opening the fss_out\n");
        }
    }

    fd_log = open(managerlog, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd_log == -1) {
        perror("Error opening the manager log file\n");
    }

    fd_config = open(configfile, O_RDONLY | O_CREAT, 0644);
    if (fd_config == -1) {
        perror("Error opening the config file\n");
    }

    char ch;
    char source_file[1000];
    char target_file[1000];
    int source_target = 0;
    int i = 0;
    int line_count = 0;
    Queue tasks = queue_create(NULL);

    FILE *file = fopen(configfile, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            line_count++;
        }
    }
    fclose(file);

    HashTable directories = hash_create(line_count, NULL, false);
    file = fopen(configfile, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    while ((ch = fgetc(file)) != EOF) {
        if (ch == ' ') {
            source_file[i] = '\0';
            source_target = 1;
            i = 0;
            continue;
        }
        
        if (ch == '\n') {
            target_file[i] = '\0';
            ConfigFile cf = malloc(sizeof(*cf));
            strcpy(cf->source, source_file);
            strcpy(cf->target, target_file);
            hash_insert(directories, cf);
            queue_insert_back(tasks, cf);
            source_target = 0;
            i = 0;
        }

        if (source_target == 0) {
            source_file[i] = ch;
        } else {
            target_file[i] = ch;
        }

        i++;
    }
    fclose(file);
    

    int workers = 0;
    HashTable active_workers = hash_create(MAX_WORKERS, NULL, true);
    while (queue_size(tasks) > 0 || workers > 0) {
        ConfigFile task = queue_front(tasks);
        int pipefd[2]; // pipefd[0] is for reading, pipefd[1] is for writing
        if (pipe(pipefd) == -1) {
            perror("pipe failed");
            // Handle error appropriately (e.g., continue to next iteration? exit?)
            continue;
        }

        if (queue_size(tasks) > 0 && workers < MAX_WORKERS) {
            pid_t pid = fork();
            if (pid == 0) {
                close(pipefd[0]);
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    perror("dup2 failed");
                    exit(1); // Worker cannot report, critical error
                }
                close(pipefd[1]); // Close original write end after dup2
                char *source_path = task->source;
                char *target_path = task->target;

                char *worker_argv[] = {"./worker", source_path, target_path, "ALL", "FULL", NULL};
                execv("./worker", worker_argv); // Εκτελεί το worker πρόγραμμα [source: 40]

                // Αν η execv επιστρέψει, σημαίνει ότι απέτυχε!
                perror("execv failed");
                exit(1); // Σημαντικό να τερματίσει το παιδί αν αποτύχει η execv
            } else if (pid > 0) {
                Worker worker = create_worker(pid, pipefd[0], task);
                hash_insert(active_workers, worker);
                close(pipefd[1]);
                queue_remove_front(tasks);
                workers++;
            } else {
                perror("fork");
                return 1;
            }
        }
        pid_t terminated_pid;
        int status;
        // Έλεγξε αν *οποιοδήποτε* παιδί έχει τερματίσει ΧΩΡΙΣ να περιμένεις
        while ((terminated_pid = waitpid(-1, &status, WNOHANG)) > 0) {
            // Ένας worker τερμάτισε!
            printf("Manager: Worker with PID %d finished.\n", terminated_pid);

            Worker worker = hash_search(active_workers, terminated_pid);

            close(worker->read_pipe_fd);

            hash_remove(active_workers, terminated_pid);

            workers--;
        }
        // Αν waitpid επέστρεψε 0, κανείς δεν τερμάτισε αυτή τη στιγμή.
        // Αν επέστρεψε -1 (και errno != ECHILD), έγινε σφάλμα.
        if (terminated_pid == -1 && errno != ECHILD) {
            perror("waitpid error");
        }
    }
}