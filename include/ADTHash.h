#include "ADTVector.h"
#include "common_types.h"


typedef struct hash_table* HashTable;
typedef struct worker* Worker;
typedef struct configfile* ConfigFile;


ConfigFile create_cf(int id, char status[30], char source[100], char target[100], int watch);

Worker create_worker(pid_t PID, int read_pipe_fd, ConfigFile task);

Vector get_vector(HashTable hash);

char* get_source(ConfigFile cf);

char* get_target(ConfigFile cf);

int get_read_pipe_fd(Worker worker);

char* get_status(ConfigFile cf);

int get_watch(ConfigFile cf);

void set_watch(ConfigFile cf, int watch);

int get_size(HashTable hash);

int hash_function(int id, int size);

HashTable hash_create(int size, DestroyFunc destroy_value, bool Workers_Configfile);

void hash_insert(HashTable hash, Pointer value);

Pointer hash_search(HashTable hash, Pointer id);

void hash_remove(HashTable hash, Pointer id);