#ifndef _MACHINE_AUTOCONF_H_
#define _MACHINE_AUTOCONF_H_

#ifdef _KERNEL
void initppc (u_int, u_int, u_int, void *);
void install_extint (void (*)(void)); 
void dumpsys (void);
void softnet (int);
void strayintr (int);
void *mapiodev (paddr_t, psize_t);
void lcsplx (int);

void inittodr (time_t);
void resettodr (void);
void cpu_initclocks (void);
void decr_intr (struct clockframe *);
void setstatclockrate (int);

int pfb_cnattach (int);			/* pci/pfb.c */
#endif /* _KERNEL */

#endif /* _MACHINE_AUTOCONF_H_ */
