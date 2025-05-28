#ifndef SCHEDULE_RR_P_H // 
#define SCHEDULE_RR_P_H // 

#include "task.h" // Inclui a definição da estrutura Task
#include "list.h" // Inclui as definições da lista encadeada (struct node, funções da lista)

// Define o número de níveis de prioridade.
#define MAX_PRIORITY_LEVELS 10 // Para prioridades de 1 a 10

// Array de filas de aptos, uma para cada nível de prioridade.
// 'extern' significa que está definido em um arquivo .c (schedule_rr_p.c).
extern struct node *priority_queues[MAX_PRIORITY_LEVELS];

// Declaração da função da thread do timer
void* timer_function(void* arg);

// Declaração da função para adicionar uma tarefa ao escalonador RR_p
void rr_p_add(char *name, int priority, int burst);

// Declaração da função principal do escalonador RR_p
void rr_p_schedule();

#endif // SCHEDULE_RR_P_H