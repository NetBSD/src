/*	$NetBSD: cdefs.h,v 1.1.116.1 2012/02/18 07:32:25 mrg Exp $	*/

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

/* We're elf only: inspected by sys/cdefs.h  */
#ifndef __ELF__
#define __ELF__
#endif

#define	__ALIGNBYTES		15

#endif /* !_MACHINE_CDEFS_H_ */
