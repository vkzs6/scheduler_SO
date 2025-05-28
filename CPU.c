/**
 * "Virtual" CPU that also maintains track of system time.
 */

#include <stdio.h> // Para usar printf

#include "task.h"  // Inclui a definição da estrutura Task
// Não precisa incluir "cpu.h" aqui explicitamente se "scheduler.c" ou outro arquivo principal o fizer,
// mas é uma boa prática para autossuficiência do módulo.

// run this task for the specified time slice
void run(Task *task, int slice) {
    printf("Running task = [%s] [%d] [%d] for %d units.\n",task->name, task->priority, task->burst,slice);
}
