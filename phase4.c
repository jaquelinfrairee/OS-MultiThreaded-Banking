#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS 2
#define INITIAL_BALANCE 1000.0

typedef struct {
	int account_id;
	double balance;
	int transaction_count;
	pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

void deposit_safe(int account_id, double amount) {

	pthread_mutex_lock(&accounts[account_id].lock);

	double current_balance = accounts[account_id].balance;

	usleep(1);
	double new_balance = current_balance + amount;

	accounts[account_id].balance = new_balance;
	accounts[account_id].transaction_count++;

	pthread_mutex_unlock(&accounts[account_id].lock);
}
void withdrawal_safe(int account_id, double amount) {
	
	pthread_mutex_lock(&accounts[account_id].lock);
	double current_balance = accounts[account_id].balance;

	usleep(1);

	double new_balance = current_balance - amount;

	accounts[account_id].balance = new_balance;
	accounts[account_id].transaction_count++;
	
	pthread_mutex_unlock(&accounts[account_id].lock);

}

void transfer_deadlock_example(int from_id, int to_id, double amount) {
	pthread_mutex_lock(&accounts[from_id].lock);
	printf("Thread %lu: Locked account %d\n", pthread_self(), from_id);

	usleep(100000);

	printf("Thread %lu: Waiting for account %d\n", pthread_self(), to_id);
	pthread_mutex_lock(&accounts[to_id].lock);

	accounts[from_id].balance -= amount;
	accounts[to_id].balance += amount;

	pthread_mutex_unlock(&accounts[to_id].lock);
	pthread_mutex_unlock(&accounts[from_id].lock);

}

void safe_transfer_ordered(int from_id, int to_id, double amount) {
	if (from_id == to_id) return;
	
	int first = (from_id < to_id) ? from_id : to_id;
	int second = (from_id < to_id) ? to_id : from_id;

	pthread_mutex_lock(&accounts[first].lock);
	pthread_mutex_lock(&accounts[second].lock);

	if (accounts[from_id].balance >= amount) {
	accounts[from_id].balance -= amount;
	accounts[to_id].balance += amount;

	accounts[from_id].transaction_count++;
	accounts[to_id].transaction_count++;
	}
	pthread_mutex_unlock(&accounts[second].lock);
	pthread_mutex_unlock(&accounts[first].lock);
}

void* thread_1(void* arg) {
	(void)arg;
	safe_transfer_ordered(0, 1, 10);
	return NULL;
}

void* thread_2(void* arg) {
	(void)arg;
	safe_transfer_ordered(1, 0, 10);
	return NULL;
} 

int main() {
	printf("=== Phase 4: Deadlock Resolution ===\n\n");

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;


	pthread_mutex_init(&accounts[i].lock, NULL);

	}
	printf("Initial State:\n");
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf(" Account %d: $%.2f\n", i, accounts[i].balance);
	}

	double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE;

	printf("\nExpected total: $%.2f\n\n", expected_total);

	pthread_t t1, t2;

	pthread_create(&t1, NULL, thread_1, NULL);
	pthread_create(&t2, NULL, thread_2, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	printf("\n=== Final Results ===\n");
	double actual_total = 0.0;

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf("Account %d: $%.2f (%d transactions)\n",
		i, accounts[i].balance, accounts[i].transaction_count);
	actual_total += accounts[i].balance;
	}

	printf("\nExpected total: $%.2f\n", expected_total);
	printf("Actual total: $%.2f\n", actual_total);
	printf("Difference: $%.2f\n", expected_total - actual_total);
	

	if (expected_total != actual_total) {
	//	printf("\n*** RACE CONDITION DETECTED! ***\n");
	//	printf("Run the program multiple times to see different results.\n");
	}

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		pthread_mutex_destroy(&accounts[i].lock);
	}

	return 0;
	
}
