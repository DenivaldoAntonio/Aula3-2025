#include "mlfq.h"

#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include <unistd.h>

#define NUM_QUEUES 7    // Total de níveis de prioridade (0 é o mais alto, 6 é o mais baixo)
#define QUANTUM 500     // Tempo máximo que cada processo pode usar o CPU por vez (em milissegundos)


void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    // Criamos 7 filas internas para os diferentes níveis de prioridade
    static queue_t queues[NUM_QUEUES] = {{0}};
    static int initialized = 0;

    // Inicialização das filas (só na primeira chamada)
    if (!initialized) {
        for (int i = 0; i < NUM_QUEUES; i++) {
            queues[i].head = queues[i].tail = NULL;  // cada fila começa vazia
        }
        initialized = 1;
    }

    // Mover processos novos da fila de entrada (rq) para a fila 0

    queue_elem_t *elem = rq->head;
    while (elem) {
        elem->pcb->priority_level = 0;                   // todos os novos processos começam com prioridade máxima
        enqueue_pcb(&queues[0], elem->pcb);     // colocamos na fila 0 (nível mais alto)
        elem = elem->next;                             // avança para o próximo elemento da fila de entrada
    }
    rq->head = rq->tail = NULL;                      // a fila principal (rq) fica vazia após o movimento

    // Atualização do processo que está atualmente no CPU

    if (*cpu_task) {
        // Incrementa o tempo total que este processo já usou o CPU
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // Caso o processo tenha completado todo o seu tempo de execução
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Envia mensagem de conclusão (DONE) para o cliente/processo
            msg_t msg = {
                    .pid = (*cpu_task)->pid,
                    .request = PROCESS_REQUEST_DONE,
                    .time_ms = current_time_ms
            };
            write((*cpu_task)->sockfd, &msg, sizeof(msg_t));

            // Liberta o PCB da memória — o processo terminou
            free(*cpu_task);
            *cpu_task = NULL;                   // CPU fica livre
        }

            // Caso tenha gasto o time-slice de 500 ms e ainda não tenha terminado
        else if (current_time_ms - (*cpu_task)->slice_start_ms >= QUANTUM) {
            // Calcula o novo nível de prioridade (rebaixa o processo)
            int new_level = (*cpu_task)->priority_level + 1;

            // Garante que o processo não desça além da última fila
            if (new_level >= NUM_QUEUES) {
                new_level = NUM_QUEUES - 1;
            }

            // Atualiza a prioridade do processo
            (*cpu_task)->priority_level = new_level;

            // Reencola o processo na fila correspondente ao novo nível
            enqueue_pcb(&queues[new_level], *cpu_task);

            // Liberta o CPU (o processo voltará a esperar sua vez)
            *cpu_task = NULL;
        }
    }

    // ------------------------------------------------------------------
    // Escolha do próximo processo para o CPU
    // ------------------------------------------------------------------
    if (*cpu_task == NULL) {
        // Percorre as filas da mais prioritária (0) para a menos (6)
        for (int i = 0; i < NUM_QUEUES; i++) {
            if (queues[i].head != NULL) {              // encontrou fila não vazia
                *cpu_task = dequeue_pcb(&queues[i]);   // retira o primeiro processo dessa fila
                if (*cpu_task) {
                    // Regista o instante em que começou o novo time-slice
                    (*cpu_task)->slice_start_ms = current_time_ms;

                    // Guarda o nível de prioridade atual (para referência)
                    (*cpu_task)->priority_level = i;
                }
                break; // sai do ciclo após escolher um processo
            }
        }
    }
}
