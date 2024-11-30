#include "../include/linkedList.h"

LinkedList create_linked_list()
{
    LinkedList linkedList = (LinkedList)malloc(sizeof(LinkedListStruct));
    linkedList->head = NULL;
    linkedList->size = 0;
    pthread_mutex_init(&linkedList->lock, NULL);
    return linkedList;
}

void *get_linked_list(int pos, LinkedList linkedList)
{
    pthread_mutex_lock(&linkedList->lock);
    int s = 0;
    LinkedListNode *list = linkedList->head;
    while (list != NULL)
    {
        if (s == pos)
        {
            pthread_mutex_unlock(&linkedList->lock);
            return (list->data);
        }
        s++;
        list = list->next;
    }
    pthread_mutex_unlock(&linkedList->lock);
    return NULL;
}

void append_linked_list(void *data, LinkedList linkedList)
{
    if (!linkedList)
        return;

    pthread_mutex_lock(&linkedList->lock);
    LinkedListNode *D = (LinkedListNode *)malloc(sizeof(LinkedListNode));
    if (!D)
    {
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }

    D->data = (void *)malloc(strlen((char *)data) + 1);
    strcpy((char *)D->data, (char *)data);
    D->next = NULL;

    linkedList->size++;

    if (linkedList->head == NULL)
    {
        linkedList->head = D;
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }

    LinkedListNode *l = linkedList->head;
    while (l->next != NULL)
        l = l->next;
    l->next = D;
    pthread_mutex_unlock(&linkedList->lock);
}

void insert_linked_list(void *data, int pos, LinkedList linkedList)
{
    if (!linkedList || pos < 0)
        return;

    pthread_mutex_lock(&linkedList->lock);
    if (pos > linkedList->size)
    {
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }

    LinkedListNode *D = (LinkedListNode *)malloc(sizeof(LinkedListNode));
    if (!D)
    {
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }
    D->data = (void *)malloc(strlen((char *)data) + 1);
    strcpy((char *)D->data, (char *)data);

    if (pos == 0)
    {
        D->next = linkedList->head;
        linkedList->head = D;
        linkedList->size++;
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }

    LinkedListNode *l = linkedList->head;
    for (int i = 0; i < pos - 1 && l; i++)
        l = l->next;

    if (!l)
    {
        free(D);
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }

    D->next = l->next;
    l->next = D;
    linkedList->size++;
    pthread_mutex_unlock(&linkedList->lock);
}

void delete_linked_list(int pos, LinkedList linkedList)
{
    if (!linkedList || pos < 0 || pos >= linkedList->size)
        return;

    pthread_mutex_lock(&linkedList->lock);
    LinkedListNode *l = linkedList->head;

    if (pos == 0)
    {
        if (!l)
        {
            pthread_mutex_unlock(&linkedList->lock);
            return;
        }
        linkedList->head = l->next;
        linkedList->size--;
        free(l);
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }

    for (int i = 0; i < pos - 1 && l; i++)
        l = l->next;

    if (!l || !l->next)
    {
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }

    LinkedListNode *D = l->next;
    l->next = D->next;
    linkedList->size--;
    free(D);
    pthread_mutex_unlock(&linkedList->lock);
}

void delete_by_data_linked_list(void *data, LinkedList linkedList)
{
    pthread_mutex_lock(&linkedList->lock);
    LinkedListNode *l = linkedList->head;

    if (l->data == data)
    {
        linkedList->head = l->next;
        linkedList->size--;
        free(l);
        pthread_mutex_unlock(&linkedList->lock);
        return;
    }

    while (l->next != NULL)
    {
        if (l->next->data == data)
        {
            LinkedListNode *D = l->next;
            l->next = D->next;
            linkedList->size--;
            free(D);
            pthread_mutex_unlock(&linkedList->lock);
            return;
        }
        l = l->next;
    }
    pthread_mutex_unlock(&linkedList->lock);
}

void destroy_linked_list(LinkedList linkedList)
{
    pthread_mutex_lock(&linkedList->lock);
    LinkedListNode *l = linkedList->head;
    while (l != NULL)
    {
        LinkedListNode *D = l;
        l = l->next;
        free(D);
    }
    pthread_mutex_unlock(&linkedList->lock);
    pthread_mutex_destroy(&linkedList->lock);
    free(linkedList);
}

void print_linked_list(LinkedList linkedList)
{
    pthread_mutex_lock(&linkedList->lock);
    LinkedListNode *node = linkedList->head;
    int index = 0;
    while (node != NULL)
    {
        printf("Index %d: Value: %s\n", index, (char *)node->data);
        node = node->next;
        index++;
    }
    pthread_mutex_unlock(&linkedList->lock);
}

void copy_linked_list(LinkedList source, LinkedList destination)
{
    pthread_mutex_lock(&source->lock);
    LinkedListNode *node = source->head;
    while (node != NULL)
    {
        append_linked_list(node->data, destination);
        node = node->next;
    }
    pthread_mutex_unlock(&source->lock);
}

// for testing
// void *thread_func(void *arg)
// {
//     LinkedList linkedList = (LinkedList)arg;

//     // Append some data
//     append_linked_list("value1", linkedList);
//     append_linked_list("value2", linkedList);
//     append_linked_list("value3", linkedList);
//     append_linked_list("value4", linkedList);
//     append_linked_list("value5", linkedList);

//     // Get and print the values
//     printf("Thread %ld - pos 0: %s\n", pthread_self(), (char *)get_linked_list(0, linkedList));
//     printf("Thread %ld - pos 1: %s\n", pthread_self(), (char *)get_linked_list(1, linkedList));
//     printf("Thread %ld - pos 2: %s\n", pthread_self(), (char *)get_linked_list(2, linkedList));

//     // Delete a value and try to get its value
//     delete_linked_list(1, linkedList);
//     printf("Thread %ld - pos 1 after deletion: %s\n", pthread_self(), (char *)get_linked_list(1, linkedList));

//     // Insert a new value at position 1
//     insert_linked_list("new_value2", 1, linkedList);
//     printf("Thread %ld - pos 1 after insert: %s\n", pthread_self(), (char *)get_linked_list(1, linkedList));

//     return NULL;
// }

// int main()
// {
//     // Create a linked list
//     LinkedList linkedList = create_linked_list();

//     // Create threads
//     pthread_t threads[5];
//     for (int i = 0; i < 5; i++)
//     {
//         pthread_create(&threads[i], NULL, thread_func, (void *)linkedList);
//     }

//     // Wait for threads to finish
//     for (int i = 0; i < 5; i++)
//     {
//         pthread_join(threads[i], NULL);
//     }

//     // Print the entire linked list
//     printf("Final linked list:\n");
//     print_linked_list(linkedList);

//     // Destroy the linked list
//     destroy_linked_list(linkedList);

//     return 0;
// }
