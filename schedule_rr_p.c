// gcc -o rrp_scheduler driver.c schedule_rr_p.c list.c CPU.c -I. -Wall -g -pthread
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "schedule_rr_p.h"
#include "CPU.h"
#include "list.h"

struct node *priority_queues[MAX_PRIORITY_LEVELS] = {NULL}; // Definition

pthread_t g_timer_thread_id; // Definition (no extern)
volatile int g_time_slice_expired_flag = 0; // Definition with initialization
volatile int g_scheduler_is_active = 0;
volatile int g_timer_can_start_counting = 0;
volatile int g_task_completed_early = 0;
const int g_time_unit_us = 100000; 

// Timer thread's function
void* timer_function(void* arg) {
    printf("TIMER_THREAD: Started.\n");
    while (g_scheduler_is_active) { // Uses g_scheduler_is_active
        while (!g_timer_can_start_counting && g_scheduler_is_active) { // Uses g_timer_can_start_counting, g_scheduler_is_active
            usleep(10000);
        }

        if (!g_scheduler_is_active) { // Uses g_scheduler_is_active
            break;
        }

        g_time_slice_expired_flag = 0; // Uses g_time_slice_expired_flag
        g_task_completed_early = 0;    // Uses g_task_completed_early

        for (int i = 0; i < QUANTUM; ++i) { // QUANTUM is from CPU.h
            if (!g_scheduler_is_active || !g_timer_can_start_counting || g_task_completed_early) { // Uses all three
                break;
            }
            usleep(g_time_unit_us); // Uses g_time_unit_us
        }

        if (g_scheduler_is_active && g_timer_can_start_counting && !g_task_completed_early) { // Uses all three
            g_time_slice_expired_flag = 1; // Uses g_time_slice_expired_flag
        }
        g_timer_can_start_counting = 0; // Uses g_timer_can_start_counting
    }
    printf("TIMER_THREAD: Exiting.\n");
    return NULL;
}

void rr_p_add(char *name, int priority, int burst) {
    if (priority < 1 || priority > MAX_PRIORITY_LEVELS) {
        fprintf(stderr, "RR_P_ADD: Invalid priority %d for task %s. Clamping to nearest valid.\n", priority, name);
        if (priority < 1) priority = 1;
        if (priority > MAX_PRIORITY_LEVELS) priority = MAX_PRIORITY_LEVELS;
    }

    Task *newTask = (Task *)malloc(sizeof(Task));
    if (!newTask) {
        fprintf(stderr, "RR_P_ADD: Malloc failed for task %s\n", name);
        return;
    }
    newTask->name = strdup(name);
    if (!newTask->name) {
        fprintf(stderr, "RR_P_ADD: Strdup failed for task name %s\n", name);
        free(newTask);
        return;
    }
    newTask->priority = priority;
    newTask->burst = burst;

    // Add to the tail of the queue for the task's priority (adjust for 0-indexed array)
    insert_tail(&priority_queues[priority - 1], newTask);
    printf("RR_P: Added task [%s], Priority: %d, Burst: %d to priority queue %d.\n",
           name, priority, burst, priority);
}

// Helper function to check if all queues are empty
int all_queues_empty() {
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        if (priority_queues[i] != NULL) {
            return 0; // Found a non-empty queue
        }
    }
    return 1; // All queues are empty
}

void rr_p_schedule() {
   Task *current_task = NULL;
   int current_task_priority_level_idx = -1; // To know which queue to re-add to

   printf("RR_P_SCHEDULER: Starting RR with Priority Scheduler (Quantum: %d units).\n", QUANTUM);
   
   g_scheduler_is_active = 1;
   g_time_slice_expired_flag = 0;
   g_timer_can_start_counting = 0;
   g_task_completed_early = 0;
   if (pthread_create(&g_timer_thread_id, NULL, timer_function, NULL) != 0) {
      fprintf(stderr, "RR_P_SCHEDULER: Error creating timer thread. Exiting.\n");
      g_scheduler_is_active = 0;
      return;
   }

   while (!all_queues_empty()) {
      current_task = NULL; // Reset for each scheduling decision

      // Find the highest priority non-empty queue
      for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
         if (priority_queues[i] != NULL) {
               current_task = remove_head(&priority_queues[i]);
               current_task_priority_level_idx = i; // Save original queue index
               break; // Found a task
         }
      }

      if (current_task == NULL) {
         // Should not happen if all_queues_empty() is correct and loop condition is !all_queues_empty()
         // but can be a fallback or indicate a brief moment where queues become empty.
         // If main loop condition is fine, this path might not be strictly needed if scheduling one task per main loop iter.
         usleep(10000); // Small sleep if no task was found but loop continues (e.g. waiting for new arrivals - not applicable here)
         continue;
      }

      printf("RR_P_SCHEDULER: Next task to run: [%s] (Priority: %d, RB: %d) from PrioQ %d\n",
            current_task->name, current_task->priority, current_task->burst, current_task_priority_level_idx + 1);

      // --- Timer Interaction (same as in your working RR) ---
      g_time_slice_expired_flag = 0;
      g_task_completed_early = 0;
      if (g_scheduler_is_active) { // Check if timer system is active
            g_timer_can_start_counting = 1;
      }
      // ----------------------------------------------------

      int initial_burst_this_turn = current_task->burst;
      int executed_units_in_loop = 0;

      run(current_task, (initial_burst_this_turn < QUANTUM ? initial_burst_this_turn : QUANTUM));

      for (int unit = 0; unit < QUANTUM; ++unit) {
         if (current_task->burst <= 0) {
               if (g_scheduler_is_active) {
                  g_task_completed_early = 1;
                  g_timer_can_start_counting = 0;
               }
               break;
         }
         if (g_scheduler_is_active && g_time_slice_expired_flag) {
               // Preemption message handled after loop
               break;
         }
         current_task->burst--;
         executed_units_in_loop++;
         if (g_scheduler_is_active) {
               usleep(g_time_unit_us);
         } else { // Basic sequential execution if timer isn't active
               if (executed_units_in_loop >= initial_burst_this_turn) break;
         }
      }
      
      int final_reported_execution_time = executed_units_in_loop;

      if (g_scheduler_is_active && g_time_slice_expired_flag) {
         printf("RR_P_SCHEDULER: Task [%s] preempted by timer.\n", current_task->name);
         if (initial_burst_this_turn >= QUANTUM) {
               current_task->burst = initial_burst_this_turn - QUANTUM;
               final_reported_execution_time = QUANTUM;
         } else {
               current_task->burst = 0;
               final_reported_execution_time = initial_burst_this_turn;
         }
      } else if (current_task->burst == 0) {
         // Task finished on its own
         final_reported_execution_time = executed_units_in_loop;
      } else {
         // Ran for some units, not preempted by timer, not finished
         final_reported_execution_time = executed_units_in_loop;
      }

      if (current_task->burst < 0) current_task->burst = 0;

      if (g_scheduler_is_active) {
         g_timer_can_start_counting = 0;
      }

      printf("RR_P_SCHEDULER: Task [%s] executed for %d units. RB: %d.\n",
            current_task->name, final_reported_execution_time, current_task->burst);

      if (current_task->burst > 0) {
         printf("RR_P_SCHEDULER: Task [%s] not finished, re-adding to PrioQ %d.\n",
                  current_task->name, current_task_priority_level_idx + 1);
         // Add back to the TAIL of its original priority queue
         insert_tail(&priority_queues[current_task_priority_level_idx], current_task);
      } else {
         printf("RR_P_SCHEDULER: Task [%s] completed.\n", current_task->name);
         free(current_task->name);
         free(current_task);
      }
      printf("RR_P_SCHEDULER: --- End of current cycle ---\n");
   }

   printf("RR_P_SCHEDULER: All tasks completed.\n");

   if (g_scheduler_is_active) {
      printf("RR_P_SCHEDULER: Signalling timer thread to exit...\n");
      g_scheduler_is_active = 0;
      g_timer_can_start_counting = 0;
      g_task_completed_early = 1; // Ensure it breaks any counting loop
      pthread_join(g_timer_thread_id, NULL);
      printf("RR_P_SCHEDULER: Timer thread joined. Scheduler finished.\n");
   } else {
      printf("RR_P_SCHEDULER: Scheduler finished (timer was not active).\n");
   }
}