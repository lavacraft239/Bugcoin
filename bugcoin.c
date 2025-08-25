#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_BUGCOINS 100000000

typedef struct {
    int index;
    int prevHash;
    int nonce;
    char data[256];
    int hash;
    int reward;
} Block;

int simpleHash(char *str) {
    int hash = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        hash += str[i];
    }
    return hash % 10000;
}

int mineBlock(char *data, int prevHash) {
    int nonce = 0;
    int hash;
    char temp[512];
    do {
        sprintf(temp, "%d%d%s", prevHash, nonce, data);
        hash = simpleHash(temp);
        nonce++;
    } while (hash % 1000 != 0);
    return nonce - 1;
}

int totalBugcoins = 0;
pthread_mutex_t lock;

void saveBlock(Block b) {
    FILE *f = fopen(".bugcoin", "a");
    if (!f) {
        printf("Error opening .bugcoin\n");
        return;
    }
    fprintf(f, "%d|%d|%d|%s|%d|%d\n", b.index, b.prevHash, b.nonce, b.data, b.hash, b.reward);
    fclose(f);
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
        if (totalBugcoins + b.reward > MAX_BUGCOINS) b.reward = MAX_BUGCOINS - totalBugcoins;
        totalBugcoins += b.reward;
    }
    pthread_mutex_unlock(&lock);

    saveBlock(b);
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

    pthread_mutex_destroy(&lock);
    return 0;
}
