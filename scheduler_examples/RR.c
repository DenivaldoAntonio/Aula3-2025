#include "RR.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>

#define QUANTUM 500   // tempo de quantum em milissegundos


void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    // Se o CPU tem um processo a correr
    if (*cpu_task) {
        // Atualiza o tempo já gasto por este processo
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // Verifica se já passou o quantum desde que começou a rodar
        if (current_time_ms - (*cpu_task)->slice_start_ms >= QUANTUM) {

            // CASO 1: o processo ainda não terminou
            if ((*cpu_task)->ellapsed_time_ms < (*cpu_task)->time_ms) {
                // Reencolamos o processo no fim da fila (vai esperar a sua vez de novo)
                enqueue_pcb(rq, *cpu_task);

                // O CPU fica livre para outro processo
                *cpu_task = NULL;
            }
                // CASO 2: o processo terminou dentro do quantum
            else {
                // Criamos mensagem DONE para avisar a aplicação
                msg_t msg = {
                        .pid = (*cpu_task)->pid,
                        .request = PROCESS_REQUEST_DONE,
                        .time_ms = current_time_ms
                };
                if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                    perror("write");
                }

                // Libertamos a memória do processo porque já terminou
                free(*cpu_task);

                // CPU fica livre
                *cpu_task = NULL;
            }
        }
    }

    // Se o CPU está livre, escolhemos o próximo da fila de prontos
    if (*cpu_task == NULL) {
        // Retiramos um processo da fila
        *cpu_task = dequeue_pcb(rq);

        // Se realmente havia um processo na fila
        if (*cpu_task) {
            // Guardamos o instante em que começou a rodar,
            // para depois medir quando o seu quantum expira
            (*cpu_task)->slice_start_ms = current_time_ms;
        }
    }
}
