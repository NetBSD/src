#	$NetBSD: Makefile,v 1.18 2003/10/26 07:25:34 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

NOMAN=  	# defined

PROG=		dbtest

# add -DSTATISTICS to CPPFLAGS to get usage statistics.  Note that
# for this to work, libc must be compiled with -DSTATISTICS as well
CPPFLAGS+=	-g -D__DBINTERFACE_PRIVATE -DDEBUG
CLEANFILES+=	t1 t2 t3

regress: dbtest
	${HOST_SH} ${.CURDIR}/run.test

.include <bsd.prog.mk>
