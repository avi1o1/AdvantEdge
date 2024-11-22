#include "fileSystem.h"
#include "cluster.h"

int inodeNumber = 0;
HashMap fileToInode;

// Old LRU Cache Implementation
// typedef struct {
//     char *userPath;
//     Inode *inode;
//     LRUCache *next;
// } LRUCache;

// LRUCache *head = NULL;

pthread_mutex_t fileMappings = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inodeNumberMutex = PTHREAD_MUTEX_INITIALIZER;

int chooseCluster(int inumber)
{
    if (getActiveClusterCount() == 0)
        return -1;
    return inumber % getActiveClusterCount();
}

int getNextInodeNumber()
{
    pthread_mutex_lock(&inodeNumberMutex);
    int nextInode = inodeNumber++;
    pthread_mutex_unlock(&inodeNumberMutex);
    return nextInode;
}

void initFileMappings()
{
    pthread_mutex_lock(&fileMappings);
    fileToInode = create_hash_map(FILE_MAPPING_SIZE);
    pthread_mutex_unlock(&fileMappings);
}

int addFile(char *userPath, int isDir)
{
    Inode *newInode = (Inode *)malloc(sizeof(Inode));
    newInode->inodeNumber = getNextInodeNumber();
    newInode->userPath = strdup(userPath);
    newInode->isDir = isDir;
    if (isDir)
    {
        newInode->children = create_linked_list();
        newInode->permission = 0700;
    }
    else
    {
        newInode->children = NULL;
        newInode->permission = 0644;
    }

    if (!strcmp(userPath, "/Kalimba.mp3"))
        newInode->size = 8808038;
    else
        newInode->size = 0;
    newInode->creationTime = time(NULL);
    newInode->lastModificationTime = time(NULL);
    newInode->lastAccessTime = time(NULL);

    newInode->readersCount = 0;
    newInode->writersCount = 0;
    newInode->toBeDeleted = false;
    pthread_mutex_init(&newInode->available, NULL);

    newInode->clusterID = chooseCluster(newInode->inodeNumber);
    if (newInode->clusterID == -1)
    {
        free(newInode);
        return -5;
    }

    pthread_mutex_lock(&fileMappings);
    if (get_hash_map(userPath, fileToInode) != NULL)
    {
        pthread_mutex_unlock(&fileMappings);
        free(newInode);
        return -4;
    }

    set_hash_map(userPath, newInode, fileToInode);
    pthread_mutex_unlock(&fileMappings);

    // Add the new file to the parent directory
    char *parentPath = strdup(userPath);
    char *lastSlash = strrchr(parentPath, '/');
    if (lastSlash != NULL && lastSlash != parentPath)
    {
        *lastSlash = '\0';

        Inode *parentInode = (Inode *)get_hash_map(parentPath, fileToInode);
        pthread_mutex_lock(&parentInode->available);
        if (parentInode != NULL && parentInode->isDir && !parentInode->toBeDeleted)
            append_linked_list(newInode, parentInode->children);
        pthread_mutex_unlock(&parentInode->available); // Changed from lock to unlock
    }
    free(parentPath);

    if (strcmp(userPath, "/Kalimba.mp3") && createFileSS(newInode))
    {
        delete_hash_map(userPath, fileToInode);
        // remove from parent directory
        if (lastSlash != NULL && lastSlash != parentPath)
        {
            Inode *parentInode = (Inode *)get_hash_map(parentPath, fileToInode);
            pthread_mutex_lock(&parentInode->available);
            if (parentInode != NULL && parentInode->isDir && !parentInode->toBeDeleted)
                delete_by_data_linked_list(newInode, parentInode->children);
            pthread_mutex_unlock(&parentInode->available);
        }
        free(newInode);
        return -1;
    }

    // Add to end of LRU cache if size of cache is less than 10
    // if(head == NULL)
    // {
    //     head = (LRUCache *)malloc(sizeof(LRUCache));
    //     head->userPath = strdup(userPath);
    //     head->inode = newInode;
    //     head->next = NULL;
    // }
    // else
    // {
    //     LRUCache *current = head;
    //     int length = 1;
    //     while(current->next != NULL)
    //     {
    //         current = current->next;
    //         length++;
    //         if(length == 10)
    //             break;
    //     }
    //     if(length == 10)
    //     {
    //         free(current->next->userPath);
    //         free(current->next);
    //         current->next = NULL;
    //     }
    //     current->next = (LRUCache *)malloc(sizeof(LRUCache));
    //     current->next->userPath = strdup(userPath);
    //     current->next->inode = newInode;
    //     current->next->next = NULL;
    // }

    return 0;
}

int deleteFileHelper(Inode *inode)
{
    if (inode == NULL)
        return -1;

    // If it's a directory, delete all children
    inode->toBeDeleted = true; // TODO: Make sure this is handled properly in request handling

    if (inode->isDir)
    {
        pthread_mutex_lock(&inode->available);
        LinkedListNode *current = inode->children->head;
        while (current != NULL)
        {
            Inode *child = (Inode *)current->data;
            deleteFileHelper(child);
            current = current->next;
        }
        pthread_mutex_unlock(&inode->available);
    }

    deleteFileSS(inode);

    // Remove the inode from the hash map
    delete_hash_map(inode->userPath, fileToInode);

    // Free the inode memory
    free(inode->userPath);
    free(inode);
    return 0;
}

int deleteFile(char *userPath)
{
    // Remove the file from LRU cache
    // if(head != NULL)
    // {
    //     LRUCache *current = head;
    //     LRUCache *prev = NULL;
    //     while(current != NULL)
    //     {
    //         if(strcmp(current->userPath, userPath) == 0)
    //         {
    //             deleteFileHelper(current->inode);
    //             if(prev != NULL)
    //             {
    //                 prev->next = current->next;
    //                 free(current->userPath);
    //                 free(current);
    //             }
    //             else
    //             {
    //                 head = current->next;
    //                 free(current->userPath);
    //                 free(current);
    //             }
    //             break;
    //         }
    //         prev = current;
    //         current = current->next;
    //     }
    // }

    Inode *inode = (Inode *)get_hash_map(userPath, fileToInode);
    if (inode == NULL)
        return -3;

    // Remove the file from the parent directory
    char *parentPath = strdup(inode->userPath);
    char *lastSlash = strrchr(parentPath, '/');
    if (lastSlash != NULL && lastSlash != parentPath)
    {
        *lastSlash = '\0';

        Inode *parentInode = (Inode *)get_hash_map(parentPath, fileToInode);
        if (parentInode == NULL)
            return -3;
        pthread_mutex_lock(&parentInode->available);
        if (parentInode->isDir)
            delete_by_data_linked_list(inode, parentInode->children);
        pthread_mutex_unlock(&parentInode->available);
    }
    free(parentPath);

    int ret_val = deleteFileHelper(get_hash_map(userPath, fileToInode));
    return ret_val;
}

Inode *getInode(char *userPath)
{
    // LRU Cache Search
    // if(head != NULL)
    // {
    //     LRUCache *current = head;
    //     LRUCache *prev = NULL;
    //     while(current != NULL)
    //     {
    //         if(strcmp(current->userPath, userPath) == 0)
    //         {
    //             if(prev != NULL)
    //             {
    //                 prev->next = current->next;
    //                 current->next = head;
    //                 head = current;
    //             }
    //             return current->inode;
    //         }
    //         prev = current;
    //         current = current->next;
    //     }
    // }
    Inode *inode = get_hash_map(userPath, fileToInode);
    if (inode && !inode->toBeDeleted)
        return inode;
    return NULL;
}

int renameFile(char *path, char *newName)
{
    // int found = 0;

    // // LRU Cache Search
    // if(head != NULL)
    // {
    //     LRUCache *current = head;
    //     LRUCache *prev = NULL;
    //     while(current != NULL)
    //     {
    //         if(strcmp(current->userPath, path) == 0)
    //         {
    //             found = 1;
    //             if(prev != NULL)
    //             {
    //                 prev->next = current->next;
    //                 current->next = head;
    //                 head = current;
    //             }
    //             break;
    //         }
    //         prev = current;
    //         current = current->next;
    //     }
    // }

    Inode *inode = get_hash_map(path, fileToInode);
    if (inode == NULL)
        return -3;

    char *parentPath = strdup(path);
    char *lastSlash = strrchr(parentPath, '/');
    if (lastSlash == NULL)
    {
        free(parentPath);
        return -1;
    }

    *lastSlash = '\0';
    char *newPath = (char *)malloc(strlen(parentPath) + strlen(newName) + 2);
    sprintf(newPath, "%s/%s", parentPath, newName);
    free(parentPath);

    pthread_mutex_lock(&inode->available);
    if (get_hash_map(newPath, fileToInode) != NULL)
    {
        pthread_mutex_unlock(&inode->available);
        free(newPath);
        return -4;
    }

    // Update the user path
    free(inode->userPath);
    inode->userPath = newPath;

    // Update the file mappings
    delete_hash_map(path, fileToInode);
    set_hash_map(newPath, inode, fileToInode);

    inode->lastModificationTime = time(NULL);
    inode->lastAccessTime = time(NULL);

    pthread_mutex_unlock(&inode->available);

    if (renameFileSS(path, inode))
    {
        pthread_mutex_lock(&inode->available);
        free(inode->userPath);
        inode->userPath = strdup(path);
        pthread_mutex_unlock(&inode->available);
        return -1;
    }

    return 0;
}

int justAddFileToMappings(char *userPath, int isDir, int clusterID)
{
    Inode *newInode = (Inode *)malloc(sizeof(Inode));
    newInode->inodeNumber = getNextInodeNumber();
    newInode->userPath = strdup(userPath);
    newInode->isDir = isDir;
    if (isDir)
    {
        newInode->children = create_linked_list();
        newInode->permission = 0700;
    }
    else
    {
        newInode->children = NULL;
        newInode->permission = 0644;
    }

    if (!strcmp(userPath, "/Kalimba.mp3"))
        newInode->size = 8808038;
    else
        newInode->size = 0;
    newInode->creationTime = time(NULL);
    newInode->lastModificationTime = time(NULL);
    newInode->lastAccessTime = time(NULL);

    newInode->readersCount = 0;
    newInode->writersCount = 0;
    newInode->toBeDeleted = false;
    pthread_mutex_init(&newInode->available, NULL);

    newInode->clusterID = clusterID;
    if (newInode->clusterID == -1)
    {
        free(newInode);
        return -5;
    }

    pthread_mutex_lock(&fileMappings);
    if (get_hash_map(userPath, fileToInode) != NULL)
    {
        pthread_mutex_unlock(&fileMappings);
        free(newInode);
        return -4;
    }

    set_hash_map(userPath, newInode, fileToInode);
    pthread_mutex_unlock(&fileMappings);

    // Add the new file to the parent directory
    char *parentPath = strdup(userPath);
    char *lastSlash = strrchr(parentPath, '/');
    if (lastSlash != NULL && lastSlash != parentPath)
    {
        *lastSlash = '\0';

        Inode *parentInode = (Inode *)get_hash_map(parentPath, fileToInode);
        pthread_mutex_lock(&parentInode->available);
        if (parentInode != NULL && parentInode->isDir && !parentInode->toBeDeleted)
            append_linked_list(newInode, parentInode->children);
        pthread_mutex_unlock(&parentInode->available); // Changed from lock to unlock
    }
    free(parentPath);

    return 0;
}
