#	$NetBSD: Makefile,v 1.15 2001/03/11 16:40:40 bjh21 Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	dbtest

# add -DSTATISTICS to CPPFLAGS to get usage statistics.  Note that
# for this to work, libc must be compiled with -DSTATISTICS as well
CPPFLAGS+= -g -D__DBINTERFACE_PRIVATE -DDEBUG
MKMAN=  no
CLEANFILES+= t1 t2 t3

regress: dbtest
	sh ${.CURDIR}/run.test

.include <bsd.prog.mk>
