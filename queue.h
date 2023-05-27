#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

// Structure for queue
typedef struct queue {
    int numbers;
    int capacity;
    int* array;
    int next;
    int previous;
} queue;

// Queue
struct queue* initQueue(unsigned capacity)
{
    struct queue* queue = (struct queue*)mmap(NULL, sizeof(struct queue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    queue->capacity = capacity;
    queue->next = queue->numbers = 0;
    queue->previous = capacity - 1;
    queue->array = (int*)mmap(NULL, queue->capacity * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    return queue;
}

// Queue is full when size = capacity
int isFull(struct queue* queue)
{
    return (queue->numbers == queue->capacity);
}

// Queue is empty when size = 0
int isEmpty(struct queue* queue)
{
    return (queue->numbers == 0);
}

// Function adds an item to the queue.
// It affects rear and size
void enqueue(struct queue* queue, int item)
{
    if (isFull(queue))
        return;
    queue->previous = (queue->previous + 1)
                      % queue->capacity;
    queue->array[queue->previous] = item;
    queue->numbers = queue->numbers + 1;
}

// Function removes an item from queue.
// It affects front and size
int dequeue(struct queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;

    int item = queue->array[queue->next];
    queue->next = (queue->next + 1) % queue->capacity;
    queue->numbers = queue->numbers - 1;
    return item;
}

// Function gets front of queue
int next(struct queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->next];
}

// Function gets rear of queue
int rear(struct queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->previous];
}

sem_t *mutex_clerk, *mutex_customer, *nextcustomer, *sem_print; // Semaphores
int *counter; // Variables
FILE *f_out; // Output file
queue **queues;
bool *isClosed = false;

void init_sem() {
    mutex_clerk = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    mutex_customer = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_print = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    counter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    queues = mmap(NULL, sizeof(queue*) * 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    isClosed = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    nextcustomer = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    *counter = 1;

    sem_init(mutex_clerk, 1, 1);
    sem_init(mutex_customer, 1, 1);
    sem_init(sem_print, 1, 1);
    sem_init(nextcustomer, 1, 1);

}

void cleanup() {
    // Destroy semaphores
    sem_destroy(mutex_clerk);
    sem_destroy(mutex_customer);
    sem_destroy(sem_print);

    // Unmap shared memory
    munmap(mutex_clerk, sizeof(sem_t));
    munmap(mutex_customer, sizeof(sem_t));
    munmap(sem_print, sizeof(sem_t));
    munmap(counter, sizeof(int));
    munmap(isClosed, sizeof(bool));
    munmap(nextcustomer, sizeof(sem_t));

    // Unmap and free the queues
    for (int i = 0; i < 4; i++) {
        munmap(queues[i]->array, queues[i]->capacity * sizeof(int));
        munmap(queues[i], sizeof(struct queue));
    }

    munmap(queues, sizeof(queue*) * 4);

    // Close the output file
    fclose(f_out);
}

void print_msg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    sem_wait(sem_print);
    fprintf(f_out, "%d: ", *counter);
    *counter += 1;
    vfprintf(f_out, format, args);
    fprintf(f_out, "\n");
    fflush(f_out);
    sem_post(sem_print);
    va_end(args);
}
