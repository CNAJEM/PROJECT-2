
// chash.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <semaphore.h>

// Hash Record Structure
typedef struct hash_struct
{
    uint32_t hash;
    char name[50];
    uint32_t salary;
    struct hash_struct *next;
} hashRecord;

// Reader-Writer Lock Structure
typedef struct _rwlock_t {
    sem_t writelock;
    sem_t lock;
    int readers;
} rwlock_t;

// Command Structure
typedef struct {
    char command[10]; // insert, delete, search, print
    char name[50];
    uint32_t salary;
} Command;

// Global Variables
hashRecord *head = NULL;
pthread_mutex_t insert_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t insert_cond = PTHREAD_COND_INITIALIZER;
int insert_counter = 0;
int lock_acquisitions = 0;
int lock_releases = 0;
rwlock_t rwlock;
FILE *log_file;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
int num_commands = 0;

// Function Declarations
void rwlock_init(rwlock_t *lock);
void rwlock_acquire_readlock(rwlock_t *lock);
void rwlock_release_readlock(rwlock_t *lock);
void rwlock_acquire_writelock(rwlock_t *lock);
void rwlock_release_writelock(rwlock_t *lock);
uint32_t jenkins_one_at_a_time_hash(const char *key);
void insert_record(char *name, uint32_t salary);
void delete_record(char *name);
hashRecord *search_record(char *name);
void print_table();
void init_logging();
void close_logging();
uint64_t get_timestamp();
void log_command(uint64_t timestamp, const char *command, const char *name, uint32_t salary);
void log_lock(uint64_t timestamp, const char *message);
void log_waiting(uint64_t timestamp, const char *message);
void log_search(uint64_t timestamp, uint32_t hash, const char *name, uint32_t salary);
void log_search_not_found(uint64_t timestamp);
void log_final_counts(int acquisitions, int releases);
void increment_lock_acquisitions();
void increment_lock_releases();
int read_commands(Command *commands);
void *execute_command(void *arg);

// Reader-Writer Lock Functions
void rwlock_init(rwlock_t *lock) {
    lock->readers = 0;
    sem_init(&lock->lock, 0, 1);
    sem_init(&lock->writelock, 0, 1);
}

void rwlock_acquire_readlock(rwlock_t *lock) {
    sem_wait(&lock->lock);
    lock->readers++;
    if (lock->readers == 1)
        sem_wait(&lock->writelock);
    sem_post(&lock->lock);
    increment_lock_acquisitions(); // Count lock acquisition
}

void rwlock_release_readlock(rwlock_t *lock) {
    sem_wait(&lock->lock);
    lock->readers--;
    if (lock->readers == 0)
        sem_post(&lock->writelock);
    sem_post(&lock->lock);
    increment_lock_releases(); // Count lock release
}

void rwlock_acquire_writelock(rwlock_t *lock) {
    sem_wait(&lock->writelock);
    increment_lock_acquisitions(); // Count lock acquisition
}

void rwlock_release_writelock(rwlock_t *lock) {
    sem_post(&lock->writelock);
    increment_lock_releases(); // Count lock release
}

// Jenkins One-at-a-Time Hash Function
uint32_t jenkins_one_at_a_time_hash(const char *key)
{
    size_t i = 0;
    uint32_t hash = 0;
    while (key[i] != '\0')
    {
        hash += key[i++];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

// Hash Table Functions
void insert_record(char *name, uint32_t salary) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);
    hashRecord *current = head;
    hashRecord *prev = NULL;

    while (current != NULL && current->hash < hash) {
        prev = current;
        current = current->next;
    }

    if (current != NULL && current->hash == hash) {
        // Update existing
        strcpy(current->name, name);
        current->salary = salary;
        return;
    }

    // Insert new node
    hashRecord *new_node = (hashRecord *)malloc(sizeof(hashRecord));
    new_node->hash = hash;
    strcpy(new_node->name, name);
    new_node->salary = salary;
    new_node->next = current;

    if (prev == NULL) {
        head = new_node;
    } else {
        prev->next = new_node;
    }
}

void delete_record(char *name) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);
    hashRecord *current = head;
    hashRecord *prev = NULL;

    while (current != NULL && current->hash < hash) {
        prev = current;
        current = current->next;
    }

    if (current != NULL && current->hash == hash) {
        // Delete node
        if (prev == NULL) {
            head = current->next;
        } else {
            prev->next = current->next;
        }
        free(current);
    }
    // Else, do nothing
}

hashRecord *search_record(char *name) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);
    hashRecord *current = head;

    while (current != NULL && current->hash <= hash) {
        if (current->hash == hash && strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void print_table() {
    hashRecord *current = head;
    while (current != NULL) {
        fprintf(log_file, "%u,%s,%u\n", current->hash, current->name, current->salary);
        current = current->next;
    }
}

// Logging Functions
void init_logging() {
    log_file = fopen("output.txt", "w");
    if (log_file == NULL) {
        fprintf(stderr, "Could not open output.txt for writing\n");
        exit(1);
    }
}

void close_logging() {
    fclose(log_file);
}

uint64_t get_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    return timestamp;
}

void log_command(uint64_t timestamp, const char *command, const char *name, uint32_t salary) {
    pthread_mutex_lock(&log_mutex);
    if (salary > 0) {
        fprintf(log_file, "%llu,%s,%s,%u\n", timestamp, command, name, salary);
    } else if (strcmp(command, "print") == 0) {
        fprintf(log_file, "%llu,%s\n", timestamp, "PRINT");
    } else {
        fprintf(log_file, "%llu,%s,%s\n", timestamp, command, name);
    }
    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void log_lock(uint64_t timestamp, const char *message) {
    pthread_mutex_lock(&log_mutex);
    fprintf(log_file, "%llu,%s\n", timestamp, message);
    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void log_waiting(uint64_t timestamp, const char *message) {
    pthread_mutex_lock(&log_mutex);
    fprintf(log_file, "%llu: %s\n", timestamp, message);
    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void log_search(uint64_t timestamp, uint32_t hash, const char *name, uint32_t salary) {
    pthread_mutex_lock(&log_mutex);
    fprintf(log_file, "%u,%s,%u\n", hash, name, salary);
    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void log_search_not_found(uint64_t timestamp) {
    pthread_mutex_lock(&log_mutex);
    fprintf(log_file, "%llu: Search: No Record Found\n", timestamp);
    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void log_final_counts(int acquisitions, int releases) {
    pthread_mutex_lock(&log_mutex);
    fprintf(log_file, "\nNumber of lock acquisitions:  %d\n", acquisitions);
    fprintf(log_file, "Number of lock releases:  %d\n", releases);
    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void increment_lock_acquisitions() {
    __sync_fetch_and_add(&lock_acquisitions, 1);
}

void increment_lock_releases() {
    __sync_fetch_and_add(&lock_releases, 1);
}

// Command Parsing Function
int read_commands(Command *commands) {
    FILE *file = fopen("commands.txt", "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open commands.txt\n");
        return -1;
    }

    char line[256];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ",\n");
        if (token == NULL) continue;

        if (strcmp(token, "threads") == 0) {
            token = strtok(NULL, ",\n");
            // We can use the number of threads if needed
        } else {
            strcpy(commands[count].command, token);
            token = strtok(NULL, ",\n");
            if (token != NULL) {
                strcpy(commands[count].name, token);
            } else {
                commands[count].name[0] = '\0';
            }
            token = strtok(NULL, ",\n");
            if (token != NULL) {
                commands[count].salary = atoi(token);
            } else {
                commands[count].salary = 0;
            }
            count++;
        }
    }
    fclose(file);
    num_commands = count;
    return count;
}

// Thread Function
void *execute_command(void *arg) {
    Command *cmd = (Command *)arg;
    uint64_t timestamp = get_timestamp();

    if (strcmp(cmd->command, "insert") == 0) {
        log_command(timestamp, "INSERT", cmd->name, cmd->salary);
        rwlock_acquire_writelock(&rwlock);
        log_lock(get_timestamp(), "WRITE LOCK ACQUIRED");

        insert_record(cmd->name, cmd->salary);

        rwlock_release_writelock(&rwlock);
        log_lock(get_timestamp(), "WRITE LOCK RELEASED");

        pthread_mutex_lock(&insert_mutex);
        insert_counter--;
        if (insert_counter == 0) {
            pthread_cond_broadcast(&insert_cond);
        }
        pthread_mutex_unlock(&insert_mutex);
    }
    else if (strcmp(cmd->command, "delete") == 0) {
        pthread_mutex_lock(&insert_mutex);
        while (insert_counter > 0) {
            log_waiting(timestamp, "WAITING ON INSERTS");
            pthread_cond_wait(&insert_cond, &insert_mutex);
            log_waiting(get_timestamp(), "DELETE AWAKENED");
        }
        pthread_mutex_unlock(&insert_mutex);

        log_command(get_timestamp(), "DELETE", cmd->name, 0);
        rwlock_acquire_writelock(&rwlock);
        log_lock(get_timestamp(), "WRITE LOCK ACQUIRED");

        delete_record(cmd->name);

        rwlock_release_writelock(&rwlock);
        log_lock(get_timestamp(), "WRITE LOCK RELEASED");
    }
    else if (strcmp(cmd->command, "search") == 0) {
        pthread_mutex_lock(&insert_mutex);
        while (insert_counter > 0) {
            log_waiting(timestamp, "WAITING ON INSERTS");
            pthread_cond_wait(&insert_cond, &insert_mutex);
            log_waiting(get_timestamp(), "SEARCH AWAKENED");
        }
        pthread_mutex_unlock(&insert_mutex);

        log_command(get_timestamp(), "SEARCH", cmd->name, 0);
        rwlock_acquire_readlock(&rwlock);
        log_lock(get_timestamp(), "READ LOCK ACQUIRED");

        hashRecord *result = search_record(cmd->name);
        if (result) {
            log_search(get_timestamp(), result->hash, result->name, result->salary);
        } else {
            log_search_not_found(get_timestamp());
        }

        rwlock_release_readlock(&rwlock);
        log_lock(get_timestamp(), "READ LOCK RELEASED");
    }
    else if (strcmp(cmd->command, "print") == 0) {
        log_command(timestamp, "PRINT", "", 0);

        rwlock_acquire_readlock(&rwlock);
        log_lock(get_timestamp(), "READ LOCK ACQUIRED");

        print_table();

        rwlock_release_readlock(&rwlock);
        log_lock(get_timestamp(), "READ LOCK RELEASED");
    }
    return NULL;
}

// Main Function
int main() {
    Command commands[1000];
    int count = read_commands(commands);
    if (count <= 0) {
        fprintf(stderr, "No commands to process.\n");
        return 1;
    }

    init_logging();
    rwlock_init(&rwlock);

    pthread_t threads[count];
    int i;

    // Count the number of insert commands
    for (i = 0; i < count; i++) {
        if (strcmp(commands[i].command, "insert") == 0) {
            insert_counter++;
        }
    }

    // Create threads
    for (i = 0; i < count; i++) {
        pthread_create(&threads[i], NULL, execute_command, (void *)&commands[i]);
    }

    // Wait for threads to finish
    for (i = 0; i < count; i++) {
        pthread_join(threads[i], NULL);
    }

    // Final Output
    log_final_counts(lock_acquisitions, lock_releases);

    // Acquire read lock to print the final table
    rwlock_acquire_readlock(&rwlock);
    log_lock(get_timestamp(), "READ LOCK ACQUIRED");

    print_table(); // This will print to output.txt

    rwlock_release_readlock(&rwlock);
    log_lock(get_timestamp(), "READ LOCK RELEASED");

    close_logging();
    return 0;
}



