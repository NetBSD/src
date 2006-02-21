# $NetBSD: Makefile,v 1.1 2006/02/21 16:36:57 drochner Exp $

NOMAN=		# defined

PROG=		subnormal
LDADD+=		-lm

#CPPFLAGS+=	-DUSE_FLOAT

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
