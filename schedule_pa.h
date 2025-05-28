#ifndef SCHEDULE_PA_H
#define SCHEDULE_PA_H

#include "task.h"
#include "list.h"

// --- Constantes para o Algoritmo de Aging ---

// Limiar de Envelhecimento: Quantas unidades de tempo uma tarefa espera
// antes que sua prioridade seja aumentada.
#define AGING_THRESHOLD 10

// Prioridade Máxima: A prioridade mais alta que uma tarefa pode alcançar (1, conforme o projeto).
#define MAX_EFFECTIVE_PRIORITY 1

// --- Variáveis Globais do Escalonador PA (declaradas como extern) ---
// Fila de prontos para o algoritmo PA
extern struct node *pa_ready_queue;

// --- Funções do Escalonador PA ---

/**
 * Adiciona uma nova tarefa ao sistema para o escalonador de Prioridade com Aging.
 * O campo task->deadline será usado internamente para marcar o tempo de entrada na fila.
 */
void pa_add(char *name, int priority, int burst);

/**
 * Executa o algoritmo de escalonamento de Prioridade com Aging.
 */
void pa_schedule();

#endif // SCHEDULE_PA_H