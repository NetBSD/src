# $NetBSD: Makefile,v 1.3.12.2 2020/04/21 19:37:55 martin Exp $

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

LDADD+= -lkafs -lsl
LDADD+= -ledit -lterminfo
DPADD+= ${LIBKAFS} ${LIBSL}
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
