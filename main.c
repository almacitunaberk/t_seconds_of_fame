#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <semaphore.h>
#include <unistd.h>
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
double calculate_speaking_time();
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

int utc_system_timestamp(char[], float);
int utc_system_timestamp_now(char[]);

/*
struct timespec timeToWait;
            struct timeval now;            
            gettimeofday(&now,NULL);
            timeToWait.tv_sec = now.tv_sec+speaking_time; 
            timeToWait.tv_nsec = now.tv_usec * 1000;
            char buffer[31];
            utc_system_timestamp(buffer);

*/

// Allocate exactly 31 bytes for buf
int utc_system_timestamp(char buf[], float speaking_time) {
    const int bufsize = 31;
    const int tmpsize = 21;
    struct timespec now;
    struct timespec timeToWait;
    struct tm tm;
    int retval = clock_gettime(CLOCK_REALTIME, &now);
    timeToWait.tv_sec = now.tv_sec + speaking_time;
    timeToWait.tv_nsec = now.tv_nsec * 1000;
    gmtime_r(&timeToWait.tv_sec, &tm);
    strftime(buf, tmpsize, "%Y-%m-%dT%H:%M:%S.", &tm);
    sprintf(buf + tmpsize -1, "%09luZ", timeToWait.tv_nsec);
    return retval;
}


int utc_system_timestamp_now(char buf[]) {
    const int bufsize = 31;
    const int tmpsize = 21;
    struct timespec now;
    struct tm tm;
    int retval = clock_gettime(CLOCK_REALTIME, &now);
    gmtime_r(&now.tv_sec, &tm);
    strftime(buf, tmpsize, "%Y-%m-%dT%H:%M:%S.", &tm);
    sprintf(buf + tmpsize -1, "%09luZ", now.tv_nsec);
    return retval;
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
bool is_breaking_news_event_happening_array[MAX_QUESTIONS] = { false };
pthread_cond_t breaking_news_event_happening_array[MAX_QUESTIONS];
pthread_mutex_t breaking_news_event_happening_mutex_array[MAX_QUESTIONS];

// Breaking news event ending
bool is_breaking_news_event_ended_array[MAX_QUESTIONS] = { false };
pthread_cond_t breaking_news_event_ending_array[MAX_QUESTIONS];
pthread_mutex_t breaking_news_event_ending_mutex_array[MAX_QUESTIONS];

// Breaking news event happening single
bool is_breaking_news_event_happening = false;
pthread_cond_t breaking_news_event_happening;
pthread_mutex_t breaking_news_event_happening_mutex;

// Breaking news event ending single
bool is_breaking_news_event_ended = false;
pthread_cond_t breaking_news_event_ended;
pthread_mutex_t breaking_news_event_ended_mutex;

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
        pthread_mutex_lock(&answering_queue_mutex);        
        while(answering_queue->size > 0) {            
            pthread_mutex_lock(&breaking_news_event_happening_mutex);
            if (!is_breaking_news_event_happening) {
                // printf("%s In Moderator, breaking news event NOT happening\n", gettimestamp());
                pthread_mutex_unlock(&breaking_news_event_happening_mutex);
                int commentatorID = dequeue(answering_queue);
                pthread_mutex_lock(&permission_to_speak_mutex[commentatorID]);
                may_i_speak[commentatorID] = true;
                // printf("%s Moderator gives permission to Commentator #%d to speak for the question #%d\n", gettimestamp(), commentatorID, i+1);
                pthread_mutex_unlock(&permission_to_speak_mutex[commentatorID]);
                pthread_cond_signal(&permission_to_speak[commentatorID]);
                pthread_mutex_lock(&done_speaking_mutex[commentatorID]);
                while(!finished_speaking[commentatorID]) {
                    pthread_cond_wait(&done_speaking[commentatorID], &done_speaking_mutex[commentatorID]);
                }
                finished_speaking[commentatorID] = false;                    
                pthread_mutex_unlock(&done_speaking_mutex[commentatorID]);
            } else {
                // printf("%s In Moderator, breaking news event happening before giving any permission!!\n", gettimestamp());
                pthread_mutex_unlock(&breaking_news_event_happening_mutex);
                pthread_mutex_lock(&breaking_news_event_ended_mutex);
                // printf("%s Waiting for the breaking news event to finish before giving any permission\n", gettimestamp());
                while(!is_breaking_news_event_ended) {                    
                    pthread_cond_wait(&breaking_news_event_ended, &breaking_news_event_ended_mutex);
                }
                pthread_mutex_unlock(&breaking_news_event_ended_mutex);
            }
            
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
            // printf("%s All commentators decided to answer or not for question #%d\n", gettimestamp(), i+1);
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
            pthread_mutex_lock(&breaking_news_event_happening_mutex);
            double speaking_time = calculate_speaking_time();    
            struct timeval tp;
            struct timespec timetoexpire;
            gettimeofday(&tp, NULL);
            long new_nsec = tp.tv_usec * 1000 + (speaking_time - (long)speaking_time) * 1e9;
            timetoexpire.tv_sec = tp.tv_sec + (long)speaking_time + (new_nsec / (long)1e9);
            timetoexpire.tv_nsec = new_nsec % (long)1e9;
            printf("%s Commentator #%d 's turn to speak for %f seconds for question #%d\n", gettimestamp(), commentatorID, speaking_time, i+1);                                                               
            int res = pthread_cond_timedwait(&breaking_news_event_happening, &breaking_news_event_happening_mutex, &timetoexpire);            
            if (res == 0) {
                printf("%s Commentator #%d is cut short due to a breaking news\n", gettimestamp(), commentatorID);
            } else {
                printf("%s Commentator #%d finished speaking\n", gettimestamp(), commentatorID);
            }            
            pthread_mutex_unlock(&breaking_news_event_happening_mutex);
            pthread_sleep(speaking_time);
            pthread_mutex_lock(&done_speaking_mutex[commentatorID]);
            finished_speaking[commentatorID] = true;
            pthread_mutex_unlock(&done_speaking_mutex[commentatorID]);            
            pthread_cond_signal(&done_speaking[commentatorID]);
        }        
    }
}

void *breaking_news_event_listener(void *args) {
    while(true) {
        pthread_mutex_lock(&breaking_news_event_happening_mutex);
        while(!is_breaking_news_event_happening) {
            pthread_cond_wait(&breaking_news_event_happening, &breaking_news_event_happening_mutex);            
        }
        printf("%s !!! Breaking news event happening !!!\n", gettimestamp());
        pthread_mutex_unlock(&breaking_news_event_happening_mutex);
        pthread_mutex_lock(&breaking_news_event_ended_mutex);
        while(!is_breaking_news_event_ended) {
            pthread_cond_wait(&breaking_news_event_ended, &breaking_news_event_ended_mutex);            
        }
        printf("%s Breaking news event ended\n", gettimestamp());
        pthread_mutex_unlock(&breaking_news_event_ended_mutex);        
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
    b = 0.2;
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
        
        //pthread_cond_init(&breaking_news_event_happening[i], NULL);
        //pthread_mutex_init(&breaking_news_event_happening_mutex[i], NULL);

        //pthread_cond_init(&breaking_news_event_ending[i], NULL);
        //pthread_mutex_init(&breaking_news_event_ending_mutex[i], NULL);
    } 

    pthread_cond_init(&breaking_news_event_happening, NULL);
    pthread_mutex_init(&breaking_news_event_happening_mutex, NULL);

    pthread_cond_init(&breaking_news_event_ended, NULL);
    pthread_mutex_init(&breaking_news_event_ended_mutex, NULL);

    pthread_cond_init(&is_program_over, NULL);
    pthread_mutex_init(&is_program_over_mutex, NULL);

    pthread_mutex_init(&deciding_process_mutex, NULL);
    pthread_mutex_init(&wanting_to_answer_mutex, NULL);
    pthread_mutex_init(&answering_queue_mutex, NULL);

    printf("%s After every mutex and cond is initialized with default NULL\n", gettimestamp());
    printf("%s Starting thread creation\n", gettimestamp());
    
    for (int i = 0; i < number_of_commentators + 2; i++)
    {
        if (i == 0) {
            if (pthread_create(&threads[0], NULL, moderator, NULL) != 0) {
                perror("Failed to create the moderator thread\n");
            }
        } else if (i != number_of_commentators + 1) { 
            if ((pthread_create(&threads[i], NULL, commentator, (void *)(long)i) != 0)) {
                printf("Failed to create the commentator thread with ID = %d\n", i);
            }
        } else {
            if ((pthread_create(&threads[i], NULL, breaking_news_event_listener, NULL) != 0)) {
                printf("Failed to create the breaking news event listener thread\n");
            }
        }
    }        
    printf("%s Every thread is created\n", gettimestamp());

    while(true) {        
        pthread_mutex_lock(&is_program_over_mutex);
        if (is_questions_over) {
            break;
        }
        pthread_mutex_unlock(&is_program_over_mutex);
        pthread_mutex_lock(&breaking_news_event_happening_mutex);
        is_breaking_news_event_happening = calculate_breaking_news_event();   
        if (is_breaking_news_event_happening) {
            // printf("%s Breaking news event happening in Main thread\n", gettimestamp());
            pthread_mutex_lock(&breaking_news_event_ended_mutex);
            is_breaking_news_event_ended = false;
            pthread_mutex_unlock(&breaking_news_event_ended_mutex);            
            // printf("%s in Main breaking news event happening is triggered\n", gettimestamp());
            pthread_mutex_unlock(&breaking_news_event_happening_mutex);
            pthread_cond_broadcast(&breaking_news_event_happening);        
            pthread_sleep(5);
            pthread_mutex_lock(&breaking_news_event_ended_mutex);
            pthread_mutex_lock(&breaking_news_event_happening_mutex);
            is_breaking_news_event_happening = false;
            is_breaking_news_event_ended = true;
            pthread_mutex_unlock(&breaking_news_event_happening_mutex);            
            // printf("%s in Main, after waited 5 seconds for the breaking news event\n", gettimestamp());
            
            
            pthread_mutex_unlock(&breaking_news_event_ended_mutex);
            pthread_cond_broadcast(&breaking_news_event_ended);
            // printf("%s in Main, breaking news event is ended\n", gettimestamp());
        } else {
            pthread_mutex_unlock(&breaking_news_event_happening_mutex);
            // printf("%s NO BREAKING EVENT\n", gettimestamp());
        }   
        pthread_sleep(1);              
    }
    pthread_mutex_unlock(&is_program_over_mutex); 
    printf("Moderator asked all questions. Preparing to terminate\n");
    printf("Main pthread join\n");    
    for (int i = 0; i < number_of_commentators + 1; i++) {
        pthread_join(threads[i], NULL);
    }        
    pthread_cancel(threads[number_of_commentators+1]);
    printf("Program is over\n");
    
}


// Random number generation between 0 and 1 is taken from Stackoverflow
// the link is:
//   https://stackoverflow.com/questions/6218399/how-to-generate-a-random-number-between-0-and-1/6219525

bool decide_to_answer_or_not() {
    float random_probability = (float)random() / (float)RAND_MAX;
    if (random_probability < p) {
        return true;
    } else {
        return false;
    }
}


// Random float generation between 1 and t is taken from Stackoverflow
// the link is
//   https://stackoverflow.com/questions/17846212/generate-a-random-number-between-1-and-10-in-c
double calculate_speaking_time() {
    double speak_time = (double)random() / (double)(RAND_MAX / t);
    return speak_time;
}

bool calculate_breaking_news_event() {
    float random_probability = (float)random() / (float)RAND_MAX;
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
