// gcc -o rr_scheduler driver.c schedule_rr.c list.c CPU.c -I. -Wall -g -pthread
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strdup used in rr_add
#include <pthread.h>
#include <unistd.h> // For usleep/sleep

#include "schedule_rr.h"
#include "CPU.h" // For QUANTUM and run() [cite: 3]

// Define the global ready queue
struct node *rr_ready_queue = NULL;

pthread_t g_timer_thread_id;
volatile int g_time_slice_expired_flag = 0;
volatile int g_scheduler_is_active = 0;         // To control the timer thread's main loop
volatile int g_timer_can_start_counting = 0;    // Scheduler signals timer to start a quantum
volatile int g_task_completed_early = 0;      // Scheduler signals timer if task finished early
const int g_time_unit_us = 100000; 

// Timer thread's function
void* timer_function(void* arg) {
    printf("TIMER_THREAD: Started.\n");
    while (g_scheduler_is_active) {
        // Wait for the scheduler to signal that a task is running and timer can start
        while (!g_timer_can_start_counting && g_scheduler_is_active) {
            usleep(10000); // Sleep briefly to avoid busy-waiting for the signal
        }

        if (!g_scheduler_is_active) { // Check again after waking up
            break;
        }

        // printf("TIMER_THREAD: Starting countdown for %d units.\n", QUANTUM);
        g_time_slice_expired_flag = 0; // Ensure flag is clear before starting
        g_task_completed_early = 0;    // Reset early completion flag

        for (int i = 0; i < QUANTUM; ++i) {
            if (!g_scheduler_is_active || !g_timer_can_start_counting || g_task_completed_early) {
                // Scheduler ended, or told timer to stop counting for this task (e.g. task finished)
                // printf("TIMER_THREAD: Countdown interrupted (early exit or scheduler ended).\n");
                break;
            }
            usleep(g_time_unit_us); // Sleep for one simulated time unit
        }

        if (g_scheduler_is_active && g_timer_can_start_counting && !g_task_completed_early) {
            // If the loop completed naturally (quantum expired) and task wasn't marked as finished early
            g_time_slice_expired_flag = 1;
            // printf("TIMER_THREAD: Time slice EXPIRED.\n");
        }
        g_timer_can_start_counting = 0; // Reset: wait for next signal from scheduler
    }
    printf("TIMER_THREAD: Exiting.\n");
    return NULL;
}

void rr_add(char *name, int priority, int burst) {
    Task *newTask = (Task *)malloc(sizeof(Task));
    if (!newTask) {
        fprintf(stderr, "Error allocating memory for task\n");
        return;
    }
    // strdup allocates memory, ensure it's freed when task is done
    newTask->name = strdup(name);
    newTask->priority = priority; // Priority not used in basic RR, but part of Task struct
    newTask->burst = burst;
    // Add any other fields from task.h if necessary

    // Use your new list function to add to the tail for FIFO
    insert_tail(&rr_ready_queue, newTask);
    printf("RR: Added task %s, Priority: %d, Burst: %d\n", name, priority, burst);
}

void rr_schedule() {
    Task *current_task;
    // int time_slice = QUANTUM; // QUANTUM from CPU.h [cite: 3] // We'll use QUANTUM directly

    printf("RR_SCHEDULER: Starting Round-Robin Scheduler (Quantum: %d units).\n", QUANTUM);

    g_scheduler_is_active = 1; // Signal that the scheduler (and thus timer) is active
    g_time_slice_expired_flag = 0;
    g_timer_can_start_counting = 0;
    g_task_completed_early = 0;

    if (pthread_create(&g_timer_thread_id, NULL, timer_function, NULL) != 0) {
        fprintf(stderr, "RR_SCHEDULER: Error creating timer thread. Running without timer.\n");
        g_scheduler_is_active = 0; // Disable timer integration if thread creation fails
    }

    while (rr_ready_queue != NULL) {
        current_task = remove_head(&rr_ready_queue);

        if (current_task == NULL) {
            break;
        }

        printf("RR_SCHEDULER: Next task to run: [%s] (Priority: %d, Remaining Burst: %d)\n",
               current_task->name, current_task->priority, current_task->burst);

        // --- Timer Interaction ---
        g_time_slice_expired_flag = 0;    // Reset flag before task runs
        g_task_completed_early = 0;       // Reset early completion flag
        if (g_scheduler_is_active) {      // Only if timer thread was successfully created
             g_timer_can_start_counting = 1; // Signal timer to start its countdown
        }
        // -------------------------

        // Call run() to display intent [cite: 3]
        // The actual slice for run() could be min(burst, QUANTUM)
        run(current_task, (current_task->burst < QUANTUM ? current_task->burst : QUANTUM));

        int initial_burst_this_turn = current_task->burst; // Capture burst before simulation
        int executed_units_in_loop = 0; // How many units our simulation loop ran

        // Simulate task execution unit by unit
        for (int unit = 0; unit < QUANTUM; ++unit) {
            if (current_task->burst <= 0) { // Task finished its own work
                if (g_scheduler_is_active) {
                    g_task_completed_early = 1;
                    g_timer_can_start_counting = 0; // Stop current timer cycle
                }
                break; // Exit simulation loop
            }

            if (g_scheduler_is_active && g_time_slice_expired_flag) {
                // Timer preempted, do not simulate further for this task in this turn.
                // The "preempted by timer" message will be handled after the loop.
                break; // Exit simulation loop
            }

            // Simulate one unit of execution by the scheduler
            current_task->burst--;
            executed_units_in_loop++;
            
            if (g_scheduler_is_active) {
                 usleep(g_time_unit_us); // Scheduler "sleeps" for one time unit
            } else {
                // Fallback for sequential execution if timer thread failed
                if (executed_units_in_loop >= initial_burst_this_turn) break; 
            }
        }

        // --- Determine actual execution time and update burst based on preemption ---
        int final_reported_execution_time;

        if (g_scheduler_is_active && g_time_slice_expired_flag) {
            printf("RR_SCHEDULER: Task [%s] preempted by timer.\n", current_task->name);
            // Timer fired, meaning the full quantum time slot has passed.
            // Charge the task for the full quantum if it had enough burst initially.
            if (initial_burst_this_turn >= QUANTUM) {
                current_task->burst = initial_burst_this_turn - QUANTUM; // Correct final burst
                final_reported_execution_time = QUANTUM;
            } else {
                // Task had less than a quantum to begin with, but timer still fired
                // (perhaps concurrently with task finishing). It ran for its entirety.
                current_task->burst = 0; // Should already be 0 if loop completed due to burst <=0
                final_reported_execution_time = initial_burst_this_turn;
            }
        } else if (current_task->burst == 0) { // Task finished on its own
            // `executed_units_in_loop` correctly reflects how much it ran.
            final_reported_execution_time = executed_units_in_loop;
             // (g_task_completed_early should have been set if timer was active)
        } else { // Task ran for some units, not preempted by timer flag, and not finished
                 // This case implies it completed its `executed_units_in_loop`
                 // which should be QUANTUM if it didn't finish early.
            final_reported_execution_time = executed_units_in_loop;
        }
        
        // Ensure burst doesn't go negative from adjustment
        if (current_task->burst < 0) {
            current_task->burst = 0;
        }

        if (g_scheduler_is_active) {
            g_timer_can_start_counting = 0; // Tell timer its job for this slice is done or interrupted
        }

        printf("RR_SCHEDULER: Task [%s] actually executed for %d units. Remaining burst: %d.\n",
               current_task->name, final_reported_execution_time, current_task->burst);

        if (current_task->burst > 0) {
            printf("RR_SCHEDULER: Task [%s] not finished, re-adding to queue.\n", current_task->name);
            insert_tail(&rr_ready_queue, current_task);
        } else {
            printf("RR_SCHEDULER: Task [%s] completed.\n", current_task->name);
            free(current_task->name);
            free(current_task);
        }
        g_time_slice_expired_flag = 0; // Reset for safety, though timer also resets.
        printf("RR_SCHEDULER: --- End of current cycle ---\n");
    }

    printf("RR_SCHEDULER: All tasks completed.\n");

    if (g_scheduler_is_active) { // If timer thread was started
        printf("RR_SCHEDULER: Signalling timer thread to exit...\n");
        g_scheduler_is_active = 0;      // Signal timer thread to stop its main loop
        g_timer_can_start_counting = 0; // Ensure it's not stuck waiting to count
        g_task_completed_early = 1;     // Ensure it breaks any counting loop
        pthread_join(g_timer_thread_id, NULL); // Wait for timer thread to finish
        printf("RR_SCHEDULER: Timer thread joined. Scheduler finished.\n");
    } else {
        printf("RR_SCHEDULER: Scheduler finished (timer was not active).\n");
    }
}