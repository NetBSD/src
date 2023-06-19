# $NetBSD: Makefile,v 1.2 2023/06/19 23:11:43 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser ${HEIMDIST}/lib/hcrypto

PROG= kvno


#HEIMSRCS= kcc-commands.in

SRCS=	kvno.c ui.c
MAN= kvno.1

CPPFLAGS+= -I${DESTDIR}/usr/include/krb5 -I${HEIMDIST}/lib
COPTS.kvno.c+= -Wno-deprecated-declarations

LDADD+= -lkafs -lsl
LDADD+= -ledit -lterminfo
DPADD+= ${LIBKAFS} ${LIBSL}
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
