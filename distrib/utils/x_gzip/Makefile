#	$NetBSD: Makefile,v 1.3.4.1 2001/07/29 20:32:31 he Exp $

PROG=	gzip
LDADD=	-lz
DPADD=	${LIBZ}
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDSTATIC?=-static

.include <bsd.prog.mk>
