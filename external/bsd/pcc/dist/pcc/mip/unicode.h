/*	Id: unicode.h,v 1.3 2014/06/06 15:31:56 plunky Exp 	*/	
/*	$NetBSD: unicode.h,v 1.1.1.1.2.2 2014/08/10 07:10:07 tls Exp $	*/

#ifndef _UNICODE_H
#define _UNICODE_H

#if 0
extern char *cp2u8(char *p,unsigned int c);
#endif
extern unsigned int u82cp(char **q);
extern int u8len(char *t);
extern unsigned int esc2char(char **q);
extern void u8error(const char *fmt, ...);

#endif
