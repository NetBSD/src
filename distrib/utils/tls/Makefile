# $NetBSD: Makefile,v 1.6 2001/10/05 01:17:29 jmc Exp $
# Tiny ls  (i.e. for boot media)

PROG=	tls
MKMAN=	no
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>
