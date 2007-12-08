/*	$NetBSD: autoconf.h,v 1.7.2.2 2007/12/08 18:17:33 mjf Exp $	*/

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
void ofppc_init_comcons(int);
void copy_disp_props(struct device *, int, prop_dictionary_t);

int rascons_cnattach(void);
#endif /* _KERNEL */

#endif /* _OFPPC_AUTOCONF_H_ */
