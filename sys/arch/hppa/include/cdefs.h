/*	$NetBSD: cdefs.h,v 1.1.150.1 2012/04/17 00:06:26 yamt Exp $	*/

#ifndef	_HPPA_CDEFS_H_
#define	_HPPA_CDEFS_H_

/*
 * the hppa assembler uses ; as the comment character,
 * so we can't let sys/cdefs.h use it as an assembler
 * statement delimiter.
 */
#define __ASM_DELIMITER "\n\t"

#define	__ALIGNBYTES	7

#endif /* !_HPPA_CDEFS_H_ */
