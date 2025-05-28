/**
 * Representation of a task in the system.
 */

#ifndef TASK_H
#define TASK_H

// representation of a task
typedef struct task {
    char *name;     // Nome da tarefa (ex: "P1", "ProcessoA")
    int tid;        // ID da tarefa (não usado no seu código atual, mas útil para identificar unicamente)
    int priority;   // Nível de prioridade da tarefa
    int burst;      // Tempo total que a tarefa precisa para executar na CPU
    int deadline;   // Prazo final para a tarefa ser concluída (usado no EDF)
    unsigned long time_last_seen_by_scheduler; // NOVO CAMPO para Aging
} Task;

#endif



