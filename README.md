Guia de Execução dos Escalonadores e Implementação de Menu Interativo
Este guia detalha duas formas de compilar e executar os quatro algoritmos de escalonamento implementados (Round Robin, Round Robin com Prioridade, EDF e Prioridade com Aging) utilizando o Makefile e o driver.c.

Método 1: Compilação Individual via Makefile e Modificação Manual do driver.c
Este método requer que você edite o arquivo driver.c para cada algoritmo que deseja testar, selecionando o cabeçalho (.h) e as funções add e schedule corretas, e então use o make para compilar o alvo específico.

Passos Gerais:

Abra driver.c.

Modifique os #include: Comente os includes dos arquivos de cabeçalho dos outros escalonadores e deixe apenas o do algoritmo desejado descomentado.

Modifique as chamadas das funções add e schedule: Dentro do loop de leitura de tarefas e após o loop, comente as chamadas das funções dos outros escalonadores e deixe apenas as do algoritmo desejado descomentadas. Lembre-se que edf_add requer um parâmetro deadline adicional, que também precisa ser lido do arquivo de entrada quando EDF está selecionado.

Salve driver.c.

Limpe compilações anteriores (recomendado): No terminal, execute make clean.

Compile o alvo específico usando make <alvo>.

Execute o programa gerado.

Para cada Escalonador:

A. Round Robin (RR)

Modificações em driver.c:

Descomente: #include "schedule_rr.h"

Descomente: add_rr(name, priority, burst); (a prioridade será ignorada pelo RR)

Descomente: schedule_rr();

Comente as linhas equivalentes dos outros três escalonadores.

Certifique-se de que a leitura do deadline (se existir no seu loop) esteja comentada, pois add_rr não o utiliza.

Comando make:

make rr

Comando para Executar:

./rr nome_do_seu_arquivo_de_tarefas.txt

B. Round Robin com Prioridade (RR_P)

Modificações em driver.c:

Descomente: #include "schedule_rr_p.h"

Descomente: rr_p_add(name, priority, burst);

Descomente: rr_p_schedule();

Comente as linhas equivalentes dos outros três escalonadores.

Certifique-se de que a leitura do deadline esteja comentada.

Comando make:

make rr_p

Comando para Executar:

./rr_p nome_do_seu_arquivo_de_tarefas.txt

C. Earliest Deadline First (EDF)

Modificações em driver.c:

Descomente: #include "schedule_edf.h"

Importante: No loop de leitura de tarefas em driver.c, você precisa ler o quarto parâmetro, o deadline. Certifique-se que a linha deadline = atoi(strsep(&temp,",")); (ou similar) esteja ativa.

Descomente: edf_add(name, priority, burst, deadline);

Descomente: edf_schedule();

Comente as linhas equivalentes dos outros três escalonadores.

Seu arquivo de tarefas para EDF deve ter 4 colunas: nome,prioridade,burst,deadline.

Comando make:

make edf

Comando para Executar:

./edf nome_do_seu_arquivo_de_tarefas_com_deadline.txt

D. Prioridade com Aging (PA)

Modificações em driver.c:

Descomente: #include "schedule_pa.h"

Descomente: pa_add(name, priority, burst);

Descomente: pa_schedule();

Comente as linhas equivalentes dos outros três escalonadores.

Certifique-se de que a leitura do deadline esteja comentada (a menos que você esteja usando o campo deadline internamente para o PA, como fizemos, mas pa_add só espera 3 argumentos).

Comando make:

make pa

Comando para Executar:

./pa nome_do_seu_arquivo_de_tarefas.txt

Método 2: Implementando um Menu Interativo no driver.c (Sugestão do Professor)
Este método é mais prático, pois permite que você compile um único executável que pode rodar qualquer um dos quatro escalonadores, selecionando-o através de um menu no momento da execução.

Vantagens:

Não é necessário editar e recompilar o driver.c para cada teste.

Mais fácil de demonstrar e comparar os algoritmos rapidamente.

1. Modificações no driver.c:

Você precisará:

Incluir todos os arquivos de cabeçalho dos seus escalonadores.

Adicionar código para exibir um menu e ler a escolha do usuário.

Usar uma estrutura switch (ou if-else if) para chamar as funções add e schedule apropriadas com base na escolha.

Exemplo de driver.c modificado (principais alterações):

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "task.h"
#include "list.h"
#include "CPU.h" // Para QUANTUM, se precisar ser exibido ou usado no driver

// Inclua TODOS os cabeçalhos dos seus escalonadores
#include "schedule_rr.h"
#include "schedule_rr_p.h"
#include "schedule_edf.h"
#include "schedule_pa.h"

#define SIZE 100

int main(int argc, char *argv[]) {
    FILE *in;
    char *temp;
    char task_line[SIZE]; // Renomeado para evitar conflito com a struct Task

    char *name;
    int priority;
    int burst;
    int deadline = 0; // Para EDF, inicialize
    char *deadline_str;

    int choice;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_de_tarefas>\n", argv[0]);
        return 1;
    }

    // Menu para seleção do algoritmo
    printf("Selecione o Algoritmo de Escalonamento:\n");
    printf("1. Earliest Deadline First (EDF)\n");
    printf("2. Round Robin (RR)\n");
    printf("3. Round Robin com Prioridade (RR_P)\n");
    printf("4. Prioridade com Aging (PA)\n");
    printf("Escolha: ");
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Entrada inválida.\n");
        return 1;
    }

    // Limpar o buffer do stdin após scanf
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    in = fopen(argv[1], "r");
    if (in == NULL) {
        perror("Erro ao abrir o arquivo de tarefas");
        return 1;
    }

    while (fgets(task_line, SIZE, in) != NULL) {
        temp = strdup(task_line);
        name = strsep(&temp, ",");
        priority = atoi(strsep(&temp, ","));
        burst = atoi(strsep(&temp, ","));

        // Para EDF, precisamos ler o deadline. Para outros, este valor pode ser ignorado pelas funções 'add'.
        deadline_str = strsep(&temp, ","); 
        if (deadline_str != NULL) {
            deadline = atoi(deadline_str);
        } else if (choice == 1) { // Se é EDF e não há deadline no arquivo
            fprintf(stderr, "Aviso: Tarefa %s sem deadline para EDF. Usando 0.\n", name);
            deadline = 0; 
        }


        switch (choice) {
            case 1: // EDF
                edf_add(name, priority, burst, deadline);
                break;
            case 2: // RR
                add_rr(name, priority, burst);
                break;
            case 3: // RR_P
                rr_p_add(name, priority, burst);
                break;
            case 4: // PA
                pa_add(name, priority, burst);
                break;
            default:
                // Não deveria acontecer se a entrada do menu foi validada, mas é uma boa prática
                break; 
        }
        free(temp);
    }
    fclose(in);

    // Invoca o escalonador escolhido
    switch (choice) {
        case 1:
            edf_schedule();
            break;
        case 2:
            schedule_rr();
            break;
        case 3:
            rr_p_schedule();
            break;
        case 4:
            pa_schedule();
            break;
        default:
            printf("Nenhum algoritmo selecionado ou escolha inválida.\n");
            return 1;
    }

    return 0;
}

2. Ajustes no Makefile:

Você precisará de um novo alvo no seu Makefile que compile o driver.c e o linke com todos os arquivos objeto dos seus escalonadores (schedule_rr.o, schedule_rr_p.o, schedule_edf.o, schedule_pa.o).

Exemplo de novo alvo no Makefile:

# ... (regras existentes para .o como driver.o, list.o, CPU.o, schedule_rr.o, etc., permanecem) ...

# Novo alvo para o programa com menu
scheduler_menu: driver.o list.o CPU.o schedule_rr.o schedule_rr_p.o schedule_edf.o schedule_pa.o
	$(CC) $(CFLAGS) -o scheduler_menu driver.o list.o CPU.o schedule_rr.o schedule_rr_p.o schedule_edf.o schedule_pa.o -pthread

# Adicione scheduler_menu à regra clean
clean:
	rm -rf *.o
	rm -rf rr_p 
	rm -rf edf
	rm -rf pa
	rm -rf rr
	rm -rf scheduler_menu # Adicionado

Nota: A flag -pthread é incluída porque pelo menos RR, RR_P e PA usam threads. Se EDF também usar, está coberto. Se não, não prejudica.

3. Como Compilar e Executar com o Menu:

Salve o driver.c modificado e o Makefile atualizado.

Compile:

make clean
make scheduler_menu

Execute:

./scheduler_menu nome_do_seu_arquivo_de_tarefas.txt

O programa então exibirá o menu, e você poderá escolher o algoritmo desejado. Lembre-se que se escolher EDF, seu arquivo de tarefas deve conter a coluna de deadline.

Conclusão:

Embora o método de compilação individual funcione, a implementação de um menu no driver.c é uma melhoria significativa em termos de usabilidade e eficiência para testar e demonstrar seu projeto. Recomenda-se adotar esta segunda abordagem.
