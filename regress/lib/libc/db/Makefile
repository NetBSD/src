#	$NetBSD: Makefile,v 1.17 2002/09/18 05:41:37 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

NOMAN=  	# defined

PROG=		dbtest

# add -DSTATISTICS to CPPFLAGS to get usage statistics.  Note that
# for this to work, libc must be compiled with -DSTATISTICS as well
CPPFLAGS+=	-g -D__DBINTERFACE_PRIVATE -DDEBUG
CLEANFILES+=	t1 t2 t3

regress: dbtest
	sh ${.CURDIR}/run.test

.include <bsd.prog.mk>
