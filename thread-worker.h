// File:	worker_t.h

// List all group member's name: Jared Cheung, Konrad Radecki (jlc598, kr536)
// username of iLab: 
// iLab Server: ilab4.cs.rutgers.edu

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>


#define BLOCKED 0
#define RUNNING 1
#define READY 2
#define TERMINATED 3
#define YIELD 4 


typedef uint worker_t;

typedef struct TCB {
	// YOUR CODE HERE
	worker_t pid;
	int status; // 0 for blocked, 1 for running, 2 for ready, 3 for terminated, 4 for yield
	ucontext_t context;
	int priority;
	int counter;
	void* return_value;
	worker_t waiter;
	double arrival_time; 
	double first_run_time;
	double completion_time;

} tcb;

typedef struct threadNode
{
    worker_t pid;
    int status; // 0 is blocked 1 is running
    struct threadNode* next;
}tNode;

/* LL */
typedef struct node
{
    tcb* value;
    struct node *next;
}node;

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */
	int mid; 
     int locked;
     int ready_waiting; 
     worker_t pid;
	/*
	worker_t pid;
	int status; //1 = locked, 0 = unlocked
*/
	// YOUR CODE HERE
} worker_mutex_t;
/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE


/* Function Declarations: */

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);

/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void);

static void schedule();

static void sched_stcf();

static void sched_mlfq();

// Your Code here

void stcf_insertProcess(int index);
void mlfq_schedule();
void mlfq_insertProcess(int counter);
node* findNode(worker_t target_tid);


//helper fucntions
void setTime();
void startTimer();
void stopTimer();
long getTime();
void swap();


#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#define pthread_yield worker_yield
#endif

#endif

