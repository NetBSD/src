# $NetBSD: Makefile,v 1.4.4.1 2001/07/29 20:32:30 he Exp $
# Tiny ls  (i.e. for boot media)

PROG=	tls
MKMAN=	no
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>
