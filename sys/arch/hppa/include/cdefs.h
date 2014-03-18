/*	$NetBSD: cdefs.h,v 1.3 2014/03/18 17:11:19 christos Exp $	*/

#ifndef	_HPPA_CDEFS_H_
#define	_HPPA_CDEFS_H_

/*
 * the hppa assembler uses ; as the comment character,
 * so we can't let sys/cdefs.h use it as an assembler
 * statement delimiter.
 */
#define __ASM_DELIMITER "\n\t"

#define	__ALIGNBYTES	((size_t)7)

#endif /* !_HPPA_CDEFS_H_ */
