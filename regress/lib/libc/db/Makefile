#	$NetBSD: Makefile,v 1.12.2.1 1997/11/08 07:39:36 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	dbtest

# add -DSTATISTICS to CPPFLAGS to get usage statistics.  Note that
# for this to work, libc must be compiled with -DSTATISTICS as well
CPPFLAGS+= -g -D__DBINTERFACE_PRIVATE -DDEBUG
NOMAN=  noman
CLEANFILES+= t1 t2 t3

regress:
	sh ${.CURDIR}/run.test

.include <bsd.prog.mk>
