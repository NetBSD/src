/*	$NetBSD: fdboot.h,v 1.2 1998/01/05 07:03:45 perry Exp $	*/

/***************************************************************
 *
 *	file: boot.h
 *
 *	author: chapuni(GBA02750@niftyserve.or.jp)
 *
 */

extern int RAW_READ __P((void *buf, int pos, size_t length));
extern unsigned B_KEYINP __P((void));
extern void B_CLR_ST __P((unsigned x));
extern void B_PUTC __P((unsigned c));
extern void B_PRINT __P((unsigned char *p));
extern unsigned B_COLOR __P((unsigned w));
extern unsigned B_LOCATE __P((unsigned x, unsigned y));
extern unsigned JISSFT __P((unsigned c));
extern unsigned short B_SFTSNS __P((void));
extern int SYS_STAT __P((int flags));
extern int getcpu __P((void));

/* eof */
