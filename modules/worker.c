#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>


char* log_date() {
    static char buffer[30];  // static: ζει και μετά το τέλος της συνάρτησης
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    snprintf(buffer, sizeof(buffer), "[%04d-%02d-%02d %02d:%02d:%02d]", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

    return buffer;
}


int main(int argc, char* argv[]) {
    char* source = argv[1];
    char* target = argv[2];
    char* filename = argv[3];
    char* operation = argv[4];
    char details[100] = "";
    char error[100] = "\0";
    char result[10] = "";
    int files_synced = 0;
    int files_skipped = 0;

    DIR *dir_stream = opendir(source);
    struct dirent *entry;

    if (strcmp(operation, "FULL") == 0) {
        struct dirent *entry;
        while ((entry = readdir(dir_stream)) != NULL) {
            // Έχουμε μια εγγραφή καταλόγου
            char *filename = entry->d_name;

            // Πολύ Σημαντικό: Αγνοούμε τις ειδικές εγγραφές "." και ".."
            if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
                continue; // Πήγαινε στην επόμενη εγγραφή
            }

            // Κατασκεύασε την πλήρη διαδρομή πηγής
            char source_filepath[500]; // Χρησιμοποίησε PATH_MAX ή κατάλληλο μέγεθος
            snprintf(source_filepath, sizeof(source_filepath), "%s/%s", source, filename);

            // Κατασκεύασε την πλήρη διαδρομή προορισμού
            char target_filepath[1000];
            snprintf(target_filepath, sizeof(target_filepath), "%s/%s", target, filename);

            // Τώρα, χρησιμοποίησε τις low-level I/O κλήσεις για να αντιγράψεις
            // το αρχείο από source_filepath στο target_filepath:
            int fd_source = open(source_filepath, O_RDONLY);
            if (fd_source == -1) {
                printf("Error opening file: %s\n", strerror(errno));
                files_skipped++;
                continue;;
            } else {
                files_synced++;
            }

            int fd_target = open(target_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_target == -1) { 
                printf("Error opening file: %s\n", strerror(errno));
                continue;;
            }

            ssize_t bytes_read;
            char buffer[1024];
            while ((bytes_read = read(fd_source, buffer, sizeof(buffer))) > 0) {
                if (write(fd_target, buffer, bytes_read) != bytes_read) {
                    printf("Error writing in the file: %s\n", strerror(errno));
                    return -1;
                }
            }
            close(fd_source);
            close(fd_target);
        }

        if (files_synced > 0 && files_skipped > 0) {
            strcat(result, "PARTIAL");
            snprintf(details, sizeof(details), "%d files copied, %d skipped", files_synced, files_skipped);
        } else if (files_synced > 0 && files_skipped == 0) {
            strcat(result, "SUCCESS");
            snprintf(details, sizeof(details), "%d files copied", files_synced);
        } else if (files_synced == 0 && files_skipped > 0) {
            strcat(result, "ERROR");
            snprintf(details, sizeof(details), "%d files skipped", files_skipped);
        }
        
        closedir(dir_stream);

    } else if (strcmp(operation, "ADDED") == 0 || strcmp(operation, "MODIFIED") == 0) {
        char source_filepath[500]; // Χρησιμοποίησε PATH_MAX ή κατάλληλο μέγεθος
        snprintf(source_filepath, sizeof(source_filepath), "%s/%s", source, filename);

        char target_filepath[1000];
        snprintf(target_filepath, sizeof(target_filepath), "%s/%s", target, filename);

        int fd_file = open(source_filepath, O_RDONLY);
        if (fd_file == -1) {
            strcat(error, strerror(errno));
            return -1;
        }

        int fd_target = open(target_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_target == -1) { 
            printf("Error opening file: %s\n", strerror(errno));
            return -1;
        }

        ssize_t bytes_read;
        char buffer[1024];
        while ((bytes_read = read(fd_file, buffer, sizeof(buffer))) > 0) {
            if (write(fd_target, buffer, bytes_read) != bytes_read) {
                printf("Error writing in the file: %s\n", strerror(errno));
                return -1;
             }
        }

        snprintf(details, sizeof(details), "File: %s", filename);

        if (error[0] != '\0') {
            size_t len = strlen(details);

            // Πρόσθεσε στο τέλος με offset
            snprintf(details + len, sizeof(details) - len, " - %s", error);
        }

        close(fd_file);
        close(fd_target);

    } else if (strcmp(operation, "DELETED") == 0) {

        char target_filepath[1000];
        snprintf(target_filepath, sizeof(target_filepath), "%s/%s", target, filename);

        int check = unlink(target_filepath);
        if (check == -1) {
            strcat(error, strerror(errno));
            return -1;
        }

        snprintf(details, sizeof(details), "File: %s", filename);

        if (error[0] != '\0') {
            size_t len = strlen(details);

            // Πρόσθεσε στο τέλος με offset
            snprintf(details + len, sizeof(details) - len, " - %s", error);
        }

    } else {
        printf("Erorrr\n");
    }

    log_date();
    printf("[%s] [%s] [%s] [%d] [%s] [%s] [%s]\n", log_date(), source, target, getpid(), operation, result, details);
    
    return 0;
}