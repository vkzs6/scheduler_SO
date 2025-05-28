#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para strdup
#include <pthread.h> // Para a thread do timer
#include <unistd.h>  // Para usleep

#include "schedule_pa.h"
#include "CPU.h"      // Para run() e QUANTUM
#include "list.h"     // Para as operações da lista (insert_tail, delete, remove_head)
#include "task.h"     // Para a estrutura Task

// --- Variáveis Globais do Escalonador PA ---
struct node *pa_ready_queue = NULL; // Fila de prontos

// Variáveis globais para a thread do timer (semelhantes ao RR_p)
static pthread_t g_timer_thread_id_pa;
static volatile int g_time_slice_expired_flag_pa = 0;
static volatile int g_scheduler_is_active_pa = 0;
static volatile int g_timer_can_start_counting_pa = 0;
static volatile int g_task_completed_early_pa = 0;
static const int g_time_unit_us_pa = 100000; // 100ms por unidade de tempo simulada

// Contador de tempo global para o sistema (importante para o aging)
static int g_current_system_time_pa = 0;

// --- Função da Thread do Timer (semelhante à do RR_p) ---
void* timer_function_pa(void* arg) {
    printf("PA_TIMER_THREAD: Iniciada.\n");
    while (g_scheduler_is_active_pa) {
        while (!g_timer_can_start_counting_pa && g_scheduler_is_active_pa) {
            usleep(10000); // Pequena pausa para não sobrecarregar a CPU
        }

        if (!g_scheduler_is_active_pa) {
            break;
        }

        g_time_slice_expired_flag_pa = 0;
        g_task_completed_early_pa = 0;

        for (int i = 0; i < QUANTUM; ++i) { // QUANTUM de CPU.h
            if (!g_scheduler_is_active_pa || !g_timer_can_start_counting_pa || g_task_completed_early_pa) {
                break;
            }
            usleep(g_time_unit_us_pa);
        }

        if (g_scheduler_is_active_pa && g_timer_can_start_counting_pa && !g_task_completed_early_pa) {
            g_time_slice_expired_flag_pa = 1;
        }
        g_timer_can_start_counting_pa = 0;
    }
    printf("PA_TIMER_THREAD: Finalizando.\n");
    return NULL;
}

// --- Funções do Escalonador PA ---

void pa_add(char *name, int priority, int burst) {
    Task *newTask = (Task *)malloc(sizeof(Task));
    if (!newTask) {
        fprintf(stderr, "PA_ADD: Falha ao alocar memória para a tarefa %s\n", name);
        return;
    }
    newTask->name = strdup(name);
    if (!newTask->name) {
        fprintf(stderr, "PA_ADD: Falha ao duplicar nome para a tarefa %s\n", name);
        free(newTask);
        return;
    }
    newTask->priority = priority;
    newTask->burst = burst;
    // ATENÇÃO: Usando o campo 'deadline' para armazenar o tempo de entrada na fila de prontos
    // ou o tempo da última verificação de aging para esta tarefa.
    newTask->deadline = g_current_system_time_pa;

    insert_tail(&pa_ready_queue, newTask);
    printf("PA_ADD: Tarefa [%s] (Prio: %d, Burst: %d) adicionada. Tempo do sistema: %d\n",
           name, priority, burst, g_current_system_time_pa);
}

// Função auxiliar para aplicar o aging e selecionar a próxima tarefa
Task* apply_aging_and_select_next_task() {
    if (pa_ready_queue == NULL) {
        return NULL;
    }

    struct node *current_node = pa_ready_queue;
    Task *selected_task = NULL;
    

    int highest_priority = 9999; // Assumindo que prioridades são positivas

    // Aplicar Aging
    printf("PA_AGING_CHECK (Tempo do Sistema: %d): Verificando envelhecimento...\n", g_current_system_time_pa);
    while (current_node != NULL) {
        Task *task = current_node->task;
        // Tempo que a tarefa esperou desde a última verificação/entrada.
        // task->deadline armazena o g_current_system_time_pa da última vez.
        int time_waited = g_current_system_time_pa - task->deadline;

        if (task->priority > MAX_EFFECTIVE_PRIORITY && time_waited >= AGING_THRESHOLD) {
            int old_priority = task->priority;
            task->priority = (task->priority - 1 < MAX_EFFECTIVE_PRIORITY) ? MAX_EFFECTIVE_PRIORITY : task->priority - 1;
            printf("PA_AGING: Tarefa [%s] envelheceu! Prio: %d -> %d (Esperou: %d unidades)\n",
                   task->name, old_priority, task->priority, time_waited);
            task->deadline = g_current_system_time_pa; // Reseta o contador de espera para o próximo aging
        }
        current_node = current_node->next;
    }

    // Selecionar a tarefa com a maior prioridade (menor valor numérico)
    // Em caso de empate, a que estiver primeiro na lista (FIFO entre prioridades iguais)
    current_node = pa_ready_queue; // Reinicia a iteração para seleção
    while (current_node != NULL) {
        if (current_node->task->priority < highest_priority) {
            highest_priority = current_node->task->priority;
            selected_task = current_node->task;
            // Guarda o nó anterior ao selecionado para facilitar a remoção se não for a cabeça.
            // No entanto, a função delete do seu list.c remove pelo nome, então isso é menos crítico aqui.
        }
        current_node = current_node->next;
    }

    if (selected_task) {
        // Remove a tarefa selecionada da fila.
        // A função delete fornecida no seu list.c usa strcmp para encontrar a tarefa pelo nome.
        delete(&pa_ready_queue, selected_task);
    }
    return selected_task;
}


void pa_schedule() {
    Task *current_task = NULL;
    g_current_system_time_pa = 0; // Reseta o tempo do sistema no início do escalonamento

    printf("PA_SCHEDULER: Iniciando Escalonador de Prioridade com Aging (Quantum: %d, Limiar Aging: %d).\n", QUANTUM, AGING_THRESHOLD);

    g_scheduler_is_active_pa = 1;
    g_time_slice_expired_flag_pa = 0;
    g_timer_can_start_counting_pa = 0;
    g_task_completed_early_pa = 0;

    if (pthread_create(&g_timer_thread_id_pa, NULL, timer_function_pa, NULL) != 0) {
        fprintf(stderr, "PA_SCHEDULER: Erro ao criar a thread do timer. Saindo.\n");
        g_scheduler_is_active_pa = 0;
        return;
    }

    while (1) {
        current_task = apply_aging_and_select_next_task();

        if (current_task == NULL) {
            // Se não há tarefas, mas o escalonador ainda deveria estar ativo
            // (ex: esperando por futuras chegadas, não aplicável em um sistema estático simples),
            // ou se todas as tarefas terminaram.
            // Para um conjunto estático de tarefas, se a fila está vazia, terminamos.
            if (pa_ready_queue == NULL) { // Dupla verificação, apply_aging_and_select_next_task já checa
                 printf("PA_SCHEDULER: Fila de prontos vazia. Todas as tarefas concluídas.\n");
                 break;
            }
            // Se apply_aging... retornou NULL mas a fila não está REALMENTE vazia (improvável com a lógica atual),
            // ou se estamos em um ponto onde não há trabalho mas o sistema não deve parar.
            // Para este projeto, se a fila está vazia, o loop principal deve terminar.
            // Se retornou NULL mas a fila não está vazia, é um bug.
            // Vamos simular passagem de tempo se não há tarefas prontas (para o aging de futuras tarefas)
            printf("PA_SCHEDULER: Nenhuma tarefa selecionada para rodar. Avançando tempo do sistema.\n");
            usleep(g_time_unit_us_pa); // Simula uma unidade de tempo ociosa
            g_current_system_time_pa++;
            continue; // Tenta selecionar novamente após avançar o tempo (para aplicar aging)
        }

        printf("PA_SCHEDULER: Próxima tarefa para executar: [%s] (Prio Atual: %d, Burst Restante: %d)\n",
               current_task->name, current_task->priority, current_task->burst);

        g_time_slice_expired_flag_pa = 0;
        g_task_completed_early_pa = 0;
        if (g_scheduler_is_active_pa) {
            g_timer_can_start_counting_pa = 1;
        }

        int slice_to_run = (current_task->burst < QUANTUM) ? current_task->burst : QUANTUM;
        run(current_task, slice_to_run); // Anuncia a execução

        int units_executed_this_turn = 0;
        for (int unit = 0; unit < slice_to_run; ++unit) {
            if (current_task->burst <= 0) { // Deveria ser verificado antes de decrementar
                 if (g_scheduler_is_active_pa) {
                    g_task_completed_early_pa = 1;
                    g_timer_can_start_counting_pa = 0;
                }
                break;
            }
             if (g_scheduler_is_active_pa && g_time_slice_expired_flag_pa) {
                break;
            }

            current_task->burst--;
            units_executed_this_turn++;
            g_current_system_time_pa++; // Avança o tempo global do sistema
            
            if (g_scheduler_is_active_pa) { // Apenas dorme se o timer estiver ativo para simular
                 usleep(g_time_unit_us_pa);
            }
        }
        
        // Se a tarefa foi preemptada, mas o loop de unidades não rodou o slice_to_run inteiro,
        // o burst não foi decrementado pelo timer. Precisamos garantir que o burst reflita o tempo de CPU.
        // A lógica acima já decrementa o burst por unidade de tempo simulada.
        // Se g_time_slice_expired_flag_pa é true, então units_executed_this_turn pode ser menor que slice_to_run.
        // O burst já foi decrementado 'units_executed_this_turn' vezes.

        if (g_scheduler_is_active_pa) {
            g_timer_can_start_counting_pa = 0; // Para o timer desta tarefa
        }

        printf("PA_SCHEDULER: Tarefa [%s] executou por %d unidades. Burst restante: %d. Tempo do Sistema: %d\n",
               current_task->name, units_executed_this_turn, current_task->burst, g_current_system_time_pa);

        if (current_task->burst > 0) {
            printf("PA_SCHEDULER: Tarefa [%s] não concluída, readicionando à fila.\n", current_task->name);
            // ATENÇÃO: Atualiza o 'deadline' (tempo de entrada/última_verificação) antes de re-enfileirar
            current_task->deadline = g_current_system_time_pa;
            insert_tail(&pa_ready_queue, current_task);
        } else {
            printf("PA_SCHEDULER: Tarefa [%s] concluída.\n", current_task->name);
            free(current_task->name);
            free(current_task);
        }
        printf("PA_SCHEDULER: --- Fim do ciclo atual ---\n");
    }

    printf("PA_SCHEDULER: Todas as tarefas foram concluídas.\n");

    if (g_scheduler_is_active_pa) {
        printf("PA_SCHEDULER: Sinalizando para a thread do timer finalizar...\n");
        g_scheduler_is_active_pa = 0;
        g_timer_can_start_counting_pa = 0; // Garante que pare
        g_task_completed_early_pa = 1;     // Ajuda a quebrar o loop do timer
        pthread_join(g_timer_thread_id_pa, NULL);
        printf("PA_SCHEDULER: Thread do timer finalizada. Escalonador PA encerrado.\n");
    } else {
        printf("PA_SCHEDULER: Escalonador PA encerrado (timer não estava ativo).\n");
    }
}