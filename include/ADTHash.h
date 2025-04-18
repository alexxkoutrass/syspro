#include "common_types.h"


typedef struct hash_table* HashTable;
typedef struct worker* Worker;
typedef struct configfile* ConfigFile;



int hash_function(int id, int size);

HashTable hash_create(int size, DestroyFunc destroy_value, bool Workers_Configfile);

void hash_insert(HashTable hash, Pointer value);

Pointer hash_search(HashTable hash, int id);

void hash_remove(HashTable hash, int id);