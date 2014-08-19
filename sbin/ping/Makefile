#	$NetBSD: Makefile,v 1.16.12.1 2014/08/20 00:02:27 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

USE_FORT?= yes	# setuid
RUMPPRG=ping
MAN=	ping.8
BINOWN=	root
BINMODE=4555
LDADD=	-lm
DPADD=	${LIBM}
CPPFLAGS+=	-DIPSEC
LDADD+= -lipsec
DPADD+= ${LIBIPSEC}

.if ${MACHINE_ARCH} == "vax"
COPTS.ping.c=-O0
.endif

.include <bsd.prog.mk>
