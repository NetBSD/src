#	$NetBSD: Makefile,v 1.8 2005/02/23 01:25:50 christos Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

WARNS=3
PROG=	rexecd
MAN=	rexecd.8

.if ${USE_PAM} != "no"

CPPFLAGS+= -DUSE_PAM -DPAM_DEBUG
LDADD+= -lpam
DPADD+=	${LIBPAM}

.else

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.endif

.include <bsd.prog.mk>
