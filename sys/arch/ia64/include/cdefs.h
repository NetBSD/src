/*	$NetBSD: cdefs.h,v 1.2.10.1 2014/05/18 17:45:13 rmind Exp $	*/

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

/* We're elf only: inspected by sys/cdefs.h  */
#ifndef __ELF__
#define __ELF__
#endif

#define	__ALIGNBYTES		((size_t)15)

#endif /* !_MACHINE_CDEFS_H_ */
