/*	$NetBSD: autoconf.h,v 1.5 2002/09/18 01:44:13 chs Exp $	*/

#ifndef _OFPPC_AUTOCONF_H_
#define _OFPPC_AUTOCONF_H_

struct confargs {
	const char	*ca_name;
	u_int		ca_node;
	int		ca_nreg;
	u_int		*ca_reg;
	int		ca_nintr;
	int		*ca_intr;

	u_int		ca_baseaddr;
	/* bus_space_tag_t ca_tag; */
};

#ifdef _KERNEL
void initppc(u_int, u_int, char *);
void strayintr(int);

void inittodr(time_t);
void resettodr(void);
void cpu_initclocks(void);
void decr_intr(struct clockframe *);
void setstatclockrate(int);
#endif /* _KERNEL */

#endif /* _OFPPC_AUTOCONF_H_ */
