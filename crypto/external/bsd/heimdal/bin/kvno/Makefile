# $NetBSD: Makefile,v 1.1.46.1 2023/08/11 13:39:45 martin Exp $

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

PROGDPLIBS+= ${DPLIBKAFS} ${DPLIBSL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= -ledit -lterminfo ${KRB5LDADD}
DPADD+= ${LIBEDIT} ${LIBTERMINFO} ${KRB5DPADD}
