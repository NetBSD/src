/*      $NetBSD: footbridge.h,v 1.2.134.1 2009/05/13 17:16:13 jym Exp $  */

#ifndef _FOOTBRIDGE_H_
#define _FOOTBRIDGE_H_

#include <sys/termios.h>
#include <arm/bus.h>
void footbridge_pci_bs_tag_init(void);
void footbridge_sa110_cc_setup(void);
void footbridge_create_io_bs_tag(struct bus_space *, void *);
void footbridge_create_mem_bs_tag(struct bus_space *, void *);
int fcomcnattach(u_int, int, tcflag_t);
int fcomcndetach(void);
void calibrate_delay(void);

#endif
