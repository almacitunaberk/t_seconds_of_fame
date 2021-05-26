#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#define MAX_NUMBER_OF_THREADS = 1000

pthread_t threads[MAX_NUMBER_OF_THREADS] = {0};

float p;
int q;
int n;
float t;
int number_of_commentators;
int index_of_commentator;
int number_of_decided_commentators;
int number_of_wanting_to_answer_commentators;
pthread_cond_t permission_to_speak_line[MAX_NUMBER_OF_THREADS-1];

struct Queue* answering_queue;

int pthread_sleep(double seconds);

pthread_mutex_t general_mutex;
pthread_cond_t question_is_asked;
pthread_cond_t commentator_done_speaking;
pthread_cond_t commentators_decided;


void* moderator(void *args) {
    for (int i=0; i<q; i++) {
        number_of_decided_commentators = 0;
        number_of_wanting_to_answer_commentators = 0;
        pthread_mutex_lock(&general_mutex);
        pthread_cond_broadcast(&question_is_asked);
        pthread_cond_wait(&commentators_decided, &general_mutex);
        for (int j=0; j<number_of_wanting_to_answer_commentators; j++) {
            int commentatorID = dequeue(answering_queue);
            pthread_cond_signal(&permission_to_speak_line[commentatorID]);
            pthread_cond_wait(&commentator_done_speaking, &general_mutex);
        }
        
    }
}

void* commentator(int commentatorID) {
    for (int i=0; i<q; i++) {
        pthread_mutex_lock(&general_mutex);
        pthread_cond_wait(&question_is_asked, &general_mutex);
        will_i_answer = decide_to_answer_or_not();
        if (will_i_answer) {
            enqueue(answering_queue, commentatorID);
            number_of_wanting_to_answer_commentators++;
        }
        number_of_decided_commentators++;
        
        if (number_of_decided_commentators == number_of_commentators) {
            pthread_cond_signal(&commentators_decided);
        }
        
        if (will_i_answer) {
            pthread_cond_wait(&permission_to_speak_line[commentatorID], &general_mutex);
            float speaking_time = calculate_speaking_time();
            pthread_sleep(speaking_time);
            pthread_cond_signal(&commentator_done_speaking);
        }
        
        pthread_mutex_unlock(&general_mutex);
    }
}

int main() {
    answering_queue = createQueue(number_of_commentators);
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

struct Queue {
    int front, rear, size;
    unsigned capacity;
    int* array;
};
 
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
    printf("%d enqueued to queue\n", item);
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
