#ifndef _MACHINE_AUTOCONF_H_
#define _MACHINE_AUTOCONF_H_

#ifdef _KERNEL
void initppc (u_int, u_int, char *);
void install_extint (void (*)(void)); 
void dumpsys (void);
void softnet (void);
void strayintr (int);

void inittodr (time_t);
void resettodr (void);
void cpu_initclocks (void);
void decr_intr (struct clockframe *);
void setstatclockrate (int);

#ifdef __BROKEN_DK_ESTABLISH
void dk_cleanup(void);
#endif

#endif /* _KERNEL */

#endif /* _MACHINE_AUTOCONF_H_ */
