# $NetBSD: Makefile,v 1.1.20.1 2009/05/11 19:20:34 bouyer Exp $

WARNS=	0	# Will be fixed later

.include <bsd.own.mk>

PROG=	racoonctl
SRCS=	racoonctl.c str2val.c kmpstat.c vmbuf.c sockmisc.c misc.c
MAN=	racoonctl.8

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools
CFLAGS+=-g
CPPFLAGS+=	-I${DIST}/src/racoon -I${DIST}/src/libipsec	\
		-I${NETBSDSRCDIR}/lib/libipsec 			\
		-DIPSEC_DEBUG -DHAVE_CONFIG_H 			\
		-DADMINPORTDIR=\"/var/run\"
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}
CLEANFILES+=	racoonctl.cat8

.PATH:  ${NETBSDSRCDIR}/lib/libipsec ${DIST}/src/racoon

.include <bsd.prog.mk>

