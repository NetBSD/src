#ifndef _MACHINE_GTENVAR_H
#define	_MACHINE_GTENVAR_H

struct rasops_info;

struct gten_softc {
	struct device gt_dev;

	struct rasops_info *gt_ri;
	paddr_t gt_paddr;
	bus_size_t gt_memsize;
	bus_addr_t gt_memaddr;
	int gt_nscreens;
	u_char gt_cmap_red[256];
	u_char gt_cmap_green[256];
	u_char gt_cmap_blue[256];
};

int     gten_cnattach(bus_space_tag_t);

#endif /* _MACHINE_GTENVAR_H_ */
