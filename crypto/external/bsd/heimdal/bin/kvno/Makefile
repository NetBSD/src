# $NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:45:15 tls Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kvno


#HEIMSRCS= kcc-commands.in

SRCS=	kvno.c
MAN= kvno.1

CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
COPTS.kvno.c+= -Wno-deprecated-declarations

LDADD+= -lkafs -lsl
LDADD+= -ledit -lterminfo
DPADD+= ${LIBKAFS} ${LIBSL}
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
