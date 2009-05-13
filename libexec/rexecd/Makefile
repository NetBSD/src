#	$NetBSD: Makefile,v 1.9.34.1 2009/05/13 19:18:42 jym Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

PROG=	rexecd
MAN=	rexecd.8

.if ${USE_PAM} != "no"

CPPFLAGS+= -DUSE_PAM -DPAM_DEBUG
LDADD+= -lpam ${PAM_STATIC_LDADD}
DPADD+=	${LIBPAM} ${PAM_STATIC_DPADD}

.else

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.endif

.include <bsd.prog.mk>
