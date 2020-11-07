#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

double get_current_time();

struct bank_account {
    int balance;
    char* identifier;
} typedef bank_account;

struct bank_account_node {
    bank_account *account;
    struct bank_account_node *next;
} typedef bank_account_node;

const int BANK_MUTEX_INDEX = 0;
const int TRANSACTION_COUNT_MUTEX_INDEX = 1;
const int RANDOM_MUTEX_INDEX = 2;

pthread_mutex_t mutex_list[] = {
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER
};

bank_account_node* bank_account_chain;

void mutex_setup();
void add_bank_account(bank_account* account);//adds a new account to bank account chain. Thread safe.
void create_bank_chain(bank_account* initial);//used by add_bank_account. Not thread safe.
void list_accounts();//lists all accounts on chain. Thread safe.
void print_account(bank_account* bank_account);//prints an bank account structure.
bank_account *get_bank_account(char* identifier); // linear search for account by identifier. Thread Safe. Worst case n^2.
int transfer_money(bank_account *from, bank_account *to, int amount); // Transfers money from account 'from' to account 'to'. Returns 1 if transfer is OK.


/*....testing methods....*/
void setup_bank_chain_test_data();//creates test data.
void *setup_bank_chain_test_thread(void *void_ptr);//tests transactions between accounts on a seperate thread.

//Test macro parameters, these all have an impact on performance
#define TEST_BANK_ACCOUNTS_COUNT 3 // should probably not increase this too much
#define TEST_THREAD_COUNT 100
#define TEST_MAX_TRANSACTION_COUNT 1000 // limit 100.000, increase thread count accordingly

int transaction_count = 0;
int program_running = 1;
/*...............*/

int main() {
    mutex_setup();
    setup_bank_chain_test_data();
    list_accounts();

    // TEST 1: 1 THREAD
    double test_one_start_time = get_current_time();
    pthread_t p1;
    pthread_create(&p1, NULL, setup_bank_chain_test_thread, (void *) NULL);
    while (program_running) {  }
    double test_one_complete_time = get_current_time();

    // TEST 2: 50 THREADS
    program_running = 1;
    transaction_count = 0;
    for (int i=0; i < TEST_THREAD_COUNT; i++) {
        pthread_t p;
        pthread_create(&p, NULL, setup_bank_chain_test_thread, (void *) NULL);
    }
    while (program_running) {}

    printf("Performed %d transactions\n", TEST_MAX_TRANSACTION_COUNT);
    printf("------------------------------------------------\n");
    printf("%.6f seconds with 1 thread\n", test_one_complete_time - test_one_start_time);
    printf("%.6f seconds with %i threads\n", get_current_time() - test_one_complete_time, TEST_THREAD_COUNT);

    return 0;
}

void mutex_setup() {
    size_t n = sizeof(mutex_list) / sizeof(mutex_list[0]);
    for (int i = 0; i < n; i++) {
        pthread_mutex_init(&mutex_list[i], NULL);
    }
}

// adds a bank account to the bank account chain
// Thread Safe
void add_bank_account(bank_account* account) {
    pthread_mutex_lock(&mutex_list[BANK_MUTEX_INDEX]);
    if (bank_account_chain == NULL) {
        create_bank_chain(account);
    } else {
        bank_account_node *account_tail_node = bank_account_chain;
        while (account_tail_node->next != NULL) {
            account_tail_node = account_tail_node->next;
        }

        bank_account_node *account_node = malloc(sizeof(bank_account_node));
        account_node->account = account;
        account_node->next = NULL;
        account_tail_node->next = account_node;
    }
    pthread_mutex_unlock(&mutex_list[BANK_MUTEX_INDEX]);
}

// "create_bank_account" chain is called from add_bank_account
// do not call this "create_bank_account" manually as it isn't thread safe!
void create_bank_chain(bank_account* account) {
    bank_account_chain = malloc(sizeof(bank_account_node));
    bank_account_chain->account = account;
    bank_account_chain->next = NULL;
}

// lists all of the bank accounts
// Thread Safe
void list_accounts() {
    pthread_mutex_lock(&mutex_list[BANK_MUTEX_INDEX]);
    bank_account_node *current_account_node = bank_account_chain;
    do {
        print_account( current_account_node->account );
        current_account_node = current_account_node->next;
    } while (current_account_node != NULL);
    pthread_mutex_unlock(&mutex_list[BANK_MUTEX_INDEX]);
}

// prints an bank account using a bank_account structure
void print_account(bank_account* bank_account) {
    printf("%s has a balance of %d \n", bank_account->identifier, bank_account->balance);
}

// method that tests various parts of our bank chain.
void setup_bank_chain_test_data() {
    srand(time(NULL));

    for (int i = 0; i < TEST_BANK_ACCOUNTS_COUNT; i++) {
        bank_account* bank_account = malloc(sizeof(bank_account));
        bank_account->identifier = malloc(sizeof(char) * 20);

        sprintf(bank_account->identifier, "bank account %d", i+1);
        bank_account->balance = (rand() % 20000) + 10000; // sample accounts have between 10k and 30k balance
        add_bank_account(bank_account);
    }
}

// Transfers money from account 'from' to account 'to'. Returns 1 if transfer is OK.
// Thread Safe.
int transfer_money(bank_account *from, bank_account *to, int amount) {
    pthread_mutex_lock(&mutex_list[BANK_MUTEX_INDEX]);
    if (from->balance < amount) {
        pthread_mutex_unlock(&mutex_list[BANK_MUTEX_INDEX]);
        return 0;
    }

    from->balance -= amount;
    to->balance += amount;

    pthread_mutex_unlock(&mutex_list[BANK_MUTEX_INDEX]);
    return 1;
}

// Linear search for account by identifier. Thread Safe. Worst case n^2.
// Returns NULL if no unit was found!
// Thread Safe.
bank_account *get_bank_account(char* identifier) {
    pthread_mutex_lock(&mutex_list[BANK_MUTEX_INDEX]);
    bank_account_node *current_account_node = bank_account_chain;
    do {
        if (strcmp(current_account_node->account->identifier, identifier) == 0) {
            pthread_mutex_unlock(&mutex_list[BANK_MUTEX_INDEX]);
            return current_account_node->account;
        }
        current_account_node = current_account_node->next;
    } while (current_account_node != NULL);
    pthread_mutex_unlock(&mutex_list[BANK_MUTEX_INDEX]);
    return NULL;
}

//tests transactions between accounts on a seperate thread.
void *setup_bank_chain_test_thread(void *void_ptr) {
    /*Had to artificially increase the execution time per thread because
     * the overall execution time without would become skewed,
     * and increasing the transaction count to a larger number caused a segfault
     */
    usleep(50);


    pthread_mutex_lock(&mutex_list[RANDOM_MUTEX_INDEX]);
    // find two random accounts
    char* random_account_id1 = malloc(sizeof(char)*20);
    char* random_account_id2 = malloc(sizeof(char)*20);
    sprintf(random_account_id1, "bank account %d", (rand() % (TEST_BANK_ACCOUNTS_COUNT - 1)) + 1);
    sprintf(random_account_id2, "bank account %d", (rand() % (TEST_BANK_ACCOUNTS_COUNT - 1)) + 1);

    // find a random amount to transfer
    int transfer_amount = rand() % 500;
    pthread_mutex_unlock(&mutex_list[RANDOM_MUTEX_INDEX]);

    bank_account *bank_account1 = get_bank_account( random_account_id1 );
    bank_account *bank_account2 = get_bank_account( random_account_id2 );

    if (transfer_money( bank_account1, bank_account2, transfer_amount )) {
        // transfer between bank_account1 and bank_account2 OK
    } else {
        // transfer between bank_account1 and bank_account2 failed!
    }

    pthread_mutex_lock(&mutex_list[TRANSACTION_COUNT_MUTEX_INDEX]);
    transaction_count++;
    int condition = transaction_count >= TEST_MAX_TRANSACTION_COUNT;
    pthread_mutex_unlock(&mutex_list[TRANSACTION_COUNT_MUTEX_INDEX]);

    free(random_account_id1);
    free(random_account_id2);

    if (!condition) {
        setup_bank_chain_test_thread((void*)NULL); // continue until MAX_TRANSACTION_COUNT has been reached.
    }
    else {
        program_running = 0;
    }
}

double get_current_time() {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec+now.tv_nsec*1e-9;
}