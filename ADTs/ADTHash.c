#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ADTHash.h"
#include "ADTList.h"
#include "ADTVector.h"
#include "fss_manager.c"

#define MAX 3


struct configfile {
    int id;
    char* source;
    char* target;
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
        return ((ConfigFile)value)->id;
    }
}

int hash_function(int id, int size) {
    return id % size;
}

Worker create_worker(pid_t PID, int read_pipe_fd, ConfigFile task) {
    Worker worker = malloc(sizeof(*worker));

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
}


Pointer hash_search(HashTable hash, int id) {
    List l = vector_get_at(hash->directory, hash_function(id, hash->size));

    for (ListNode node = list_first(l); node != list_last(l); node = list_next(l, node)) {
        Pointer value = list_node_value(l, node);
        int key = find_key(value, hash->Workers_Configfile);
        if (key == id) {
            return value;
        }
    }

    return NULL;
}

void hash_remove(HashTable hash, int id) {
    List l = vector_get_at(hash->directory, hash_function(id, hash->size));
    
    ListNode node = list_first(l); 
    ListNode prev_node = LIST_BOF;
    while (node != LIST_EOF) {
        Pointer value = list_node_value(l, node);
        int key = find_key(value, hash->Workers_Configfile);
        if (key == id) {
            list_remove_next(l, prev_node);
            break;
        }
        prev_node = node;
        node = list_next(l, node);
    }
}
