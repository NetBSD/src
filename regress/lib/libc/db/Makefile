#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$Id: Makefile,v 1.8 1994/01/24 03:50:04 cgd Exp $

PROG=	dbtest

# add -DSTATISTICS to CFLAGS to get usage statistics.  Note that
# for this to work, libc must be compiled with -DSTATISTICS as well
CFLAGS= -g -DDEBUG
NOMAN=  noman
CLEANFILES+= t1 t2 t3

install:

regress:
	sh ${.CURDIR}/run.test

.include <bsd.prog.mk>
