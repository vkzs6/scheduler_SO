#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>     // Para INT_MAX

#include "list.h"       // Sua biblioteca de lista
#include "task.h"       // Sua struct Task
#include "CPU.h"        // Para a função run()
#include "schedule_edf.h"


static struct node *edf_task_list_head = NULL;
static int edf_next_tid = 1;
static int edf_tasks_count = 0;


void edf_add(char *name, int priority, int burst, int deadline) {
    Task *newTask = (Task *)malloc(sizeof(Task));
    if (!newTask) {
        fprintf(stderr, "EDF Error: Memory allocation failed for new task '%s'.\n", name);
        exit(EXIT_FAILURE);
    }

    newTask->name = strdup(name);
    if (!newTask->name) {
        fprintf(stderr, "EDF Error: Memory allocation failed for task name '%s'.\n", name);
        free(newTask);
        exit(EXIT_FAILURE);
    }

    newTask->tid = edf_next_tid++; // Atribui TID sequencial interno
    newTask->priority = priority;
    newTask->burst = burst;       // Este campo será o burst restante
    newTask->deadline = deadline;

    // Insere na lista usando a função da sua biblioteca list.c
    // A função insert() do seu repositório adiciona no início.
    insert(&edf_task_list_head, newTask);
    edf_tasks_count++;

    // printf("EDF: Added Task %s [TID: %d, Burst: %d, Deadline: %d, Prio: %d]\n",
    //        newTask->name, newTask->tid, newTask->burst, newTask->deadline, newTask->priority);
}

void edf_schedule() {
    if (edf_task_list_head == NULL) {
        printf("EDF: No tasks to schedule.\n");
        return;
    }

    printf("\n--- Earliest Deadline First (EDF) Scheduling ---\n");

    int currentTime = 0;
    int completed_tasks = 0;
    struct node *iterator_node;

    // Loop principal do escalonador: continua enquanto houver tarefas não concluídas
    while (completed_tasks < edf_tasks_count) {
        struct node *selected_node = NULL;
        Task *task_to_run = NULL;
        int earliest_deadline = INT_MAX;

        // 1. Encontrar a tarefa pronta com o deadline mais próximo
        iterator_node = edf_task_list_head;
        while (iterator_node != NULL) {
            if (iterator_node->task->burst > 0) { // A tarefa ainda não terminou
                if (iterator_node->task->deadline < earliest_deadline) {
                    earliest_deadline = iterator_node->task->deadline;
                    selected_node = iterator_node;
                } else if (iterator_node->task->deadline == earliest_deadline) {
                    // Critério de desempate: menor TID
                    if (selected_node == NULL || iterator_node->task->tid < selected_node->task->tid) {
                        selected_node = iterator_node;
                    }
                }
            }
            iterator_node = iterator_node->next;
        }

        // 2. Se uma tarefa foi selecionada, execute-a por 1 unidade de tempo
        if (selected_node != NULL) {
            task_to_run = selected_node->task;

            // Chama a função run da sua CPU.c para simular a execução por 1 fatia de tempo
            run(task_to_run, 1); // O '1' representa uma unidade de tempo para preempção
            task_to_run->burst--; // Decrementa o burst restante

            // 3. Verifica se a tarefa foi concluída
            if (task_to_run->burst == 0) {
                completed_tasks++;
                printf("EDF: Task %s [TID: %d] completed at time %d. ",
                       task_to_run->name, task_to_run->tid, currentTime + 1);
                if ((currentTime + 1) > task_to_run->deadline) {
                    printf("!!! DEADLINE MISSED (Deadline: %d) !!!\n", task_to_run->deadline);
                } else {
                    printf("Deadline met.\n");
                }
            }
        } else {
            // Nenhuma tarefa está pronta para executar (todas com burst > 0 já terminaram,
            // ou estamos esperando tarefas futuras se o sistema suportasse chegadas dinâmicas)
            // Neste modelo, se completed_tasks < edf_tasks_count, e nenhuma tarefa foi selecionada,
            // pode indicar um erro, ou que todas as tarefas com burst > 0 foram finalizadas.
            // O loop deve terminar corretamente quando completed_tasks == edf_tasks_count.
            printf("EDF: CPU Idle at time %d\n", currentTime);
        }

        currentTime++;

        // Parada de segurança para evitar loop infinito em caso de erro
        if (currentTime > (edf_tasks_count * 200 + 500) && edf_tasks_count > 0) { // Limite generoso
            fprintf(stderr, "EDF Warning: Simulation time limit exceeded. Possible infinite loop or issue.\n");
            fprintf(stderr, "Completed %d out of %d tasks.\n", completed_tasks, edf_tasks_count);
            break;
        }
    }

    printf("--- EDF Scheduling Finished at time %d ---\n\n", currentTime);

    // Limpeza final: Liberar todos os nós e tarefas da lista
    // Isso é crucial porque a função delete() do seu list.c vaza memória.
    iterator_node = edf_task_list_head;
    while (iterator_node != NULL) {
        struct node *temp_node_to_free = iterator_node;
        iterator_node = iterator_node->next;

        free(temp_node_to_free->task->name); // Liberar nome da tarefa (strdup)
        free(temp_node_to_free->task);       // Liberar a struct Task
        free(temp_node_to_free);             // Liberar o nó da lista
    }

    edf_task_list_head = NULL; // Resetar para futuras execuções (se houver)
    edf_tasks_count = 0;
    edf_next_tid = 1; // Resetar contador de TID
}