/**
 * Various list operations
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "task.h"


// add a new task to the list of tasks
void insert(struct node **head, Task *newTask) { // add the new task to the list 

    struct node *newNode = malloc(sizeof(struct node));
    
    newNode->task = newTask;
    newNode->next = *head;
    *head = newNode;
}

// delete the selected task from the list
void delete(struct node **head, Task *task) {
    struct node *temp;
    struct node *prev;

    temp = *head;
    // special case - beginning of list
    if (strcmp(task->name,temp->task->name) == 0) {
        *head = (*head)->next;
    }
    else {
        // interior or last element in the list
        prev = *head;
        temp = temp->next;
        while (strcmp(task->name,temp->task->name) != 0) {
            prev = temp;
            temp = temp->next;
        }

        prev->next = temp->next;
    }
}

// traverse the list
void traverse(struct node *head) {
    struct node *temp;
    temp = head;

    while (temp != NULL) {
        printf("[%s] [%d] [%d]\n",temp->task->name, temp->task->priority, temp->task->burst);
        temp = temp->next;
    }
}

// Insert at the end
void insert_tail(struct node **head, Task *newTask) {
    struct node *newNode = malloc(sizeof(struct node));
    newNode->task = newTask;
    newNode->next = NULL;

    if (*head == NULL) { // If list is empty
        *head = newNode;
        return;
    }

    struct node *current = *head;
    while (current->next != NULL) { // Traverse to the end
        current = current->next;
    }
    current->next = newNode; // Add to tail
}

Task* remove_head(struct node **head) {
    if (*head == NULL) {
        return NULL; // Or handle error
    }
    struct node *temp_node = *head;
    Task *task_to_return = temp_node->task;
    *head = (*head)->next;
    free(temp_node); // Free the node, not the task data itself yet
    return task_to_return;
}
