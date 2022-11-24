/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
	void getSlot(task_t task); /* task tries to use slot on the bus */
	void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
	void leaveSlot(task_t task); /* task release the slot */


int current_direction_thread = SENDER;  /*for declaring current direction of data transfer*/
/*semaphores declared globally*/
struct semaphore bus_size;  /*semaphore for BUS_CAPACITY slots*/
struct semaphore send_priority_task_sema; /*semaphore for priority tasks waiting to send*/
struct semaphore receive_priority_task_sema; /*semaphore for priority tasks waiting to recieve*/
struct semaphore send_lowPriority_task_sema; /*semaphore for low priority tasks waiting to send*/
struct semaphore receive_lowPriority_task_sema; /*semaphore for low priority tasks waiting to receive*/

struct lock mutex_bus;   /*It is used for ensures mutual exclusion*/

/* initializes semaphores */ 
void init_bus(void){ 
 
    random_init((unsigned int)123456789); 
    
     sema_init(&bus_size,BUS_CAPACITY);  /*bus has BUS_CAPACITY slots*/
    
    /*counting sema*/
    sema_init(&send_priority_task_sema,0);
    sema_init(&receive_priority_task_sema,0);
    sema_init(&send_lowPriority_task_sema,0);
    sema_init(&receive_lowPriority_task_sema,0);
    lock_init (&mutex_bus);

}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
    unsigned int i;
    /* create sender threads */
    for(i = 0; i < num_tasks_send; i++)
        thread_create("sender_task", 1, senderTask, NULL);

    /* create receiver threads */
    for(i = 0; i < num_task_receive; i++)
        thread_create("receiver_task", 1, receiverTask, NULL);

    /* create high priority sender threads */
    for(i = 0; i < num_priority_send; i++)
       thread_create("prio_sender_task", 1, senderPriorityTask, NULL);

    /* create high priority receiver threads */
    for(i = 0; i < num_priority_receive; i++)
       thread_create("prio_receiver_task", 1, receiverPriorityTask, NULL);
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) 
{
    /*First we check the priority,followed by the direction and then subsequently increase the corresponding semapores */
    if(task.priority == HIGH) 
      {
      if(task.direction == SENDER)
          sema_up(&send_priority_task_sema);
      else 
          sema_up(&receive_priority_task_sema); 
      } 
    
    else  
      {      /*for normal tasks*/
      if(task.direction == SENDER)     
          sema_up(&send_lowPriority_task_sema);  
      else 
          sema_up(&receive_lowPriority_task_sema);
      }       

/*if the direction is differnt from the thread*/
    while(true)
       {
	/*enter critical section*/
	lock_acquire(&mutex_bus);
        /*checks if bus is free or in our direction*/
        if(bus_size.value == BUS_CAPACITY || current_direction_thread ==  task.direction )    
         {  

            /*Checking if there are any high priority task waiting or not*/
           if(task.priority == HIGH || (send_priority_task_sema.value==0 && receive_priority_task_sema.value==0))
             {
               /*For changing to my direction when the bus was available*/
              current_direction_thread = task.direction; 

              /*Reducing available slots on the bus*/
              sema_down(&bus_size);

              if(task.priority == HIGH) {
                if(task.direction == SENDER)
                  sema_down(&send_priority_task_sema);
                else 
                  sema_down(&receive_priority_task_sema); 
                } 
              
              else  {    /*for low priority tasks*/
                if(task.direction == SENDER)     
                  sema_down(&send_lowPriority_task_sema);  
                else 
                  sema_down(&receive_lowPriority_task_sema);
              } 


              lock_release(&mutex_bus);

              break;   /*Exits out of the while loop*/

            }
          }

        /*Exiting the critical section*/
        
        lock_release(&mutex_bus);

        //Allow other threads to execute
	timer_sleep((int64_t)((unsigned int)random_ulong()%10));
      
      
      }
}

/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
timer_sleep((int64_t)((unsigned int)random_ulong()%10));
}

/* task releases the slot */
void leaveSlot(task_t task) 
{
     /*increment available bus slots*/
    sema_up(&bus_size);
}
