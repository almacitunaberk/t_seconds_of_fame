#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <semaphore.h>
#define MAX_NUMBER_OF_THREADS 1000


// Threads array for moderator and commentators
pthread_t threads[MAX_NUMBER_OF_THREADS] = {0};
//---------------------------------------------

// Bool type def
typedef enum {false, true} bool;
//---------------------------------------------

// Queue struct
struct Queue {
    int front, rear, size;
    unsigned capacity;
    int* array;
};
struct Queue* answering_queue;
void enqueue(struct Queue*, int);
int dequeue(struct Queue*);
int front(struct Queue*);
struct Queue* createQueue(unsigned);
//---------------------------------------------

// Global variables
float p;
int q;
float t;
int number_of_commentators;
struct timeval start_time;
//---------------------------------------------

// Functions
int pthread_sleep(double seconds);
bool decide_to_answer_or_not();
float calculate_speaking_time();
//---------------------------------------------


// Cond_T and corresponding Mutex_T's and Variables

// Moderator asking the question
bool did_moderator_ask_the_question = false;
pthread_cond_t moderator_asked_the_question;
pthread_mutex_t moderator_asked_the_question_mutex;

// Commentators trying to decide whether or not to answer
bool has_all_commentators_decided = false;
pthread_cond_t all_commentators_decided;
pthread_mutex_t all_commentators_decided_mutex;

// Permission to speak for commentator with ID
bool may_i_speak[MAX_NUMBER_OF_THREADS-1] = {false};
pthread_cond_t permission_to_speak[MAX_NUMBER_OF_THREADS-1];
pthread_mutex_t permission_to_speak_mutex[MAX_NUMBER_OF_THREADS-1];

// Commentator with ID finished speaking
bool finished_speaking[MAX_NUMBER_OF_THREADS-1] = {false};
pthread_cond_t done_speaking[MAX_NUMBER_OF_THREADS-1];
pthread_cond_t done_speaking_mutex[MAX_NUMBER_OF_THREADS-1];

// The moderator has asked all questions
bool is_questions_over = false;
pthread_cond_t is_program_over;
pthread_mutex_t is_program_over_mutex;

// Commentator deciding process
int number_of_decided_commentators;
pthread_mutex_t deciding_process_mutex;

// Commentator wanting to answer
int number_of_wanting_to_answer_commentators;
pthread_mutex_t wanting_to_answer_mutex;

// Commentators answering queue
struct Queue* answering_queue;
pthread_mutex_t answering_queue_mutex;
//---------------------------------------------




void *moderator(void *args) {
    for (int i=0; i<q; i++) {
        pthread_mutex_lock(&moderator_asked_the_question_mutex);
        did_moderator_ask_the_question = true;  
        pthread_mutex_lock(&deciding_process_mutex);
        pthread_mutex_lock(&wanting_to_answer_mutex);
        number_of_decided_commentators = 0;
        number_of_wanting_to_answer_commentators = 0; 
        pthread_mutex_unlock(&wanting_to_answer_mutex);
        pthread_mutex_unlock(&deciding_process_mutex);
        printf("--------------------------------------\n");
        printf("Question Number %d\n", i+1);
        printf("Moderator asks question %d\n", i+1);
        pthread_mutex_unlock(&moderator_asked_the_question_mutex);
        pthread_cond_broadcast(&moderator_asked_the_question);
        pthread_mutex_lock(&all_commentators_decided_mutex);
        while(!has_all_commentators_decided) {
            pthread_cond_wait(&all_commentators_decided, &all_commentators_decided_mutex);
        }
        pthread_mutex_unlock(&all_commentators_decided_mutex);
        /* pthread_mutex_lock(&deciding_process_mutex);        
        pthread_mutex_lock(&wanting_to_answer_mutex);
        printf("Number of commentators decided is %d\n", number_of_decided_commentators);
        printf("Number of commentators wanting to answer is %d\n", number_of_wanting_to_answer_commentators);
        pthread_mutex_unlock(&wanting_to_answer_mutex);
        pthread_mutex_unlock(&deciding_process_mutex);
        */
        pthread_mutex_lock(&answering_queue_mutex);
        while(answering_queue->size > 0) {
            printf("Number of commentators waiting to speak for the question #%d is %d\n", i+1, answering_queue->size);
            int commentatorID = dequeue(answering_queue);
            pthread_mutex_lock(&permission_to_speak_mutex[commentatorID]);
            may_i_speak[commentatorID] = true;
            printf("Moderator gives permission to Commentator #%d to speak for the question #%d\n", commentatorID, i+1);
            pthread_mutex_unlock(&permission_to_speak_mutex[commentatorID]);
            pthread_cond_signal(&permission_to_speak[commentatorID]);
            pthread_mutex_lock(&done_speaking_mutex[commentatorID]);
            while(!finished_speaking[commentatorID]) {
                pthread_cond_wait(&done_speaking[commentatorID], &done_speaking_mutex[commentatorID]);
            }                    
            pthread_mutex_unlock(&done_speaking_mutex[commentatorID]);
        }
        pthread_mutex_unlock(&answering_queue_mutex);
    }
    pthread_mutex_lock(&is_program_over_mutex);
    is_questions_over = true;
    pthread_mutex_unlock(&is_program_over_mutex);
    pthread_cond_signal(&is_program_over);
}

void *commentator(int commentatorID) {
    for (int i=0; i<q; i++) {
        pthread_mutex_lock(&moderator_asked_the_question_mutex);
        while(!did_moderator_ask_the_question) {
            pthread_cond_wait(&moderator_asked_the_question, &moderator_asked_the_question_mutex);            
        }
        pthread_mutex_unlock(&moderator_asked_the_question_mutex);
        bool will_i_answer = decide_to_answer_or_not();        
        pthread_mutex_lock(&deciding_process_mutex);
        number_of_decided_commentators++;
        if (will_i_answer) {  
            pthread_mutex_lock(&answering_queue_mutex);
            enqueue(answering_queue, commentatorID);
            pthread_mutex_unlock(&answering_queue_mutex);
            pthread_mutex_lock(&wanting_to_answer_mutex);
            number_of_wanting_to_answer_commentators++;                                    
            printf(" Commentator #%d generates answer for question #%d, position in queue: %d\n", commentatorID, i+1, number_of_wanting_to_answer_commentators);            
        } else {
            printf("Commentator with ID %d does not want to answer \n", commentatorID);            
        }        
        if (number_of_decided_commentators == number_of_commentators) {            
            printf("All commentators decided to answer or not for question #%d\n", i+1);
            pthread_mutex_lock(&all_commentators_decided_mutex);
            has_all_commentators_decided = true;
            pthread_mutex_unlock(&all_commentators_decided_mutex);
            pthread_cond_signal(&all_commentators_decided);
        }
        pthread_mutex_unlock(&deciding_process_mutex);
        pthread_mutex_unlock(&wanting_to_answer_mutex);
        if (will_i_answer) {
            pthread_mutex_lock(&permission_to_speak_mutex[commentatorID]);
            while(!may_i_speak[commentatorID]) {
                pthread_cond_wait(&permission_to_speak[commentatorID], &permission_to_speak_mutex[commentatorID]);                
            }
            pthread_mutex_unlock(&permission_to_speak_mutex[commentatorID]);
            float speaking_time = calculate_speaking_time();                       
            printf(" Commentator #%d 's turn to speak for %f seconds for question #%d\n", commentatorID, speaking_time, i+1);
            pthread_sleep(speaking_time);
            pthread_mutex_lock(&done_speaking_mutex[commentatorID]);
            finished_speaking[commentatorID] = true;
            pthread_mutex_unlock(&done_speaking_mutex[commentatorID]);
            pthread_cond_signal(&done_speaking[commentatorID]);
        }
    }
}

int main() {
    printf("------------------------------------\n");
    printf("Starting program\n");    
    // Global variable initializer
    p = 1.0;
    q = 5;
    number_of_commentators = 4;
    t = 3;
    answering_queue = createQueue(number_of_commentators);
    gettimeofday(&start_time, NULL);
    // Cond_T, Mutex_T initializer and corresponding variable initializer
    pthread_cond_init(&moderator_asked_the_question, NULL);
    pthread_mutex_init(&moderator_asked_the_question_mutex, NULL);

    pthread_cond_init(&all_commentators_decided, NULL);
    pthread_mutex_init(&all_commentators_decided_mutex, NULL);

    for (int i=0; i<number_of_commentators + 1; i++) {
        pthread_cond_init(&permission_to_speak[i], NULL);
        pthread_mutex_init(&permission_to_speak_mutex[i], NULL);

        pthread_cond_init(&done_speaking[i], NULL);
        pthread_mutex_init(&done_speaking_mutex[i], NULL);
    }

    pthread_cond_init(&is_program_over, NULL);
    pthread_mutex_init(&is_program_over_mutex, NULL);

    pthread_mutex_init(&deciding_process_mutex, NULL);
    pthread_mutex_init(&wanting_to_answer_mutex, NULL);
    pthread_mutex_init(&answering_queue_mutex, NULL);

    printf("After every mutex and cond is initialized with default NULL\n");
    printf("Starting thread creation\n");
    if (pthread_create(&threads[0], NULL, moderator, NULL) != 0) {
        perror("Failed to create the moderator thread\n");
    }
    for (int i = 1; i < number_of_commentators + 1; i++)
    {
        if (pthread_create(&threads[i], NULL, commentator, (void *)(long)i) != 0) {
            printf("Failed to create the commentator thread with ID = %d\n", i);
        }
    }    
    printf("After every thread is created\n");
    pthread_mutex_lock(&is_program_over_mutex);
    while (!is_questions_over) {
        pthread_cond_wait(&is_program_over, &is_program_over_mutex);
    }
    printf("Moderator asked all questions. Preparing to terminate\n");
    pthread_mutex_unlock(&is_program_over_mutex);
    printf("Main pthread join\n");    
    for (int i = 0; i <= number_of_commentators + 1; i++) {
        pthread_join(threads[i], NULL);
    }        
    printf("Program is over\n");
}


// Random number generation between 0 and 1 is taken from Stackoverflow
// the link is:
//   https://stackoverflow.com/questions/6218399/how-to-generate-a-random-number-between-0-and-1/6219525

bool decide_to_answer_or_not() {
    float random_probability = random() / RAND_MAX;
    if (random_probability < p) {
        return true;
    } else {
        return false;
    }
}


// Random float generation between 1 and t is taken from Stackoverflow
// the link is
//   https://stackoverflow.com/questions/17846212/generate-a-random-number-between-1-and-10-in-c
float calculate_speaking_time() {
    float speak_time = (float)random() / (float)(RAND_MAX / t);
    return speak_time;
}

/**
 * pthread_sleep takes an integer number of seconds to pause the current thread
 * original by Yingwu Zhu
 * updated by Muhammed Nufail Farooqi
 * updated by Fahrican Kosar
 */
int pthread_sleep(double seconds){
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    if(pthread_mutex_init(&mutex,NULL)){
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL)){
        return -1;
    }

    struct timeval tp;
    struct timespec timetoexpire;
    // When to expire is an absolute time, so get the current time and add
    // it to our delay time
    gettimeofday(&tp, NULL);
    long new_nsec = tp.tv_usec * 1000 + (seconds - (long)seconds) * 1e9;
    timetoexpire.tv_sec = tp.tv_sec + (long)seconds + (new_nsec / (long)1e9);
    timetoexpire.tv_nsec = new_nsec % (long)1e9;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;
}
 
// function to create a queue
// of given capacity.
// It initializes size of queue as 0
struct Queue* createQueue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*)malloc(
        sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
 
    // This is important, see the enqueue
    queue->rear = capacity - 1;
    queue->array = (int*)malloc(
        queue->capacity * sizeof(int));
    return queue;
}
 
// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue* queue)
{
    return (queue->size == queue->capacity);
}
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
    return (queue->size == 0);
}
 
// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)
                  % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;    
}
 
// Function to remove an item from queue.
// It changes front and size
int dequeue(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1)
                   % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}
 
// Function to get front of queue
int front(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->front];
}
 
// Function to get rear of queue
int rear(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->rear];
}
