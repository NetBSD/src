#	$NetBSD: Makefile,v 1.4 2003/10/24 15:54:46 kleink Exp $

NOMAN=		# defined

PROG=		nan

# the next two lines are required for _isnanl(), which is internal to libc
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/include
CPPFLAGS+=	-D_LIBC

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
