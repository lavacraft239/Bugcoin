#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAX_BUGCOINS 10000000
#define BLOCK_DIR "/data/data/com.termux/files/home/storage/shared/Bugcoin/blocks/"
#define POOL_PORT 3333
#define MAX_PEERS 10

volatile int running = 1;  // Para detener listener
pthread_mutex_t lock;
int totalBugcoins = 0;

typedef struct {
    char ip[16];
    int port;
} Peer;

typedef struct {
    int index;
    int prevHash;
    int nonce;
    char data[256];
    int hash;
    int reward;
} Block;

Peer peers[MAX_PEERS];
int peerCount = 0;

unsigned int simpleHash(char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // djb2
        hash ^= (hash >> 3);
        hash ^= (hash << 7);
    }
    return hash % 100000;
}

unsigned int hardHash(char *str) {
    unsigned int hash = 2166136261; // FNV offset
    int c;
    while ((c = *str++)) {
        hash ^= c;
        hash *= 16777619;          // FNV prime
        hash += (hash >> 13);      // mezcla bits
        hash ^= (hash << 7);       // más mezcla
        hash += (hash >> 17);
    }
    return hash % 100000000;       // dificultad
}

int mineBlock(char *data, int prevHash) {
    int nonce = 0;
    int hash;
    char temp[512];
    do {
        sprintf(temp, "%d%d%s", prevHash, nonce, data);
        hash = hardHash(temp);
        nonce++;
    } while (hash % 100000 != 0); // podés aumentar el 1000 a 10000 para más dificultad
    return nonce - 1;
}

void saveBlockLibrary(Block b) {
    char path[256];
    sprintf(path, "%sblock_%d.bug", BLOCK_DIR, b.index);
    FILE *f = fopen(path, "w");
    if (!f) {
        printf("Error opening %s\n", path);
        return;
    }
    fprintf(f, "%d|%d|%d|%s|%d|%d\n", b.index, b.prevHash, b.nonce, b.data, b.hash, b.reward);
    fclose(f);
}

// Añadir peer
void addPeer(const char* ip, int port) {
    if (peerCount >= MAX_PEERS) return;
    strncpy(peers[peerCount].ip, ip, 15);
    peers[peerCount].port = port;
    peerCount++;
}

// Listener de peers
void *peerListener(void *arg) {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) return NULL;

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(POOL_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSock);
        return NULL;
    }

    listen(serverSock, MAX_PEERS);
    printf("Listening for peer connections on port %d...\n", POOL_PORT);

    while (running) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSock < 0) continue;

        char buffer[512];
        int bytes = recv(clientSock, buffer, sizeof(buffer)-1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            int index, prevHash, nonce, hash, reward;
            char data[256];
            sscanf(buffer, "%d|%d|%d|%255[^|]|%d|%d", &index, &prevHash, &nonce, data, &hash, &reward);

            Block b;
            b.index = index; b.prevHash = prevHash; b.nonce = nonce;
            strcpy(b.data, data); b.hash = hash; b.reward = reward;

            pthread_mutex_lock(&lock);
            if (totalBugcoins + b.reward <= MAX_BUGCOINS) totalBugcoins += b.reward;
            pthread_mutex_unlock(&lock);

            saveBlockLibrary(b);
            printf("Received block %d from peer\n", b.index);
        }
        close(clientSock);
    }
    close(serverSock);
    return NULL;
}

// Broadcast de bloque a peers
void broadcastBlock(Block b) {
    for (int i = 0; i < peerCount; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(peers[i].port);
        inet_pton(AF_INET, peers[i].ip, &addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            char buffer[512];
            sprintf(buffer, "%d|%d|%d|%s|%d|%d\n", b.index, b.prevHash, b.nonce, b.data, b.hash, b.reward);
            send(sock, buffer, strlen(buffer), 0);
        }
        close(sock);
    }
}

int getLastHash() {
    FILE *f = fopen(".bugcoin", "r");
    if (!f) return 0;
    char line[512];
    char *lastLine = NULL;
    while (fgets(line, sizeof(line), f)) {
        free(lastLine);
        lastLine = strdup(line);
    }
    fclose(f);
    if (!lastLine) return 0;
    int index, prevHash, nonce, hash, reward;
    char data[256];
    sscanf(lastLine, "%d|%d|%d|%255[^|]|%d|%d", &index, &prevHash, &nonce, data, &hash, &reward);
    free(lastLine);
    totalBugcoins += reward;
    return hash;
}

Block createBlock(int index, int prevHash, char *data) {
    Block b;
    b.index = index;
    b.prevHash = prevHash;
    strcpy(b.data, data);
    b.nonce = mineBlock(data, prevHash);

    char temp[512];
    sprintf(temp, "%d%d%s", prevHash, b.nonce, data);
    b.hash = simpleHash(temp);

    pthread_mutex_lock(&lock);
    if (totalBugcoins >= MAX_BUGCOINS) b.reward = 0;
    else {
    b.reward = 10;
    if (totalBugcoins + b.reward > MAX_BUGCOINS)
        b.reward = MAX_BUGCOINS - totalBugcoins;
    totalBugcoins += b.reward;
}
pthread_mutex_unlock(&lock);

    saveBlockLibrary(b);
    broadcastBlock(b);
    return b;
}

typedef struct {
    int startIndex;
    int lastHash;
    int duration;
} ThreadData;

void *mineThread(void *arg) {
    ThreadData *td = (ThreadData *)arg;
    time_t start = time(NULL);
    int index = td->startIndex;
    int lastHash = td->lastHash;
    while (time(NULL) - start < td->duration) {
        char data[256];
        sprintf(data, "Block mined at %ld by thread %lu", time(NULL), pthread_self());
        Block b = createBlock(index, lastHash, data);
        lastHash = b.hash;
        index++;
        printf("Blocks mined: %d, Total Bugcoins: %d\n", index, totalBugcoins);

        if (b.reward == 0) break;  // Detiene el minado si no quedan Bugcoins
        if (totalBugcoins >= MAX_BUGCOINS) break;
    }
    free(td);
    return NULL;
}

int main(int argc, char *argv[]) {
    int nodeOn = 0, mining = 0, duration = 0;
    int threads = 2;

    pthread_mutex_init(&lock, NULL);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--true") == 0) nodeOn = 1;
        if (strcmp(argv[i], "--false") == 0) nodeOn = 0;
        if (strcmp(argv[i], "--mine") == 0 && i + 1 < argc) {
            mining = (strcmp(argv[i + 1], "True") == 0);
            i++;
        }
        if (strcmp(argv[i], "--d") == 0 && i + 1 < argc) {
            char unit = argv[i + 1][strlen(argv[i + 1])-1];
            int value = atoi(argv[i + 1]);
            if (unit == 's') duration = value;
            if (unit == 'm') duration = value * 60;
            i++;
        }
        if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            threads = atoi(argv[i + 1]);
            i++;
        }
    }

    if (nodeOn) printf("Bugcoin node on\n");
    else {
        printf("Bugcoin node off\n");
        return 0;
    }

    int lastHash = getLastHash();

    // Listener de peers
    pthread_t listenerThread;
    pthread_create(&listenerThread, NULL, peerListener, NULL);

    if (mining) {
        printf("Mining Bugcoin for %d seconds using %d threads...\n", duration, threads);
        pthread_t tid[threads];
        for (int i = 0; i < threads; i++) {
            ThreadData *td = malloc(sizeof(ThreadData));
            td->startIndex = i + 1;
            td->lastHash = lastHash;
            td->duration = duration;
            pthread_create(&tid[i], NULL, mineThread, td);
        }
        for (int i = 0; i < threads; i++) pthread_join(tid[i], NULL);
        printf("Mining finished, total Bugcoins: %d\n", totalBugcoins);
    }

    // Detener listener al final
    running = 0;
    pthread_join(listenerThread, NULL);

    pthread_mutex_destroy(&lock);
    return 0;
}
