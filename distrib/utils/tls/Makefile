# $NetBSD: Makefile,v 1.5 2001/08/20 12:19:44 martin Exp $
# Tiny ls  (i.e. for boot media)

PROG=	tls
MKMAN=	no
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>
