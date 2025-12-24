#	$NetBSD: Makefile,v 1.19 2025/12/24 20:06:00 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.init.mk>

USE_FORT?= yes	# setuid
RUMPPRG=ping
MAN=	ping.8
BINOWN=	root
BINMODE=4555
LDADD=	-lm
DPADD=	${LIBM}

.if (${MKIPSEC} != "no")
CPPFLAGS+=	-DIPSEC
LDADD+= -lipsec
DPADD+= ${LIBIPSEC}
.endif

.include <bsd.prog.mk>
