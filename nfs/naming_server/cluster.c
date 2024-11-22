#include "cluster.h"
#include "commSS.h"
#include "fileSystem.h"

SuperClusterList superClusterList = NULL;
int lastStorageServerID = -1;
pthread_mutex_t superClusterListMutex = PTHREAD_MUTEX_INITIALIZER;

int activeClusterCount = 0;
pthread_mutex_t clusterCountMutex = PTHREAD_MUTEX_INITIALIZER;

int getActiveClusterCount()
{
    pthread_mutex_lock(&clusterCountMutex);
    int count = activeClusterCount;
    pthread_mutex_unlock(&clusterCountMutex);
    return count;
}

int getClusterID(int storageServerID)
{
    // Calculate cluster ID based on storage server ID ranges
    if (storageServerID >= 6)
        // For IDs 6 and above, divide by 3 to get cluster ID
        return storageServerID / 3;
    else if (storageServerID >= 2)
        // For IDs 2-5, use modulo 2 to get cluster ID
        return storageServerID % 2;
    else
        // For IDs 0-1, use ID directly as cluster ID
        return storageServerID;
}

void initSuperClusterList()
{
    pthread_mutex_lock(&superClusterListMutex);
    superClusterList = create_linked_list();
    pthread_mutex_unlock(&superClusterListMutex);
}

SuperCluster *createSuperCluster(int from)
{
    SuperCluster *superCluster = (SuperCluster *)malloc(sizeof(SuperCluster));
    superCluster->from = from;
    superCluster->to = from + SUPERCLUSTER_SIZE - 1;
    pthread_mutex_init(&superCluster->lock, NULL);
    for (int i = 0; i < SUPERCLUSTER_SIZE; i++)
        superCluster->clusters[i] = NULL;
    return superCluster;
}

int addStorageServer(char *ip, int port, int DMAport)
{
    StorageServer *storageServer = (StorageServer *)malloc(sizeof(StorageServer));
    storageServer->ip = strdup(ip);
    storageServer->port = port;
    storageServer->DMAport = DMAport;
    storageServer->isAlive = true;
    pthread_mutex_lock(&superClusterListMutex);
    storageServer->id = ++lastStorageServerID;
    pthread_mutex_unlock(&superClusterListMutex);

    int clusterID = getClusterID(storageServer->id);
    int superClusterID = clusterID / SUPERCLUSTER_SIZE;

    pthread_mutex_lock(&superClusterListMutex);
    if (superClusterList->size < superClusterID + 1)
        append_linked_list(createSuperCluster(superClusterID * SUPERCLUSTER_SIZE), superClusterList);
    pthread_mutex_unlock(&superClusterListMutex);

    SuperCluster *superCluster = get_linked_list(superClusterID, superClusterList);
    pthread_mutex_lock(&superCluster->lock);
    if (superCluster->clusters[clusterID % SUPERCLUSTER_SIZE] == NULL)
    {
        Cluster *cluster = (Cluster *)malloc(sizeof(Cluster));
        cluster->id = clusterID;
        cluster->readerCount = 0;
        cluster->isWriterActive = false;
        cluster->storageServers = create_linked_list();
        pthread_mutex_init(&cluster->lock, NULL);
        superCluster->clusters[clusterID % SUPERCLUSTER_SIZE] = cluster;

        pthread_mutex_lock(&clusterCountMutex);
        activeClusterCount++;
        pthread_mutex_unlock(&clusterCountMutex);
    }
    pthread_mutex_unlock(&superCluster->lock);

    pthread_mutex_lock(&superCluster->clusters[clusterID % SUPERCLUSTER_SIZE]->lock);
    append_linked_list(storageServer, superCluster->clusters[clusterID % SUPERCLUSTER_SIZE]->storageServers);
    pthread_mutex_unlock(&superCluster->clusters[clusterID % SUPERCLUSTER_SIZE]->lock);

    if (!clusterID)
        addFile("/Kalimba.mp3", false);

    return storageServer->id;
}

StorageServer *getStorageServerFromCluster(int index, int clusterID)
{
    int superClusterID = clusterID / SUPERCLUSTER_SIZE;
    SuperCluster *superCluster = get_linked_list(superClusterID, superClusterList);
    if (superCluster == NULL)
    {
        printf("SuperCluster not found\n");
        return NULL;
    }
    Cluster *cluster = superCluster->clusters[clusterID % SUPERCLUSTER_SIZE];
    if (cluster == NULL)
    {
        printf("Cluster not found\n");
        return NULL;
    }
    StorageServer *storageServer = NULL;
    // Go through the cluster and find the index-th storage server
    pthread_mutex_lock(&cluster->lock);
    LinkedListNode *l = cluster->storageServers->head;
    while (l != NULL)
    {
        StorageServer *server = (StorageServer *)l->data;
        if (index-- == 0)
        {
            storageServer = server;
            break;
        }
        l = l->next;
    }
    pthread_mutex_unlock(&cluster->lock);
    return storageServer;
}

StorageServer *getStorageServer(int storageServerID)
{
    int clusterID = getClusterID(storageServerID);
    // check until you get the correct storage server
    for (int i = 0; i < 3; i++)
    {
        StorageServer *storageServer = getStorageServerFromCluster(i, clusterID);
        if (storageServer == NULL)
            return NULL;
        if (storageServer->id == storageServerID)
            return storageServer;
    }
    return NULL;
}

StorageServer *getAliveStorageServerFromCluster(int clusterID)
{
    int superClusterID = clusterID / SUPERCLUSTER_SIZE;
    SuperCluster *superCluster = get_linked_list(superClusterID, superClusterList);
    Cluster *cluster = superCluster->clusters[clusterID % SUPERCLUSTER_SIZE];
    StorageServer *storageServer = NULL;
    // Go through the cluster and find the first alive storage server
    pthread_mutex_lock(&cluster->lock);
    LinkedListNode *l = cluster->storageServers->head;
    while (l != NULL)
    {
        StorageServer *server = (StorageServer *)l->data;
        if (server->isAlive)
        {
            storageServer = server;
            break;
        }
        l = l->next;
    }
    pthread_mutex_unlock(&cluster->lock);

    // send hello message to the storage server
    if (storageServer != NULL)
    {
        if (helloSS(storageServer) != 0)
        {
            storageServer->isAlive = false;
            return getAliveStorageServerFromCluster(clusterID);
        }
    }

    return storageServer;
}
