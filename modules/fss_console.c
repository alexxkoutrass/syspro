#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


int main() {

}


int add(char *command, char *pipe_in, char *pipe_out) {
    int pipe_in = open(pipe_in, O_WRONLY);
    int pipe_out = open(pipe_out, O_RDONLY);

    write(pipe_in, command, strlen(command));

    char buffer[1024];
    read(pipe_out, buffer, sizeof(buffer));
    printf("Manager replied: %s\n", buffer);

    close(pipe_in);
    close(pipe_out);
    return 0;
}

int status(char* command, char* pipe_in, char* pipe_out) {
    int pipe_in = open(pipe_in, O_WRONLY);
    int pipe_out = open(pipe_out, O_RDONLY);

    write(pipe_in, command, strlen(command));

    char buffer[1024];
    read(pipe_out, buffer, sizeof(buffer));
    printf("Manager replied: %s\n", buffer);

    close(pipe_in);
    close(pipe_out);
    return 0;
}

int cancel(char* command, char* pipe_in, char* pipe_out) {
    int pipe_in = open(pipe_in, O_WRONLY);
    int pipe_out = open(pipe_out, O_RDONLY);

    write(pipe_in, command, strlen(command));

    char buffer[1024];
    read(pipe_out, buffer, sizeof(buffer));
    printf("Manager replied: %s\n", buffer);

    close(pipe_in);
    close(pipe_out);
    return 0;
}

int sync(char* command, char* pipe_in, char* pipe_out) {
    int pipe_in = open(pipe_in, O_WRONLY);
    int pipe_out = open(pipe_out, O_RDONLY);

    write(pipe_in, command, strlen(command));

    char buffer[1024];
    read(pipe_out, buffer, sizeof(buffer));
    printf("Manager replied: %s\n", buffer);

    close(pipe_in);
    close(pipe_out);
    return 0;
}

int shutdown(char* command, char* pipe_in, char* pipe_out) {
    int pipe_in = open(pipe_in, O_WRONLY);
    int pipe_out = open(pipe_out, O_RDONLY);

    write(pipe_in, command, strlen(command));

    char buffer[1024];
    read(pipe_out, buffer, sizeof(buffer));
    printf("Manager replied: %s\n", buffer);

    close(pipe_in);
    close(pipe_out);
    return 0;
}

