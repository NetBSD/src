/*	$NetBSD: cdefs.h,v 1.2 2012/01/20 14:08:06 joerg Exp $	*/

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

/* We're elf only: inspected by sys/cdefs.h  */
#ifndef __ELF__
#define __ELF__
#endif

#define	__ALIGNBYTES		15

#endif /* !_MACHINE_CDEFS_H_ */
