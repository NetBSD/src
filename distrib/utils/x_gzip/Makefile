#	$NetBSD: Makefile,v 1.4 2001/08/20 12:19:45 martin Exp $

PROG=	gzip
LDADD=	-lz
DPADD=	${LIBZ}
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDSTATIC?=-static

.include <bsd.prog.mk>
