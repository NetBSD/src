#ifndef _ARM_OMAP_OMAP_VAR_H_
#define _ARM_OMAP_OMAP_VAR_H_

#include <sys/types.h>
#include <sys/bus.h>

/* Texas Instruments OMAP generic */


extern struct bus_space omap_bs_tag;
extern struct arm32_bus_dma_tag omap_bus_dma_tag;
extern struct bus_space omap_a4x_bs_tag;
extern struct bus_space omap_a2x_bs_tag;
extern struct bus_space nobyteacc_bs_tag;

/* platform needs to provide this */
bus_dma_tag_t omap_bus_dma_init(struct arm32_bus_dma_tag *);

#endif /* _ARM_OMAP_OMAP_VAR_H_ */
