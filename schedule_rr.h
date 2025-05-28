#ifndef SCHEDULE_RR_H
#define SCHEDULE_RR_H

#include "task.h" // Defines the Task structure
#include "list.h"   // Uses the list structure for the ready queue

extern struct node *rr_ready_queue;

void* timer_function(void* arg);

// Add a task to the RR ready queue
void rr_add(char *name, int priority, int burst);

// Invoke the RR scheduler
void rr_schedule();

#endif