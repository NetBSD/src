/*	$NetBSD: cdefs.h,v 1.1.4.2 2002/07/14 17:47:19 gehenna Exp $	*/

#ifndef	_HPPA_CDEFS_H_
#define	_HPPA_CDEFS_H_

/*
 * the hppa assembler uses ; as the comment character,
 * so we can't let sys/cdefs.h use it as an assembler
 * statement delimiter.
 */
#define __ASM_DELIMITER "\n\t"

#endif /* !_HPPA_CDEFS_H_ */
