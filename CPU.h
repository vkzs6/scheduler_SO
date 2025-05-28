#ifndef CPU   // Se CPU não estiver definido...
#define CPU

// length of a time quantum
#define QUANTUM 10 // Define o tamanho da fatia de tempo (quantum) como 10 unidades [cite: 21]
#include "task.h"  // Inclui a definição da estrutura Task

// run the specified task for the following time slice
void run(Task *task, int slice); // Declaração da função run

#endif