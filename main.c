#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <semaphore.h>
#define MAX_NUMBER_OF_THREADS 1000
#define MAX_QUESTIONS 1000


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
float b;
int number_of_commentators;
struct timeval start_time;
char currentTime[12];
int current_question = 0;
//---------------------------------------------

// Functions
int pthread_sleep(double seconds);
bool decide_to_answer_or_not();
float calculate_speaking_time();
bool calculate_breaking_news_event();
char *gettimestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    int m = (tp.tv_sec - start_time.tv_sec) / 60;
    int s = tp.tv_sec - start_time.tv_sec - m * 60;
    int ms = tp.tv_usec / 1000;
    if (ms < 0)
        ms += 1000;
    sprintf(currentTime, "[%02d:%02d.%03d]", m, s, ms);
    return currentTime;
}
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

// Checking the current question
int current_question;
pthread_mutex_t current_question_mutex;

// Next question
bool is_next_question_coming;
pthread_cond_t next_question_will_be_asked;
pthread_mutex_t next_question_will_be_asked_mutex;

// Question array
bool is_this_question_asked[MAX_QUESTIONS] = { false } ;
pthread_mutex_t question_asked_mutex[MAX_QUESTIONS];
pthread_cond_t question_asked[MAX_QUESTIONS];

// Commentators Decision Array
bool has_all_commentators_decided_array[MAX_QUESTIONS] = { false };
pthread_cond_t all_commentators_decided_array[MAX_QUESTIONS];
pthread_mutex_t all_commentators_decided_mutex_array[MAX_QUESTIONS];

// Breaking news event happening
bool is_breaking_news_event_happening[MAX_QUESTIONS] = { false };
pthread_cond_t breaking_news_event_happening[MAX_QUESTIONS];
pthread_mutex_t breaking_news_event_happening_mutex[MAX_QUESTIONS];

// Breaking news event ending
bool is_breaking_news_event_ended[MAX_QUESTIONS] = { false };
pthread_cond_t breaking_news_event_ending[MAX_QUESTIONS];
pthread_mutex_t breaking_news_event_ending_mutex[MAX_QUESTIONS];
//---------------------------------------------

void *moderator(void *args) {
    for (int i=0; i<q; i++) {        
        printf("--------------------------------------\n");
        printf("Question Number %d\n", i+1);        
        current_question = i;
        pthread_mutex_lock(&question_asked_mutex[i]);        
        is_this_question_asked[i] = true;
        pthread_mutex_lock(&deciding_process_mutex);
        pthread_mutex_lock(&wanting_to_answer_mutex);
        number_of_decided_commentators = 0;
        number_of_wanting_to_answer_commentators = 0; 
        pthread_mutex_unlock(&wanting_to_answer_mutex);
        pthread_mutex_unlock(&deciding_process_mutex);        
        printf("%s Moderator asks question %d\n", gettimestamp(), i+1);
        pthread_mutex_unlock(&question_asked_mutex[i]);
        pthread_cond_broadcast(&question_asked[i]);
        pthread_mutex_lock(&all_commentators_decided_mutex_array[i]);        
        while(!has_all_commentators_decided_array[i]) {
            pthread_cond_wait(&all_commentators_decided_array[i], &all_commentators_decided_mutex_array[i]);
        }               
        pthread_mutex_unlock(&all_commentators_decided_mutex_array[i]);              
        /* pthread_mutex_lock(&deciding_process_mutex);        
        pthread_mutex_lock(&wanting_to_answer_mutex);
        printf("Number of commentators decided is %d\n", number_of_decided_commentators);
        printf("Number of commentators wanting to answer is %d\n", number_of_wanting_to_answer_commentators);
        pthread_mutex_unlock(&wanting_to_answer_mutex);
        pthread_mutex_unlock(&deciding_process_mutex);
        */        
        pthread_mutex_lock(&answering_queue_mutex);        
        while(answering_queue->size > 0) {
            // printf("%s Number of commentators waiting to speak for the question #%d is %d\n", gettimestamp(), i+1, answering_queue->size);
            int commentatorID = dequeue(answering_queue);
            pthread_mutex_lock(&permission_to_speak_mutex[commentatorID]);
            may_i_speak[commentatorID] = true;
            printf("%s Moderator gives permission to Commentator #%d to speak for the question #%d\n", gettimestamp(), commentatorID, i+1);
            pthread_mutex_unlock(&permission_to_speak_mutex[commentatorID]);
            pthread_cond_signal(&permission_to_speak[commentatorID]);
            pthread_mutex_lock(&done_speaking_mutex[commentatorID]);
            while(!finished_speaking[commentatorID]) {
                pthread_cond_wait(&done_speaking[commentatorID], &done_speaking_mutex[commentatorID]);
            }
            finished_speaking[commentatorID] = false;                    
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
        pthread_mutex_lock(&question_asked_mutex[i]);
        while(!is_this_question_asked[i]) {
            pthread_cond_wait(&question_asked[i], &question_asked_mutex[i]);            
        }        
        pthread_mutex_unlock(&question_asked_mutex[i]);
        pthread_mutex_lock(&all_commentators_decided_mutex);
        has_all_commentators_decided = false;
        pthread_mutex_unlock(&all_commentators_decided_mutex);
        pthread_cond_signal(&all_commentators_decided);
        bool will_i_answer = decide_to_answer_or_not();        
        pthread_mutex_lock(&deciding_process_mutex);
        number_of_decided_commentators++;
        if (will_i_answer) {  
            pthread_mutex_lock(&answering_queue_mutex);
            enqueue(answering_queue, commentatorID);
            pthread_mutex_unlock(&answering_queue_mutex);
            pthread_mutex_lock(&wanting_to_answer_mutex);
            number_of_wanting_to_answer_commentators++;                                    
            printf("%s Commentator #%d generates answer for question #%d, position in queue: %d\n", gettimestamp(), commentatorID, i+1, number_of_wanting_to_answer_commentators);            
        } else {
            printf("%s Commentator with ID %d does not want to answer \n", gettimestamp(), commentatorID);            
        }               
        if (number_of_decided_commentators == number_of_commentators) {            
            printf("%s All commentators decided to answer or not for question #%d\n", gettimestamp(), i+1);
            pthread_mutex_lock(&all_commentators_decided_mutex_array[i]);
            has_all_commentators_decided_array[i] = true;
            pthread_mutex_unlock(&all_commentators_decided_mutex_array[i]);                  
            pthread_cond_signal(&all_commentators_decided_array[i]);            
        }
        pthread_mutex_unlock(&deciding_process_mutex);
        pthread_mutex_unlock(&wanting_to_answer_mutex);         
        if (will_i_answer) {
            pthread_mutex_lock(&permission_to_speak_mutex[commentatorID]);
            while(!may_i_speak[commentatorID]) {
                pthread_cond_wait(&permission_to_speak[commentatorID], &permission_to_speak_mutex[commentatorID]);                
            }
            may_i_speak[commentatorID] = false;
            pthread_mutex_unlock(&permission_to_speak_mutex[commentatorID]);
            float speaking_time = calculate_speaking_time();                       
            printf("%s Commentator #%d 's turn to speak for %f seconds for question #%d\n", gettimestamp(), commentatorID, speaking_time, i+1);
            /* pthread_mutex_lock(&breaking_news_event_happening_mutex[i]);
            while (!is_breaking_news_event_happening[i]) {
                struct timespec timeToWait;
                struct timeval now;
                int rt;
                gettimeofday(&now,NULL);
                timeToWait.tv_sec = now.tv_sec+5;
                timeToWait.tv_nsec = (now.tv_usec+1000UL*timeInMs)*1000UL;                
                rt = pthread_cond_timedwait(&breaking_news_event_happening[i], &breaking_news_event_happening_mutex[i], &timeToWait);
                if (rt == 0) {
                    pthread_mutex_lock(&done_speaking_mutex[commentatorID]);
                    finished_speaking[commentatorID] = true;
                    pthread_mutex_unlock(&done_speaking_mutex[commentatorID]);            
                    pthread_cond_signal(&done_speaking[commentatorID]);
                }                             
            }
            */
            // pthread_mutex_unlock(&breaking_news_event_happening_mutex[i]);         
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
    current_question = -1;
    gettimeofday(&start_time, NULL);
    // Cond_T, Mutex_T initializer and corresponding variable initializer
    pthread_cond_init(&moderator_asked_the_question, NULL);
    pthread_mutex_init(&current_question_mutex, NULL);

    pthread_cond_init(&all_commentators_decided, NULL);
    pthread_mutex_init(&all_commentators_decided_mutex, NULL);

    for (int i=0; i<number_of_commentators + 1; i++) {
        pthread_cond_init(&permission_to_speak[i], NULL);
        pthread_mutex_init(&permission_to_speak_mutex[i], NULL);

        pthread_cond_init(&done_speaking[i], NULL);
        pthread_mutex_init(&done_speaking_mutex[i], NULL);
    }

    for(int i=0; i<q; i++) {
        pthread_cond_init(&question_asked[i], NULL);
        pthread_mutex_init(&question_asked_mutex[i], NULL);
        
        pthread_cond_init(&all_commentators_decided_array[i], NULL);
        pthread_mutex_init(&all_commentators_decided_mutex_array[i], NULL);
        
        pthread_cond_init(&breaking_news_event_happening[i], NULL);
        pthread_mutex_init(&breaking_news_event_happening_mutex[i], NULL);

        pthread_cond_init(&breaking_news_event_ending[i], NULL);
        pthread_mutex_init(&breaking_news_event_ending_mutex[i], NULL);
    } 

    pthread_cond_init(&is_program_over, NULL);
    pthread_mutex_init(&is_program_over_mutex, NULL);

    pthread_mutex_init(&deciding_process_mutex, NULL);
    pthread_mutex_init(&wanting_to_answer_mutex, NULL);
    pthread_mutex_init(&answering_queue_mutex, NULL);

    printf("%s After every mutex and cond is initialized with default NULL\n", gettimestamp());
    printf("%s Starting thread creation\n", gettimestamp());
    
    for (int i = 0; i < number_of_commentators + 1; i++)
    {
        if (i == 0) {
            if (pthread_create(&threads[0], NULL, moderator, NULL) != 0) {
                perror("Failed to create the moderator thread\n");
            }
        } else { 
            if ((pthread_create(&threads[i], NULL, commentator, (void *)(long)i) != 0)) {
                printf("Failed to create the commentator thread with ID = %d\n", i);
            }
        }
    }    
    printf("%s Every thread is created\n", gettimestamp());
    pthread_mutex_lock(&is_program_over_mutex);
    while (!is_questions_over) {
        /* will_there_be_a_breaking_news_event = calculate_breaking_news_event();
        pthread_mutex_lock(&breaking_news_event_happening_mutex[current_question]);
        if (!is_breaking_news_event_happening[current_question]) {
            if (will_there_be_a_breaking_news_event) {
                printf("%s !!!Breaking event happened!!!", gettimestamp());
                pthread_mutex_lock(&breaking_news_event_ending_mutex[current_question]);
                while(!is_breaking_news_event_ended[current_question]) {
                    pthread_cond_wait(&breaking_news_event_ending[current_question], &breaking_news_event_ending_mutex[current_question]);
                }
                pthread_mutex_unlock(&breaking_news_event_ending_mutex[current_question]);
                pthread_cond_wait()
            }
        }
        */ 
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

bool calculate_breaking_news_event() {
    float random_probability = random() / RAND_MAX;
    if (random_probability < b) {
        return true;
    } else {
        return false;
    }
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
