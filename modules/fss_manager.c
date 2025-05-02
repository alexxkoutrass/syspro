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
#include <sys/inotify.h>
#include <sys/select.h>

#include "ADTList.h"
#include "ADTQueue.h"
#include "ADTHash.h"


void synca(HashTable directories, HashTable active_workers, Queue tasks, int worker_limit, int fd_log) {
    Vector hash_dir = get_vector(directories);

    for (VectorNode node = vector_first(hash_dir); node != VECTOR_EOF; node = vector_next(hash_dir, node)) {
        List l = vector_node_value(hash_dir, node);
        for (ListNode l_node = list_first(l); l_node != LIST_EOF; l_node = list_next(l, l_node)) {
            ConfigFile cf = list_node_value(l, l_node);
            queue_insert_back(tasks, cf);
        }
    }

    while (queue_size(tasks) > 0 || get_size(active_workers) > 0) {
        ConfigFile task = queue_front(tasks);
        int pipefd[2]; // pipefd[0] is for reading, pipefd[1] is for writing
        if (pipe(pipefd) == -1) {
            perror("pipe failed");
            // Handle error appropriately (e.g., continue to next iteration? exit?)
            continue;
        }

        if (queue_size(tasks) > 0 && get_size(active_workers) < worker_limit) {
            pid_t pid = fork();
            if (pid == 0) {
                close(pipefd[0]);
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    perror("dup2 failed");
                    exit(1); // Worker cannot report, critical error
                }
                close(pipefd[1]); // Close original write end after dup2
                char* source_path = get_source(task);
                char* target_path = get_target(task);

                char *worker_argv[] = {"./worker", source_path, target_path, "ALL", "FULL", NULL};
                execv("./worker", worker_argv); // Εκτελεί το worker πρόγραμμα [source: 40]

                // Αν η execv επιστρέψει, σημαίνει ότι απέτυχε!
                perror("execv failed");
                exit(1); // Σημαντικό να τερματίσει το παιδί αν αποτύχει η execv
            } else if (pid > 0) {
                Worker worker = create_worker(pid, pipefd[0], task);
                char exec_report[1024];

                hash_insert(active_workers, worker);
                close(pipefd[1]);
                queue_remove_front(tasks);

                ssize_t bytes_read = read(pipefd[0], exec_report, 1024);
                printf("%s\n", exec_report);
                ssize_t a = write(fd_log, exec_report, strlen(exec_report));
                if (a == 0) {
                    printf("error\n");
                }
            } else {
                perror("fork");
            }
        }
        pid_t terminated_pid;
        int status;
        // Έλεγξε αν *οποιοδήποτε* παιδί έχει τερματίσει ΧΩΡΙΣ να περιμένεις
        while ((terminated_pid = waitpid(-1, &status, WNOHANG)) > 0) {
            // Ένας worker τερμάτισε!
            printf("Manager: Worker with PID %d finished.\n", terminated_pid);

            Worker worker = hash_search(active_workers, terminated_pid);

            close(get_read_pipe_fd(worker));

            hash_remove(active_workers, terminated_pid);
        }
        // Αν waitpid επέστρεψε 0, κανείς δεν τερμάτισε αυτή τη στιγμή.
        // Αν επέστρεψε -1 (και errno != ECHILD), έγινε σφάλμα.
        if (terminated_pid == -1 && errno != ECHILD) {
            perror("waitpid error");
        }
    }
}

void shutdown() {
    printf("Close everything! I DEMAAAAND YOU!!!!!!!!!!!!!!!!!\n");
}

void handle_console_event(HashTable directories, HashTable active_workers, Queue tasks, int fd_in, int fd_out, int fd_inotify, int worker_limit, int manager_log) {
    char buffer[1024];
    ssize_t bytes_read = read(fd_in, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

    } else {
        perror("Error reading from pipe_out\n");
    }

    char *token = strtok(buffer, " ");

    if (strcmp(token, "add") == 0) {
        char source[30];
        char target[30];

        int i = 0;
        while (token != NULL) {
            printf("Token: %s\n", token);
            token = strtok(NULL, " ");
            if (i == 0) {
                source[30] = token;
                i++;
            } else {
                target[30] = token;
            }
        }

        ConfigFile new_source = hash_search(directories, source);
        int wd = inotify_add_watch(fd_inotify, source, IN_CREATE | IN_MODIFY | IN_DELETE);
        set_watch(new_source, wd);
        printf("Added stuff\n");

        char answer[100] = "Manager returns to console";
        ssize_t a = write(fd_out, answer, str(answer));
        if (a == 0) {
            printf("error\n");
        }
    } else if (strcmp(token, "cancel") == 0) {
        char source[30];

        while (token != NULL) {
            printf("Token: %s\n", token);
            token = strtok(NULL, " ");
            source[30] = token;
        }

        ConfigFile cf = create_cf(0, NULL, source, NULL, 0);
        ConfigFile new_source = hash_search(directories, source);

        int wd = get_watch(new_source);
        inotify_rm_watch(fd_inotify, wd);
        printf("Removed stuf\n");
    } else if (strcmp(token, "status") == 0) {
        char source[30];

        while (token != NULL) {
            printf("Token: %s\n", token);
            token = strtok(NULL, " ");
            source[30] = token;
        }

        ConfigFile cf = create_cf(0, NULL, source, NULL, 0);
        ConfigFile new_source = hash_search(directories, source);

        char status[30];
        strcpy(status, get_status(cf));
        printf("Got the status\n");
    } else if (strcmp(token, "sync") == 0) {
        char source[30];

        while (token != NULL) {
            printf("Token: %s\n", token);
            token = strtok(NULL, " ");
            source[30] = token;
        }

        synca(directories, active_workers, tasks, worker_limit, manager_log);
    } else if (strcmp(token, "shutdown") == 0) {
        shutdown();
    } else {
        printf("Unknown token self exploding in 3 sec:)\n");
    }
}

void handle_inotify_event(HashTable directories, HashTable active_workers, Queue tasks, int fd_inotify, int worker_limit) {
    char buffer[1024];
    int length = read(fd_inotify, buffer, 1024);
    int i = 0;
    while (i < length) {
        struct inotify_event *event = (struct inotify_event *) &buffer[i];
    
        // Π.χ. Έλεγχος τύπου event:
        if (event->mask & IN_CREATE) {
            Vector hash_dir = get_vector(directories);

            ConfigFile cf;
            int end = 0;
            for (Vector node = vector_first(hash_dir); node != VECTOR_EOF; node = vector_next(hash_dir, node)) {
                List l = vector_node_value(hash_dir, node);
                for (ListNode l_node = list_first(l); l_node != LIST_EOF; l_node = list_next(l, l_node)) {
                    cf = list_node_value(l, l_node);
                    if (get_watch(cf) == event->wd) {
                        end = 1;
                        break;
                    }
                }
                if (end == 1) {
                    break;
                }
            }

            char source_filepath[500]; // Χρησιμοποίησε PATH_MAX ή κατάλληλο μέγεθος
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", get_source(cf), event->name);

            char target_filepath[500]; // Χρησιμοποίησε PATH_MAX ή κατάλληλο μέγεθος
            snprintf(target_filepath, sizeof(target_filepath), "%s/%s", get_target(cf), event->name);
            
            if (get_size(active_workers) < worker_limit) {
                int pipefd[2]; // pipefd[0] is for reading, pipefd[1] is for writing
                if (pipe(pipefd) == -1) {
                    perror("pipe failed");
                    // Handle error appropriately (e.g., continue to next iteration? exit?)
                    continue;
                }

                pid_t pid = fork();
                if (pid == 0) {
                    close(pipefd[0]);
                    if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                        perror("dup2 failed");
                        exit(1); // Worker cannot report, critical error
                    }
                    close(pipefd[1]); // Close original write end after dup2
    
                    char *worker_argv[] = {"./worker", source_filepath, target_filepath, "ADDED", event->name, NULL};
                    execv("./worker", worker_argv); // Εκτελεί το worker πρόγραμμα [source: 40]
    
                    // Αν η execv επιστρέψει, σημαίνει ότι απέτυχε!
                    perror("execv failed");
                    exit(1); // Σημαντικό να τερματίσει το παιδί αν αποτύχει η execv
                } else if (pid > 0) {
                    Worker worker = create_worker(pid, pipefd[0], cf);
                    hash_insert(active_workers, worker);
                    close(pipefd[1]);
                    queue_remove_front(cf);
                } else {
                    perror("fork");
                    return 1;
                }

                pid_t terminated_pid;
                int status;
                printf("Manager: Worker with PID %d finished.\n", terminated_pid);
    
                Worker worker = hash_search(active_workers, terminated_pid);
    
                close(get_read_pipe_fd(worker));
    
                hash_remove(active_workers, terminated_pid);
            } else {
                queue_insert_back(tasks, cf);
            }
        } else if (event->mask & IN_MODIFY) {
            Vector hash_dir = get_vector(directories);

            ConfigFile cf;
            int end = 0;
            for (Vector node = vector_first(hash_dir); node != VECTOR_EOF; node = vector_next(hash_dir, node)) {
                List l = vector_node_value(hash_dir, node);
                for (ListNode l_node = list_first(l); l_node = LIST_EOF; l_node = list_next(l, l_node)) {
                    cf = list_node_value(l, l_node);
                    if (get_watch(cf) == event->wd) {
                        end = 1;
                        break;
                    }
                }
                if (end == 1) {
                    break;
                }
            }

            char source_filepath[500]; // Χρησιμοποίησε PATH_MAX ή κατάλληλο μέγεθος
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", get_source(cf), event->name);

            char target_filepath[500]; // Χρησιμοποίησε PATH_MAX ή κατάλληλο μέγεθος
            snprintf(target_filepath, sizeof(target_filepath), "%s/%s", get_target(cf), event->name);
            
            if (get_size(active_workers) < worker_limit) {
                int pipefd[2]; // pipefd[0] is for reading, pipefd[1] is for writing
                if (pipe(pipefd) == -1) {
                    perror("pipe failed");
                    // Handle error appropriately (e.g., continue to next iteration? exit?)
                    continue;
                }

                pid_t pid = fork();
                if (pid == 0) {
                    close(pipefd[0]);
                    if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                        perror("dup2 failed");
                        exit(1); // Worker cannot report, critical error
                    }
                    close(pipefd[1]); // Close original write end after dup2
    
                    char *worker_argv[] = {"./worker", source_filepath, target_filepath, "MODIFIED", event->name, NULL};
                    execv("./worker", worker_argv); // Εκτελεί το worker πρόγραμμα [source: 40]
    
                    // Αν η execv επιστρέψει, σημαίνει ότι απέτυχε!
                    perror("execv failed");
                    exit(1); // Σημαντικό να τερματίσει το παιδί αν αποτύχει η execv
                } else if (pid > 0) {
                    Worker worker = create_worker(pid, pipefd[0], cf);
                    hash_insert(active_workers, worker);
                    close(pipefd[1]);
                    queue_remove_front(cf);
                } else {
                    perror("fork");
                    return 1;
                }

                pid_t terminated_pid;
                int status;
                printf("Manager: Worker with PID %d finished.\n", terminated_pid);
    
                Worker worker = hash_search(active_workers, terminated_pid);
    
                close(get_read_pipe_fd(worker));
    
                hash_remove(active_workers, terminated_pid);
            } else {
                queue_insert_back(tasks, cf);
            }
        } else if (event->mask & IN_DELETE) {
            Vector hash_dir = get_vector(directories);

            ConfigFile cf;
            int end = 0;
            for (Vector node = vector_first(hash_dir); node != VECTOR_EOF; node = vector_next(hash_dir, node)) {
                List l = vector_node_value(hash_dir, node);
                for (ListNode l_node = list_first(l); l_node = LIST_EOF; l_node = list_next(l, l_node)) {
                    cf = list_node_value(l, l_node);
                    if (get_watch(cf) == event->wd) {
                        end = 1;
                        break;
                    }
                }
                if (end == 1) {
                    break;
                }
            }

            char target_filepath[500]; // Χρησιμοποίησε PATH_MAX ή κατάλληλο μέγεθος
            snprintf(target_filepath, sizeof(target_filepath), "%s/%s", get_target(cf), event->name);
            
            if (get_size(active_workers) < worker_limit) {
                int pipefd[2]; // pipefd[0] is for reading, pipefd[1] is for writing
                if (pipe(pipefd) == -1) {
                    perror("pipe failed");
                    // Handle error appropriately (e.g., continue to next iteration? exit?)
                    continue;
                }

                pid_t pid = fork();
                if (pid == 0) {
                    close(pipefd[0]);
                    if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                        perror("dup2 failed");
                        exit(1); // Worker cannot report, critical error
                    }
                    close(pipefd[1]); // Close original write end after dup2
    
                    char *worker_argv[] = {"./worker", NULL, target_filepath, "DELETED", event->name, NULL};
                    execv("./worker", worker_argv); // Εκτελεί το worker πρόγραμμα [source: 40]
    
                    // Αν η execv επιστρέψει, σημαίνει ότι απέτυχε!
                    perror("execv failed");
                    exit(1); // Σημαντικό να τερματίσει το παιδί αν αποτύχει η execv
                } else if (pid > 0) {
                    Worker worker = create_worker(pid, pipefd[0], cf);
                    hash_insert(active_workers, worker);
                    close(pipefd[1]);
                    queue_remove_front(cf);
                } else {
                    perror("fork");
                    return 1;
                }

                pid_t terminated_pid;
                int status;
                printf("Manager: Worker with PID %d finished.\n", terminated_pid);
    
                Worker worker = hash_search(active_workers, terminated_pid);
    
                close(get_read_pipe_fd(worker));
    
                hash_remove(active_workers, terminated_pid);
            } else {
                queue_insert_back(tasks, cf);
            }
            
        } else {
            printf("ERROOOOOOR DA FCK\n");
        }
    
        // Προχώρα στο επόμενο event:
        i += sizeof(struct inotify_event) + event->len;
    }
}

void log_date() {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    printf("[%04d-%02d-%02d %02d:%02d:%02d]\n", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
}

int init_system(HashTable directories, HashTable active_workers, Queue tasks, char* fss_in, char* fss_out, char* configfile, char* managerlog, int worker_limit, int fd_inotify, int* fd_in, int* fd_out, int* fd_log) {
    int check, fd_config;

    if (access(fss_in, F_OK) == 0) {
        check = unlink(fss_in);
        if (check == -1) {
            perror("Error deleting fss_in\n");
        }

        if (mkfifo(fss_in, 0644) == -1) {
            perror("Error creating the fss_in\n");
        }

        *fd_in = open(fss_in, O_RDONLY | O_NONBLOCK);
        if (*fd_in == -1) {
            perror("Eror opening the fss_in\n");
        }
    } else {
        *fd_in = open(fss_in, O_RDONLY | O_NONBLOCK);
        if (*fd_in == -1) {
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

        *fd_out = open(fss_out, O_WRONLY | O_NONBLOCK);
        if (*fd_out == -1) {
            perror("Eror opening the fss_out\n");
        }
    } else {
        *fd_out = open(fss_out, O_WRONLY | O_NONBLOCK);
        if (*fd_out == -1) {
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
    tasks = queue_create(NULL);

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

    directories = hash_create(line_count + 10, NULL, false);
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
            ConfigFile cf = create_cf(0, "FULL", source_file, target_file, inotify_add_watch(fd_inotify, source_file, IN_CREATE | IN_MODIFY | IN_DELETE));
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
    

    active_workers = hash_create(worker_limit, NULL, true);

    synca(directories, active_workers, tasks, worker_limit, fd_log);
}

void event_loop(HashTable directories, HashTable active_workers, Queue tasks, int fd_in, int fd_out, int fd_inotify, int worker_limit, int fd_log) {
    fd_set master_fds; // Το κύριο σετ με όλους τους fds που παρακολουθούμε
    fd_set read_fds;   // Το προσωρινό σετ που δίνουμε στη select() (γιατί η select το τροποποιεί)
    int max_fd;

    FD_ZERO(&master_fds);

    // 2. Προσθήκη των fds που θέλουμε να παρακολουθούμε για διάβασμα
    FD_SET(fd_in, &master_fds);      // Εντολές από console
    FD_SET(fd_inotify, &master_fds); // Γεγονότα από inotify

    max_fd = (fd_in > fd_inotify) ? fd_in : fd_inotify;

    while (1) {
        read_fds = master_fds;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity > 0) { // Υπήρξε δραστηριότητα σε κάποιον/κάποιους fd
            // 6. Έλεγχος για εντολές από το console (fss_in)
            if (FD_ISSET(fd_in, &read_fds)) {
                printf("Manager: Activity detected on fss_in pipe.\n");
                // Κάλεσε τη συνάρτηση που διαβάζει και επεξεργάζεται την εντολή
                // Η συνάρτηση αυτή ίσως αλλάξει το running σε 0 αν έρθει "shutdown"
                handle_console_event(directories, active_workers, tasks, fd_in, fd_out, fd_inotify, worker_limit, fd_log);
            }

            // 7. Έλεγχος για γεγονότα από το inotify
            if (FD_ISSET(fd_inotify, &read_fds)) {
                printf("Manager: Activity detected on inotify fd.\n");
                // Κάλεσε τη συνάρτηση που διαβάζει και επεξεργάζεται τα inotify events
                // Αυτή η συνάρτηση πιθανόν θα βάλει νέα tasks στην ουρά ή θα καλέσει
                // τη λογική εκκίνησης worker.
                handle_inotify_event(directories, active_workers, tasks, fd_inotify, worker_limit);
            }
        }        
    }
}

int main(int argc, char* argv[]) {
    HashTable directories;
    HashTable active_workers;
    Queue tasks;

    char* manager_log = argv[2];
    char* configFile = argv[4];
    int worker_limit = argv[6];

    int fd_in;
    int fd_out;
    int fd_log;
    int fd_inotify = inotify_init();

    init_system(directories, active_workers, tasks, "fss_in", "fss_out", configFile, manager_log, worker_limit, &fd_inotify, &fd_in, &fd_out, &fd_log);

    event_loop(directories, active_workers, tasks, fd_in, fd_out, fd_inotify, worker_limit, fd_log);

    return 0;
}