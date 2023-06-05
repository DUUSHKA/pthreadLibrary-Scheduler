// File:	thread-worker.c

// List all group member's name: Jared Cheung, Konrad Radecki (jlc598, kr536)
// username of iLab: 
// iLab Server: ilab4.cs.rutgers.edu

#include "thread-worker.h"
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

// timer
struct sigaction sa;
struct itimerval timer;

long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;
int num_finished_processes = 0;
int mutex_count = 0;
// head for queue1
node* head1;
int head1_init = 0;
// head for queue2
node* head2;
int head2_init = 0;

// head for queue3
node* head3;
int head3_init = 0;

// head for queue4
node* head4;
int head4_init = 0;

// head for unique_id
tNode* unique_head;
int unique_init = 0;
worker_t unique_id = 0;

// list for threads that are waiting to join
node* join_thread_head = NULL;

// blocked list head
node* blocked_thread_head = NULL;

// global scheduler context
ucontext_t scheduler_context;
int sc_init = 0;

// global main context (new)
ucontext_t main_context;
int mc_init = 0;

//currently running thread & nextRunningThread
node* currentRunning = NULL;
node* currentList;

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg)
{
    // Create Thread Control Block
    // Create and initialize the context of this thread
    // Allocate space of stack for this thread to run
    // after everything is all set, push this thread int
    // YOUR CODE HERE

    //initialize timer 
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &swap;
    sigaction(SIGPROF, &sa, NULL);

    
    //initilize lists
    if(join_thread_head == NULL)
    {
        join_thread_head = (node*)malloc(sizeof(node));
        join_thread_head->next = NULL;
    }
    if(blocked_thread_head == NULL)
    {
        blocked_thread_head = (node*)malloc(sizeof(node));
        blocked_thread_head->next = NULL;
    }

    /* scheduler set up */
    

    /* initialize tcb */
    tcb* block = (tcb*)malloc(sizeof(tcb));

    /* unique id set up */
    if (unique_init == 0)
    {
        unique_init = 1; // indicates that head is initialized

        // initialize head
        unique_head = (tNode*)malloc(sizeof(tNode));

        // initialize new tNode
        tNode* new_tNode = (tNode*)malloc(sizeof(tNode));
        new_tNode->status = RUNNING;
        new_tNode->pid = unique_id;
        unique_id++;
        new_tNode->next = NULL;

        // put tNode at end of list
        unique_head->next = new_tNode;
        unique_head->status = RUNNING;

        block->pid = new_tNode->pid;
        *thread = block->pid;
    }
    else
    {
        //pointer usued for traversal
        tNode* unique_ptr = unique_head; 

    
        while(unique_ptr->next != NULL)
        {
            if(unique_ptr->status == BLOCKED) 
            {
                break;
            }
            unique_ptr = unique_ptr->next;
        }

        if(unique_ptr->status == BLOCKED) 
        {
            unique_ptr->status == RUNNING; 
            block->pid = unique_ptr->pid; 
            *thread = block->pid;
        }

        else
        {
            // set up the new tNode
            tNode* new_tNode = (tNode*)malloc(sizeof(tNode));
            new_tNode->status = RUNNING;
            new_tNode->pid = unique_id;
            unique_id++;
            new_tNode->next = NULL;

            
            unique_ptr->next = new_tNode;

            block->pid = new_tNode->pid;
            *thread = block->pid;
        }
    }
    if (sc_init == 0)
    {
        sc_init = 1;

       
        // Set up stack
        getcontext(&(scheduler_context));
        void* scheduler_stack = malloc(SIGSTKSZ);
        scheduler_context.uc_link = NULL;
        scheduler_context.uc_stack.ss_sp = scheduler_stack;
        scheduler_context.uc_stack.ss_size = SIGSTKSZ;
        scheduler_context.uc_stack.ss_flags = 0;

        // bind scheduler context with function scheduler
        makecontext(&scheduler_context, (void*)&schedule, 0);
    }
    // Set up context for block
    ucontext_t new_ctt;
    getcontext(&(new_ctt));
    void *stack = malloc(SIGSTKSZ);
    new_ctt.uc_link = NULL;
    new_ctt.uc_stack.ss_sp = stack;
    new_ctt.uc_stack.ss_size = SIGSTKSZ;
    new_ctt.uc_stack.ss_flags = 0;
    block->status = READY;
    block->priority = 1;
    block->counter = 0;
    block->waiter = -1;
    block->arrival_time = -1;
    block ->first_run_time =-1;
    block -> completion_time = 0;
    block->context = new_ctt;

    makecontext(&(block->context),(void*)function, 1, (void*)arg);

    // Create new node add to end of head1 do same for other lists
    if (head1_init == 0)
    {
        head1_init = 1;
        head1 = (node*)malloc(sizeof(node));
        head1->next = NULL;
       
        currentList = head1;
    }

    if (head2_init == 0)
    {
        head2_init = 1;
        head2 = (node*)malloc(sizeof(node));
        head2->next = NULL;
    }

    if (head3_init == 0)
    {
        head3_init = 1;
        head3 = (node*)malloc(sizeof(node));
        head3->next = NULL;
    }

    if (head4_init == 0)
    {
        head4_init = 1;
        head4 = (node*)malloc(sizeof(node));
        head4->next = NULL;
    }

    
    node* new_node = (node*)malloc(sizeof(node));
    new_node->value = block;

    if(head1->next == NULL)
    {
        head1->next = new_node;
        new_node->next = NULL;
    }
    else
    {
        node* temp = head1->next;
        head1->next = new_node;
        new_node->next = temp;
    }

    //initialize main context
    if (mc_init == 0)
    {
        mc_init = 1;
        getcontext(&(main_context));
        void* gm_stack = malloc(SIGSTKSZ);
        main_context.uc_link = NULL;
        main_context.uc_stack.ss_sp = gm_stack;
        main_context.uc_stack.ss_size = SIGSTKSZ;
        main_context.uc_stack.ss_flags = 0;

        // set up tcb
        tcb* main_tcb = (tcb*)malloc(sizeof(tcb));
        main_tcb->counter = 0;
        main_tcb->status = READY;
        main_tcb->priority = 1;
        main_tcb->waiter = -1;
        main_tcb->context = main_context;
        main_tcb->arrival_time = -1;
        main_tcb ->first_run_time =-1;
        main_tcb -> completion_time = 0;
    
        tNode* unique_ptr2 = unique_head;
        while(unique_ptr2->next != NULL)
        {
            if(unique_ptr2->status == BLOCKED) 
            {
                break;
            }
            unique_ptr2 = unique_ptr2->next;
        }

        if(unique_ptr2->status == BLOCKED) 
        {
            unique_ptr2->status = RUNNING; 
            main_tcb->pid = unique_ptr2->pid; 
        }

        else 
        {
            //create new tNode
            tNode* new_tNode2 = (tNode*)malloc(sizeof(tNode));
            new_tNode2->status = RUNNING;
            new_tNode2->pid = unique_id;
            unique_id++;
            new_tNode2->next = NULL;

           
            unique_ptr2->next = new_tNode2;

            main_tcb->pid = new_tNode2->pid;
        }

        node* main_node = (node*)malloc(sizeof(node)); 
        main_node->value = main_tcb;
        currentRunning = main_node;

        node* main_ptr = head1->next;
        head1->next = main_node;
        main_node->next = main_ptr;
        swap();
    }

    // stop timer until it is used in scheduler 
    else
    {
        timer.it_interval.tv_usec = 0;
        timer.it_interval.tv_sec = 0;
        timer.it_value.tv_usec = 0;
        timer.it_value.tv_sec = 0;
        setitimer(ITIMER_PROF, &timer, NULL); 

        swap();
    }

   

    return block->pid; // return unique thread id to the user
}

/* give CPU possession to other user-level threads voluntarily */
int worker_yield() {
	// - change thread state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context
	
	// YOUR CODE HERE

    stopTimer();

    //Since there is a thread running, yiled it and add to end of Running list
	tcb* temp = currentRunning->value;
    temp->status = YIELD;
	swapcontext(&(temp->context),&(scheduler_context));
	tot_cntx_switches++;

	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread
	
    // YOUR CODE HERE

    stopTimer();

    // handle the thread that is waitting for the caller thread
    worker_t waiter_id = currentRunning->value->waiter; 

    if(waiter_id != -1){
        // while waiting for current thread, let the waiter thread be ready
        node* waiter_node;

        waiter_node = findNode(waiter_id);
        if(waiter_node == NULL) 
        waiter_node->value->status = READY; 
        int exit = 0;
        node* ptr = head1->next;
        while(ptr!=NULL){
            if(ptr->value->counter > 0){
                exit= 1;
            }
            ptr = ptr->next;
        }
        int waiter_counter = waiter_node->value->counter;
        node* list_ptr;
        if(exit == 0){
            //we are in mlfq
            if(waiter_counter == 0){
                list_ptr = head1->next;
            }
            else if(waiter_counter == 1){
                list_ptr = head2->next;
            }
            else if(waiter_counter == 2){
                list_ptr = head3->next;
            }
            else{
                list_ptr = head4->next;
            }
            while(list_ptr->next!=NULL){
                list_ptr = list_ptr->next;
            }
            list_ptr->next = waiter_node;
        }
        else{
            // schf
            node* list_ptr_prev = head1;
            list_ptr = head1->next;

            if(list_ptr == NULL){
                list_ptr_prev->next = waiter_node;
            }
            else{
                while(list_ptr->value->counter < waiter_counter){
                    list_ptr_prev = list_ptr;
                    list_ptr = list_ptr->next;
                }
                list_ptr_prev->next = waiter_node;
                waiter_node->next = list_ptr;
            }
        }
        currentRunning->value->waiter = -1;

    }

    currentRunning->value->return_value = value_ptr;
    currentRunning->value->status = TERMINATED;
    node* ptr = currentList;
    while(ptr->next != currentRunning){
        ptr = ptr->next;
    }
    ptr->next = currentRunning->next;
    if(join_thread_head == NULL){
        join_thread_head = (node*)malloc(sizeof(node));
        join_thread_head->next = currentRunning;
        currentRunning->next = NULL;
        setcontext(&(scheduler_context));
        return;
    }

    node* wait_ptr = join_thread_head;
    while(wait_ptr->next != NULL){
        wait_ptr = wait_ptr->next;
    }
    currentRunning->next = NULL;
    wait_ptr->next = currentRunning;
    setcontext(&(scheduler_context));
};


//* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {

	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
    stopTimer();
    int join_val = 0;
    node* thread_ptr = NULL;

    if(join_thread_head->next == NULL)
    {
        int join_val = 1; 
    }
    else
    {
        // ptr to the next since head might be NULL
        node* join_ptr = join_thread_head->next;
        while(join_ptr != NULL)
        {
            if(join_ptr->value->pid == thread)
            {
                join_val = 2;
                thread_ptr = join_ptr;
                break;
            }
            join_ptr = join_ptr->next;
        }


        //Here 0 means list no longer exists, 1 means not found and 2 means found
        if (join_val == 0)
        {
            join_val = 1;
        }
    }
        if(join_val == 2)
        {
           setTime();
        }else {
            currentRunning->value->status = BLOCKED;
            if(blocked_thread_head == NULL)
            {
                blocked_thread_head = (node*)malloc(sizeof(node));
                blocked_thread_head->next = NULL;
            }
            node* temp_ptr = currentList;
            while(temp_ptr->next != currentRunning)
            {
                temp_ptr = temp_ptr->next;
            }

            temp_ptr->next = currentRunning->next;
            node* blocked_ptr = blocked_thread_head;
            while(blocked_ptr->next != NULL)
            {
                blocked_ptr = blocked_ptr->next;
            }

            blocked_ptr->next = currentRunning;
            currentRunning->next = NULL;
            node* join_ptr2 = head1->next;
            while(join_ptr2 != NULL)
            {
                if(join_ptr2->value->pid == thread)
                {
                    thread_ptr = join_ptr2;
                    break;
                }
                join_ptr2 = join_ptr2->next;
            }
            if(thread_ptr == NULL)
            {
                node* join_ptr3 = head2->next;
                int MLFQ_val = 0;
                while(join_ptr3 != NULL)
                {
                    if(join_ptr3->value->pid == thread) 
                    {
                        thread_ptr = join_ptr3;
                        MLFQ_val = 1;
                        break;
                    }
                    join_ptr3 = join_ptr3->next;
                }
                join_ptr3 = head3->next;
                while(join_ptr3 != NULL && MLFQ_val == 0)
                {
                    if(join_ptr3->value->pid == thread) 
                    {
                        thread_ptr = join_ptr3;
                        MLFQ_val = 1;
                    }
                    join_ptr3 = join_ptr3->next;
                }
                join_ptr3 = head4->next;
                while(join_ptr3 != NULL && MLFQ_val == 0)
                {
                    if(join_ptr3->value->pid == thread)
                    {
                        thread_ptr = join_ptr3;
                        MLFQ_val = 1;
                    }
                    join_ptr3 = join_ptr3->next;
                }
            }
            thread_ptr->value->waiter = currentRunning->value->pid;

            //Current thread is waiting and has not finished
            while(thread_ptr->value->waiter != -1)
            {
                stopTimer();
                swapcontext(&(currentRunning->value->context), &scheduler_context);
                tot_cntx_switches++;
            }
        }
        if(value_ptr != NULL)
        {
            *value_ptr = thread_ptr->value->return_value; 
        }

        node* free_node = join_thread_head;

        while(free_node->next != thread_ptr)
        {
             free_node = free_node->next;
        }

         free_node->next = thread_ptr->next;

        tNode* free_ptr = unique_head->next;

        while(free_ptr->pid != thread_ptr->value->pid)
        {
            free_ptr = free_ptr->next;
        }

        free_ptr->status = BLOCKED;

        // free tcb
        free(thread_ptr->value);

        // free node
        free(thread_ptr);

	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
                          
        mutex->mid = mutex_count++;
        mutex->locked = 0;
        mutex-> pid = -1;
        mutex ->ready_waiting = 0;
	//Initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {
        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread
        if (mutex == NULL) {
		return -1;
	}
	
	while (mutex->locked) {
		currentRunning->value->status = mutex->mid;
	} 
	mutex->locked = 1;
	mutex->pid = currentRunning->value->pid;
	mutex->ready_waiting = 0;
        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {

	// - release mutex and make it available again.
	// - put threads in block list to run queue
	// so that they could compete for mutex later.
	if (mutex == NULL) {
		return -1;
	}
	
	if (mutex->locked == 0) {
		//Can't unlock an unlocked mutex
		return -1;
	} else {
		if (mutex->pid != currentRunning->value->pid) {
			//Wrong thread calling unlock
			return -1;
		} else {
			mutex->locked = 0;
			mutex->pid = -1;
			node * temp = join_thread_head->next;
			while (temp != NULL) {
				if (mutex->mid == temp->value->status) {
					temp->value->status = BLOCKED;
					temp->value->priority = 0;
					mutex->ready_waiting = 1;
					return 0;
				}
				temp = temp->next;
				}
				}
				}
				
	// YOUR CODE HERE
	return 0;
	
};

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {

	// - de-allocate dynamic memory created in rpthread_mutex_init
	if (mutex == NULL) {
		return -1;
	}
	
	if (mutex->locked) {
		return -1;
	}
	
	if (mutex->ready_waiting) {
		return -1;
	}
	
	node * temp = join_thread_head->next;
	
	while (temp != NULL) {
		if (mutex->mid == temp->value->status) {
			return -1;
		}
		temp = temp->next;
	}
	//free(mutex);
	mutex = NULL;

	return 0;
	
};
/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	

    // If the current thread has exited, schedule the next job and run it
    if(currentRunning->value->status == TERMINATED){
 currentRunning->value->completion_time = (getTime() - currentRunning->value->arrival_time);
        avg_turn_time += currentRunning->value->completion_time;
        num_finished_processes++;
        mlfq_schedule();
    }


	 // Update the counter and reinstate into thread
    if(currentRunning->value->status != YIELD){
        currentRunning->value->counter++;
        
    }

    if(currentRunning->value->status != BLOCKED)
    {
         mlfq_insertProcess(currentRunning->value->counter);
         currentRunning->value->status = READY;
    }
  
    if (currentRunning->value->arrival_time == -1) {
        currentRunning->value->arrival_time = getTime();
    }
    stopTimer();
    mlfq_schedule();
    setTime();  

    currentRunning->value->status = RUNNING; 
    if (currentRunning->value->first_run_time == -1) {
 currentRunning->value->first_run_time = (getTime() - currentRunning->value->arrival_time);
        avg_resp_time+=currentRunning->value->first_run_time;
        avg_resp_time/=1000000000;
    }
    setTime();
    setcontext(&(currentRunning->value->context));
    return;
}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	if (currentRunning->value->arrival_time == -1) {
        currentRunning->value->arrival_time = getTime();
    }
    // If the current running thread is finished, select the next thread to run
    if(currentRunning->value->status == TERMINATED){
     currentRunning->value->completion_time = (getTime() - currentRunning->value->arrival_time);
        avg_turn_time += currentRunning->value->completion_time;
        num_finished_processes++;
        currentRunning = head1->next;
        currentRunning->value->status = RUNNING; // 2 for running
        setTime();
        setcontext(&(currentRunning->value->context));
        return;
    }
	
    currentRunning->value->counter++;
    
    
    //reinsert the currentRunning Thread into the list
    stcf_insertProcess(currentRunning->value->counter);
    
     currentRunning->value->status = RUNNING; 
    if (currentRunning->value->first_run_time == -1) {
  currentRunning->value->first_run_time = (getTime() - currentRunning->value->arrival_time);
         avg_resp_time+=currentRunning->value->first_run_time;
}
   // Select the next thread to run
    currentRunning = head1->next;
    currentRunning->value->status = RUNNING; 
   
    setTime();
    setcontext(&(currentRunning->value->context));


}
/* scheduler */
static void schedule() {
	
	/*if (SCHED == PSJF)
	 	sched_psjf();
	else if (SCHED == MLFQ)
		sched_mlfq();	
		*/
		
	
    // sched_stcf();
   // sched_mlfq();

    // schedule policy
    #ifndef MLFQ
	    sched_stcf();
    #else
	    sched_mlfq();
    #endif

}



void print_app_stats(void) {
	avg_turn_time/=num_finished_processes;
	avg_resp_time/=num_finished_processes;
	//Calculated in microseconds
       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
       fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}

// YOUR CODE HERE

// MLFQ: schdule a new thread
void mlfq_schedule(){
    node* ptr1 = head1->next;
    node* ptr2 = head2->next;
    node* ptr3 = head3->next;
    node* ptr4 = head4->next;

    if(ptr1 != NULL){
        currentRunning = ptr1;
        currentList = head1;
        return;
    }
    // now head1->next is NULL, which means no threads in head1
    if(ptr2 != NULL){
        currentRunning = ptr2;
        currentList = head2;
        return;
    }
    // now head2->next is NULL, which means no threads in head1 and head2
    if(ptr3 != NULL){
        currentRunning = ptr3;
        currentList = head3;
        return;
    }
  //   now head3->next is NULL, which means no threads in head1, head2 and head3
  //  which means that all threads are in head4
    
    if(ptr4 != NULL){
        // Since only head4 has threads mlfq runs like RR instead
        currentRunning = ptr4;
        currentList = head4;
        return;
    }

    // no more threads can be scheduled 
    currentRunning = NULL;
}

// MLFQ: insert currentRunning to a queue based on amount of times run (counter)
void mlfq_insertProcess(int counter){
     if (counter == 1) {
        if (currentRunning->value->status != BLOCKED) {
            currentList->next = currentRunning->next;
        }
        node* ptr = head2;
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        ptr->next = currentRunning;
        currentRunning->next = NULL;
    }
    else if (counter == 2) {
        if (currentRunning->value->status != BLOCKED) {
            currentList->next = currentRunning->next;
        }
        node* ptr = head3;
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        ptr->next = currentRunning;
        currentRunning->next = NULL;
    }
    else if (counter > 2) {
        if (currentRunning->value->status != BLOCKED) {
            currentList->next = currentRunning->next;
        }
        node* ptr = head4;
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        ptr->next = currentRunning;
        currentRunning->next = NULL;
    }
}

//Searches for thread node using the tNode of the thread and removes it from the list 
node* findNode(worker_t target_tNode){
   node* blocked_ptr = blocked_thread_head->next;
   node* blockedPrev_ptr = blocked_thread_head;
   while(blocked_ptr != NULL){
       if(blocked_ptr->value->pid == target_tNode){
           blockedPrev_ptr->next = blocked_ptr->next;
           blocked_ptr->next = NULL;
           return blocked_ptr;
       }
       blockedPrev_ptr = blocked_ptr;
       blocked_ptr = blocked_ptr -> next;
   }
   return NULL;
   
}

// insert currentRunning Thread to the proper position in the queue
void stcf_insertProcess(int index){
    // if this node is the last node, keep the same
    if(currentRunning->next == NULL)
    {
        return;
    }

    // if this node has run the same amount of times as the one after it, keep same
    if(index <= currentRunning->next->value->counter)
    {
        return;
    }

    // change if the number of run times is less
    node* node_check = head1;

    while(node_check->next != currentRunning)
    {
        node_check = node_check->next;
    }

    node* temp = currentRunning->next;
    currentRunning->next = temp->next;
    temp->next = currentRunning;
    node_check->next = temp;
}
void setTime(){
    // 0 means do not repeat timer
    timer.it_interval.tv_usec = 0;
	timer.it_interval.tv_sec = 0;

	timer.it_value.tv_usec = 5;
	timer.it_value.tv_sec = 0;

	setitimer(ITIMER_PROF, &timer, NULL);
}

void stopTimer(){
    timer.it_interval.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_value.tv_sec = 0;
    setitimer(ITIMER_PROF, &timer, NULL); // timer is stopped 
}

void startTimer(){

    timer.it_interval.tv_usec = 20000;
    timer.it_interval.tv_sec = 0;

    timer.it_value.tv_usec = 20000;
    timer.it_value.tv_sec = 0;

    setitimer(ITIMER_PROF, &timer, NULL);
}// timer starts
long getTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
void swap(){
    // swap context when timer ends
    swapcontext(&(currentRunning->value->context),&(scheduler_context));
    tot_cntx_switches++;
}

