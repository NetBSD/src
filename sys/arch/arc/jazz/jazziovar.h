#ifndef __JAZZIOVAR_H
#define __JAZZIOVAR_H

#include <machine/bus.h>

struct jazzio_attach_args {
	char	*ja_name;
	struct abus *ja_bus;
	bus_space_tag_t	ja_bust;
	bus_dma_tag_t	ja_dmat;
	bus_addr_t	ja_addr;
	int		ja_intr;
	int		ja_dma;
};

void	jazzio_intr_establish(int, int (*)(void *), void *);
void	jazzio_intr_disestablish(int);

#endif
