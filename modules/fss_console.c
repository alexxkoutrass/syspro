#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


int console_in(char* command, char* pipe_in) {
    int fss_in = open(pipe_in, O_WRONLY);
    if (fss_in == -1) {
        perror("Error opening the pipe_in for reading\n");
        return -1;
    }

    ssize_t a = write(fss_in, command, strlen(command));
    if (a == -1) {
        perror("Error sending the command to fss_manager\n");
    }
    
    if (close(fss_in) == -1) {
        perror("Error closing logfile");
    }
    return 0;
}

int out(char* pipe_out, char* logfile) {
    int fss_out = open(pipe_out, O_RDONLY);
    if (fss_out == -1) {
        perror("Error opening the pipe_in for writing\n");
        return -1;
    }

    char buffer[1024];
    ssize_t bytes_read;
    
    int fd = open(logfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening the log-file\n");
        close(fss_out);
        return -1;
    }

    bytes_read = read(fss_out, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        printf("Manager replied: %s\n", buffer);

        ssize_t bytes_wrote = write(fd, buffer, bytes_read);
        if (bytes_wrote == -1) {
            perror("Error writing in log-file\n");
        } else if (bytes_wrote != bytes_read) {
            fprintf(stderr, "Warning: Partially wrote to log-file...\n");
        }

    } else {
        perror("Error reading from pipe_out\n");
        return -1;
    }
    
    if (close(fd) == -1) {
        perror("Error closing logfile\n");
        return -1;
    }
    if (close(fss_out) == -1) {
        perror("Error closing pipe_out\n");
        return -1;
    }
    
    return 0;
}

