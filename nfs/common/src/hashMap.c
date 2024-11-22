#include "../include/hashMap.h"

int hash(char *key, int capacity)
{
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++)
        hash += key[i];
    return hash % capacity;
}

HashMap create_hash_map(int capacity)
{
    HashMap hashMap = (HashMap)malloc(sizeof(HashMapStruct));
    hashMap->size = 0;
    hashMap->capacity = capacity;
    hashMap->data = (HashNode **)calloc(capacity, sizeof(HashNode *));
    pthread_mutex_init(&hashMap->lock, NULL);
    return hashMap;
}

void *get_hash_map(char *key, HashMap hashMap)
{
    pthread_mutex_lock(&hashMap->lock);
    int index = hash(key, hashMap->capacity);
    HashNode *node = hashMap->data[index];
    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        {
            pthread_mutex_unlock(&hashMap->lock);
            return node->value;
        }
        node = node->next;
    }
    pthread_mutex_unlock(&hashMap->lock);
    return NULL;
}

void set_hash_map(char *key, void *value, HashMap hashMap)
{
    if (!key || !hashMap)
        return;

    pthread_mutex_lock(&hashMap->lock);
    int index = hash(key, hashMap->capacity);
    HashNode *node = hashMap->data[index];

    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        {
            node->value = value;
            pthread_mutex_unlock(&hashMap->lock);
            return;
        }
        node = node->next;
    }

    HashNode *newNode = (HashNode *)malloc(sizeof(HashNode));
    if (!newNode)
    {
        pthread_mutex_unlock(&hashMap->lock);
        return;
    }

    newNode->key = strdup(key);
    if (!newNode->key)
    {
        free(newNode);
        pthread_mutex_unlock(&hashMap->lock);
        return;
    }

    newNode->value = value;
    newNode->next = hashMap->data[index];
    hashMap->data[index] = newNode;
    hashMap->size++;
    pthread_mutex_unlock(&hashMap->lock);
}

void delete_hash_map(char *key, HashMap hashMap)
{
    pthread_mutex_lock(&hashMap->lock);
    int index = hash(key, hashMap->capacity);
    HashNode *node = hashMap->data[index];
    HashNode *prev = NULL;
    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        {
            if (prev == NULL)
                hashMap->data[index] = node->next;
            else
                prev->next = node->next;
            free(node->key);
            free(node);
            hashMap->size--;
            pthread_mutex_unlock(&hashMap->lock);
            return;
        }
        prev = node;
        node = node->next;
    }
    pthread_mutex_unlock(&hashMap->lock);
}

void destroy_hash_map(HashMap hashMap)
{
    if (!hashMap)
        return;

    pthread_mutex_lock(&hashMap->lock);
    for (int i = 0; i < hashMap->capacity; i++)
    {
        HashNode *node = hashMap->data[i];
        while (node != NULL)
        {
            HashNode *next = node->next;
            free(node->key); // Free the key
            free(node);
            node = next;
        }
    }
    free(hashMap->data);
    pthread_mutex_unlock(&hashMap->lock);
    pthread_mutex_destroy(&hashMap->lock);
    free(hashMap);
}

void print_hash_map(HashMap hashMap)
{
    pthread_mutex_lock(&hashMap->lock);
    for (int i = 0; i < hashMap->capacity; i++)
    {
        HashNode *node = hashMap->data[i];
        if (node != NULL)
        {
            printf("Index %d:\n", i);
            while (node != NULL)
            {
                printf("  Key: %s, Value: %s\n", node->key, (char *)node->value);
                node = node->next;
            }
        }
    }
    pthread_mutex_unlock(&hashMap->lock);
}

// for testing
// void *thread_func(void *arg)
// {
//     HashMap hashMap = (HashMap)arg;

//     // Set some key-value pairs
//     set_hash_map("key1", "value1", hashMap);
//     set_hash_map("key2", "value2", hashMap);
//     set_hash_map("key3", "value3", hashMap);
//     set_hash_map("key4", "value4", hashMap);
//     set_hash_map("key5", "value5", hashMap);

//     // Get and print the values
//     printf("Thread %ld - key1: %s\n", pthread_self(), (char *)get_hash_map("key1", hashMap));
//     printf("Thread %ld - key2: %s\n", pthread_self(), (char *)get_hash_map("key2", hashMap));
//     printf("Thread %ld - key3: %s\n", pthread_self(), (char *)get_hash_map("key3", hashMap));

//     // Delete a key and try to get its value
//     delete_hash_map("key2", hashMap);
//     printf("Thread %ld - key2 after deletion: %s\n", pthread_self(), (char *)get_hash_map("key2", hashMap));

//     // Set a new value for an existing key
//     set_hash_map("key1", "new_value1", hashMap);
//     printf("Thread %ld - key1 after update: %s\n", pthread_self(), (char *)get_hash_map("key1", hashMap));

//     return NULL;
// }

// int main()
// {
//     // Create a hash map with a capacity of 10
//     HashMap hashMap = create_hash_map(10);

//     // Create threads
//     pthread_t threads[5];
//     for (int i = 0; i < 5; i++)
//     {
//         pthread_create(&threads[i], NULL, thread_func, (void *)hashMap);
//     }

//     // Wait for threads to finish
//     for (int i = 0; i < 5; i++)
//     {
//         pthread_join(threads[i], NULL);
//     }

//     // Print the entire hash map
//     printf("Final hash map:\n");
//     print_hash_map(hashMap);

//     // Destroy the hash map
//     destroy_hash_map(hashMap);

//     return 0;
// }
