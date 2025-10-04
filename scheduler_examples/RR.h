#ifndef SCHEDULING_RR_H
#define SCHEDULING_RR_H

#include "queue.h"

void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task);


#endif //SCHEDULING_RR_H
