#include <stdio.h>
#include <stdlib.h>   // Para malloc, free, atoi
#include <string.h>   // Para strdup, strsep

#include "task.h"
#include "list.h"
#include "schedule_pa.h"
//#include "schedule_edf.h" // Comentado - para o escalonador EDF
//#include "schedule_rr.h"  // Comentado - para o escalonador Round-Robin
//#include "schedule_rr_p.h" // Incluído - para o escalonador Round-Robin com Prioridade

#define SIZE    100 // Tamanho do buffer para ler linhas do arquivo

int main(int argc, char *argv[])
{
    FILE *in;         // Ponteiro para o arquivo de entrada
    char *temp;
    char task[SIZE];  // Buffer para cada linha lida do arquivo

    char *name;
    int priority;
    int burst;
    //int deadline; // Comentado, usado apenas para EDF

    if (argc < 2) { // Verifica se o nome do arquivo foi passado como argumento
        fprintf(stderr, "Uso: %s <arquivo_de_entrada>\n", argv[0]);
        return 1;
    }

    in = fopen(argv[1],"r"); // Abre o arquivo passado como argumento (ex: ./rr_p tarefas.txt)
    if (in == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }
    
    while (fgets(task,SIZE,in) != NULL) { // Lê uma linha do arquivo por vez
        temp = strdup(task); // Duplica a string lida (strsep modifica a string original)
        name = strsep(&temp,",");
        priority = atoi(strsep(&temp,",")); // Converte string para inteiro
        burst = atoi(strsep(&temp,","));
        //deadline = atoi(strsep(&temp, ",")); // Linha para ler o deadline, se necessário
        
        // add the task to the scheduler's list of tasks
        //edf_add(name,priority,burst, deadline); // Chamada genérica, talvez para EDF [cite: 7]
        //rr_add(name, priority, burst);      // Para Round-Robin simples (comentado)
        //rr_p_add(name,priority,burst);      // Chamada para adicionar tarefa no RR com Prioridade
        pa_add(name, priority, burst);
        free(temp); // Libera a memória da string duplicada
    }

    fclose(in); // Fecha o arquivo

    // invoke the scheduler
    //rr_schedule();    // Chamada para o escalonador RR (comentado)
    //rr_p_schedule();  // Chama a função principal do escalonador RR com Prioridade
    pa_schedule();
    //edf_schedule();
    return 0;
}
