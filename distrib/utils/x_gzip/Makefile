#	$NetBSD: Makefile,v 1.5 2001/10/05 01:17:30 jmc Exp $

PROG=	gzip
LDADD=	-lz
DPADD=	${LIBZ}
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
LDSTATIC?=-static

.include <bsd.prog.mk>
