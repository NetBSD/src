#ifndef _PCMCIA_PCMCIACHIP_H_
#define _PCMCIA_PCMCIACHIP_H_

#include <machine/bus.h>

/* interfaces for pcmcia to call the chipset */

typedef struct pcmcia_chip_functions *pcmcia_chipset_tag_t;
typedef void *pcmcia_chipset_handle_t;
typedef int pcmcia_mem_handle_t;

#define PCMCIA_MEM_ATTR		1
#define PCMCIA_MEM_COMMON	2

#define PCMCIA_WIDTH_IO8	1
#define PCMCIA_WIDTH_IO16	2

struct pcmcia_chip_functions {
    /* XXX alloc/free should probably be somewhere more generic than the
       pcmcia driver */
    int (*mem_alloc) __P((pcmcia_chipset_handle_t, bus_size_t,
			  bus_space_tag_t *, bus_space_handle_t *,
			  pcmcia_mem_handle_t *, bus_size_t *));
    void (*mem_free) __P((pcmcia_chipset_handle_t, bus_size_t,
			 bus_space_tag_t, bus_space_handle_t,
			  pcmcia_mem_handle_t));
    int (*mem_map) __P((pcmcia_chipset_handle_t, int,
			bus_size_t, bus_space_tag_t, bus_space_handle_t,
			u_long, u_long *, int *));
    void (*mem_unmap) __P((pcmcia_chipset_handle_t, int));

    int (*io_alloc) __P((pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
			  bus_space_tag_t *, bus_space_handle_t *));
    void (*io_free) __P((pcmcia_chipset_handle_t, bus_size_t,
			 bus_space_tag_t, bus_space_handle_t));
    int (*io_map) __P((pcmcia_chipset_handle_t, int, bus_size_t,
		       bus_space_tag_t, bus_space_handle_t, int *));
    void (*io_unmap) __P((pcmcia_chipset_handle_t, int));

    void *(*intr_establish) __P((pcmcia_chipset_handle_t, int,
				 int (*)(void *), void *));
    void (*intr_disestablish) __P((pcmcia_chipset_handle_t, void *));
};

#define pcmcia_chip_mem_alloc(tag, handle, size, memtp, memhp, mhandle, \
			      realsize) \
	((*(tag)->mem_alloc)((handle), (size), (memtp), (memhp), (mhandle), \
			     (realsize)))
#define pcmcia_chip_mem_free(tag, handle, size, memt, memh, mhandle) \
	((*(tag)->mem_free)((handle), (size), (memt), (memh), (mhandle)))
#define pcmcia_chip_mem_map(tag, handle, kind, size, memt, memh, \
			    card_addr, offsetp, windowp) \
	((*(tag)->mem_map)((handle), (kind), (size), (memt), (memh), \
			   (card_addr), (offsetp), (windowp)))
#define pcmcia_chip_mem_unmap(tag, handle, window) \
	((*(tag)->mem_unmap)((handle), (window)))

#define pcmcia_chip_io_alloc(tag, handle, start, size, iotp, iohp) \
	((*(tag)->io_alloc)((handle), (start), (size), (iotp), (iohp)))
#define pcmcia_chip_io_free(tag, handle, size, iot, ioh) \
	((*(tag)->io_free)((handle), (size), (iot), (ioh)))
#define pcmcia_chip_io_map(tag, handle, width, size, iot, ioh, windowp) \
	((*(tag)->io_map)((handle), (width), (size), (iot), (ioh), (windowp)))
#define pcmcia_chip_io_unmap(tag, handle, window) \
	((*(tag)->io_unmap)((handle), (window)))

#define pcmcia_chip_intr_establish(tag, handle, ipl, fct, arg) \
	((*(tag)->intr_establish)((handle), (ipl), (fct), (arg)))
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
