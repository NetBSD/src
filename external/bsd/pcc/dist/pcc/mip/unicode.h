/*	Id: unicode.h,v 1.5 2015/07/19 13:20:37 ragge Exp 	*/	
/*	$NetBSD: unicode.h,v 1.1.1.2 2016/02/09 20:29:20 plunky Exp $	*/

#ifndef _UNICODE_H
#define _UNICODE_H

extern long u82cp(char **q);
extern void u8error(const char *fmt, ...);
extern void cp2u16(long num, unsigned short *s);

#endif
