/*	$NetBSD: autoconf.h,v 1.13.36.1 2011/06/23 14:19:25 cherry Exp $	*/

#ifndef _OFPPC_AUTOCONF_H_
#define _OFPPC_AUTOCONF_H_

#include <machine/bus.h>

struct confargs {
	const char	*ca_name;
	u_int		ca_node;
	int		ca_nreg;
	u_int		*ca_reg;
	int		ca_nintr;
	int		*ca_intr;

	bus_addr_t	ca_baseaddr;
	bus_space_tag_t	ca_tag;
};

struct pciio_info {
	uint32_t	start;
	uint32_t	limit;
};

/* to support machines with more than 4 busses, change the below */
#define MAX_PCI_BUSSES		4
struct model_data {
	int			ranges_offset;
	struct pciio_info	pciiodata[MAX_PCI_BUSSES];
};

extern int console_node;
extern char model_name[64];

#ifdef _KERNEL
void initppc(u_int, u_int, char *);
void model_init(void);
void strayintr(int);
void dumpsys(void);

void inittodr(time_t);
void resettodr(void);
void cpu_initclocks(void);
void decr_intr(struct clockframe *);
void setstatclockrate(int);
void init_interrupt(void);
void init_ofppc_interrupt(void);
void ofppc_init_comcons(int);
void copy_disp_props(device_t, int, prop_dictionary_t);

void OF_start_cpu(int, u_int, int);

int rascons_cnattach(void);
#endif /* _KERNEL */

#endif /* _OFPPC_AUTOCONF_H_ */
