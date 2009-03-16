#	$NetBSD: Makefile,v 1.10 2009/03/16 02:24:56 lukem Exp $
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
