/*	$NetBSD: mpacpi.h,v 1.1.2.2 2003/01/07 21:11:47 thorpej Exp $	*/

#ifndef _I386_MPACPI_H
#define _I386_MPACPI_H

int mpacpi_scan_apics(struct device *);
int mpacpi_find_interrupts(void);

#endif /* _I386_MPACPI_H */
