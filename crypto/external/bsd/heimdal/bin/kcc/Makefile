# $NetBSD: Makefile,v 1.5 2023/06/20 17:23:01 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kcc
LINKS+= ${BINDIR}/kcc ${BINDIR}/klist

MAN= klist.1

HEIMSRCS= heimtools-commands.in

SRCS=	copy_cred_cache.c	\
	heimtools.c		\
	klist.c			\
	kswitch.c

CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
CPPFLAGS+= -I${HEIMDIST}/kuser

PROGDPLIBS+= ${DPLIBASN1} ${DPLIBKAFS} ${DPLIBSL}
LDADD+= -ledit -lterminfo
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
