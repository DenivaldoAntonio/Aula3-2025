#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>


/**
 * @brief Shortest Job First (SJF) scheduling algorithm.
 *
 * Este algoritmo escolhe sempre a tarefa com o menor tempo de execução (`time_ms`)
 * da fila de prontos (ready queue).
 *
 * @param current_time_ms Tempo atual em milissegundos.
 * @param rq Ponteiro para a ready queue contendo as tarefas prontas a executar.
 * @param cpu_task Ponteiro duplo para a tarefa atualmente no CPU.
 */
void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Task terminou
            msg_t msg = {
                    .pid = (*cpu_task)->pid,
                    .request = PROCESS_REQUEST_DONE,
                    .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free((*cpu_task));
            *cpu_task = NULL;
        }
    }

    if (*cpu_task == NULL) {
        // Se o CPU está livre, precisamos escolher uma nova tarefa

        // Começamos a percorrer a fila de prontos (ready queue) a partir do início
        queue_elem_t *elem = rq->head;

        // Ponteiro para guardar o elemento ANTERIOR ao mais curto encontrado (para remoção da lista)
        queue_elem_t *shortest_prev = NULL;

        // Ponteiro para o elemento que tem o processo mais curto encontrado até agora
        queue_elem_t *shortest_elem = NULL;

        // Inicializamos "min_time" com o maior valor possível, para depois poder comparar
        uint32_t min_time = UINT32_MAX;

        // Ponteiro auxiliar para ir acompanhando o anterior enquanto percorremos a lista
        queue_elem_t *prev = NULL;

        // Percorremos todos os elementos da ready queue
        for (; elem != NULL; prev = elem, elem = elem->next) {

            // Se o tempo deste processo é menor que o menor que vimos até agora
            if (elem->pcb->time_ms < min_time) {
                // Atualizamos o tempo mínimo
                min_time = elem->pcb->time_ms;

                // Guardamos este elemento como o mais curto até agora
                shortest_elem = elem;

                // Guardamos também o elemento anterior, para poder removê-lo corretamente depois
                shortest_prev = prev;
            }
        }


        if (shortest_elem) {
            // Retirar da fila
            if (shortest_prev) {
                shortest_prev->next = shortest_elem->next;
            } else {
                rq->head = shortest_elem->next;
            }
            if (shortest_elem == rq->tail) {
                rq->tail = shortest_prev;
            }

            *cpu_task = shortest_elem->pcb;
            free(shortest_elem);
        }
    }
}
