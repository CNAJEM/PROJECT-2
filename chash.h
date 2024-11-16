
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
void log_threads_running(int count);
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

