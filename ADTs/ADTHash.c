#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "ADTHash.h"
#include "ADTList.h"
#include "ADTVector.h"

#define MAX 3


struct configfile {
    int id;
    char status[30];
    char source[100];
    char target[100];
    int watch;
};

struct worker {
    pid_t PID;
    int read_pipe_fd;
    ConfigFile task;
};

struct hash_table {
    int size;
    Vector directory;
    bool Workers_Configfile;
};

int find_key(Pointer value, bool Workers_Configfile) {
    if (Workers_Configfile == true) {
        return ((Worker)value)->PID;
    } else {
        int sum = 0;
        for (int i = 0; ((ConfigFile)value)->source[i] != '\0'; i++) {
            sum += ((ConfigFile)value)->source[i]; // Add the ASCII value of the character to sum
        }
        return sum;
    }
}

int hash_function(int id, int size) {
    return id % size;
}

ConfigFile create_cf(int id, char status[30], char source[100], char target[100], int watch) {
    ConfigFile cf = malloc(sizeof(struct configfile));

    cf->id = id;
    strncpy(cf->source, source, sizeof(cf->source) - 1); // Ensure null-termination
    cf->source[sizeof(cf->source) - 1] = '\0'; 
    strncpy(cf->status, status, sizeof(cf->status) - 1);
    cf->status[sizeof(cf->status) - 1] = '\0';
    strncpy(cf->target, target, sizeof(cf->target) - 1);
    cf->target[sizeof(cf->target) - 1] = '\0';
    cf->watch = watch;

    return cf;
}

Vector get_vector(HashTable hash) {
    return hash->directory;
}

char* get_source(ConfigFile cf) {
    return cf->source;
}

char* get_target(ConfigFile cf) {
    return cf->target;
}

int get_read_pipe_fd(Worker worker) {
    return worker->read_pipe_fd;
}

char* get_status(ConfigFile cf) {
    return cf->status;
}

int get_watch(ConfigFile cf) {
    return cf->watch;
}

void set_watch(ConfigFile cf, int watch) {
    cf->watch = watch;
}

int get_size(HashTable hash) {
    return hash->size;
}

Worker create_worker(pid_t PID, int read_pipe_fd, ConfigFile task) {
    Worker worker = malloc(sizeof(struct worker));

    worker->PID = PID;
    worker->read_pipe_fd = read_pipe_fd;
    worker->task = task;

    return worker;
}

HashTable hash_create(int size, DestroyFunc destroy_value, bool Workers_Configfile) {
    HashTable hash = malloc(sizeof(*hash));
    if (hash == NULL) {
        fprintf(stderr, "Memory allocation failed for hash table\n");
        exit(EXIT_FAILURE);
    }

    hash->directory = vector_create(size, NULL);
    if (hash->directory == NULL) {
        fprintf(stderr, "Memory allocation failed for directory\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++) {
        List l = list_create(NULL);
        vector_set_at(hash->directory, i, l);
    }

    hash->Workers_Configfile = Workers_Configfile;
    hash->size = size;
    return hash;
}

void hash_insert(HashTable hash, Pointer value) {
    int key = find_key(value, hash->Workers_Configfile);
    int hash_key = hash_function(key, hash->size);

    List l = vector_get_at(hash->directory, hash_key);
    list_insert_next(l, list_last(l), value);
    hash->size++;
}

Pointer hash_search(HashTable hash, Pointer id) {
    int key = find_key(id, hash->Workers_Configfile);
    List l = vector_get_at(hash->directory, hash_function(key, hash->size));

    for (ListNode node = list_first(l); node != LIST_EOF; node = list_next(l, node)) {
        Pointer value = list_node_value(l, node);
        if (hash->Workers_Configfile == true) {
            if (((Worker)value)->PID == id) {
                return value;
            }
        } else {
            if (strcmp(((ConfigFile)value)->source, (char*)id) == 0) {
                return value;
            }
        }
    }

    return NULL;
}

void hash_remove(HashTable hash, Pointer id) {
    int key = find_key(id, hash->Workers_Configfile);
    List l = vector_get_at(hash->directory, hash_function(key, hash->size));
    
    ListNode node = list_first(l); 
    ListNode prev_node = LIST_BOF;
    while (node != LIST_EOF) {
        Pointer value = list_node_value(l, node);
        if (hash->Workers_Configfile == true) {
            if (((Worker)value)->PID == id) {
                list_remove_next(l, value);
                break;
            }
        } else {
            if (strcmp(((ConfigFile)value)->source, (char*)id) == 0) {
                list_remove_next(l, value);
                break;
            }
        }
        prev_node = node;
        node = list_next(l, node);
    }
    
    hash->size--;
}
