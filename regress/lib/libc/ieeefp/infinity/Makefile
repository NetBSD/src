#	$NetBSD: Makefile,v 1.4 2003/10/24 16:35:08 kleink Exp $

NOMAN=		# defined

PROG=		infinity

# the next two lines are required for _isinfl(), which is internal to libc
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/include
CPPFLAGS+=	-D_LIBC

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
