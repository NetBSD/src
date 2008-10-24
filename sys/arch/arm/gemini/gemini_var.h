#ifndef _ARM_GEMINI_VAR_H_
#define _ARM_GEMINI_VAR_H_

#include <sys/types.h>
#include <machine/bus.h>

/* GEMINI generic */


extern struct bus_space gemini_bs_tag;
extern struct arm32_bus_dma_tag gemini_bus_dma_tag;
extern struct bus_space gemini_a4x_bs_tag;
extern struct bus_space gemini_a2x_bs_tag;
extern struct bus_space nobyteacc_bs_tag;


/* platform needs to provide this */
extern bus_dma_tag_t gemini_bus_dma_init(struct arm32_bus_dma_tag *);

#endif /* _ARM_GEMINI_VAR_H_ */
