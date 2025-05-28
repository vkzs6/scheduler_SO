#ifndef LIST_H
#define LIST_H
/**
 * list data structure containing the tasks in the system
 */

#include "task.h" // Inclui a definição da Task

// Define a estrutura de um nó da lista
struct node {
    Task *task;         // Ponteiro para a tarefa armazenada neste nó
    struct node *next;  // Ponteiro para o próximo nó na lista
};

// Declaração das funções de operação da lista
void insert(struct node **head, Task *task);          // Insere no início
void delete(struct node **head, Task *task);          // Deleta uma tarefa específica
void traverse(struct node *head);                     // Percorre e imprime a lista
void insert_tail(struct node **head, Task *newTask);  // Insere no final da lista
Task* remove_head(struct node **head);                // Remove e retorna o primeiro elemento
#endif