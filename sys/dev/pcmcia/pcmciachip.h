#ifndef _PCMCIA_PCMCIACHIP_H_
#define _PCMCIA_PCMCIACHIP_H_

#include <machine/bus.h>

struct pcmcia_mem_handle;
struct pcmcia_io_handle;

/* interfaces for pcmcia to call the chipset */

typedef struct pcmcia_chip_functions *pcmcia_chipset_tag_t;
typedef void *pcmcia_chipset_handle_t;
typedef int pcmcia_mem_handle_t;

#define PCMCIA_MEM_ATTR		1
#define PCMCIA_MEM_COMMON	2

#define PCMCIA_WIDTH_IO8	1
#define PCMCIA_WIDTH_IO16	2

struct pcmcia_chip_functions {
    /* memory space allocation */
    int (*mem_alloc) __P((pcmcia_chipset_handle_t, bus_size_t,
			  struct pcmcia_mem_handle *));
    void (*mem_free) __P((pcmcia_chipset_handle_t, 
			  struct pcmcia_mem_handle *));

    /* memory space window mapping */
    int (*mem_map) __P((pcmcia_chipset_handle_t, int, bus_addr_t,
			bus_size_t, struct pcmcia_mem_handle *,
			bus_addr_t *, int *));
    void (*mem_unmap) __P((pcmcia_chipset_handle_t, int));

    /* I/O space allocation */
    int (*io_alloc) __P((pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
			 struct pcmcia_io_handle *));
    void (*io_free) __P((pcmcia_chipset_handle_t,
			 struct pcmcia_io_handle *));

    /* I/O space window mapping */
    int (*io_map) __P((pcmcia_chipset_handle_t, int, bus_addr_t,
		       bus_size_t, struct pcmcia_io_handle *, int *));
    void (*io_unmap) __P((pcmcia_chipset_handle_t, int));

    /* interrupt glue */
    void *(*intr_establish) __P((pcmcia_chipset_handle_t, u_int16_t, int,
				 int (*)(void *), void *));
    void (*intr_disestablish) __P((pcmcia_chipset_handle_t, void *));
};

#define pcmcia_chip_mem_alloc(tag, handle, size, pcmhp) \
	((*(tag)->mem_alloc)((handle), (size), (pcmhp)))
#define pcmcia_chip_mem_free(tag, handle, pcmhp) \
	((*(tag)->mem_free)((handle), (pcmhp)))
#define pcmcia_chip_mem_map(tag, handle, kind, card_addr, size, pcmhp, \
			    offsetp, windowp) \
	((*(tag)->mem_map)((handle), (kind), (card_addr), (size), (pcmhp), \
			   (offsetp), (windowp)))
#define pcmcia_chip_mem_unmap(tag, handle, window) \
	((*(tag)->mem_unmap)((handle), (window)))

#define pcmcia_chip_io_alloc(tag, handle, start, size, pcihp) \
	((*(tag)->io_alloc)((handle), (start), (size), (pcihp)))
#define pcmcia_chip_io_free(tag, handle, pcihp) \
	((*(tag)->io_free)((handle), (pcihp)))
#define pcmcia_chip_io_map(tag, handle, width, card_addr, size, pcihp, \
			   windowp) \
	((*(tag)->io_map)((handle), (width), (card_addr), (size), (pcihp), \
			  (windowp)))
#define pcmcia_chip_io_unmap(tag, handle, window) \
	((*(tag)->io_unmap)((handle), (window)))

#define pcmcia_chip_intr_establish(tag, handle, irqmask, ipl, fct, arg) \
	((*(tag)->intr_establish)((handle), (irqmask), (ipl), (fct), (arg)))
#define pcmcia_chip_intr_disestablish(tag, handle, ih) \
	((*(tag)->intr_disestablish)((handle), (ih)))

struct pcmciabus_attach_args {
    pcmcia_chipset_tag_t pct;
    pcmcia_chipset_handle_t pch;
};

/* interfaces for the chipset to call pcmcia */

int pcmcia_attach_card __P((struct device *, int *));
void pcmcia_detach_card __P((struct device *));

#endif /* _PCMCIA_PCMCIACHIP_H_ */
