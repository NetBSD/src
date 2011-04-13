# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:45 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kcc
LINKS+= ${BINDIR}/kcc ${BINDIR}/klist

MAN= klist.1

HEIMSRCS= kcc-commands.in

SRCS=	copy_cred_cache.c	\
	kcc.c			\
	klist.c			\
	kswitch.c

CPPFLAGS+= -I${DESTDIR}/usr/include/krb5

LDADD+=	-lkrb5 -lkafs -lsl -lheimbase ${LIBVERS}
DPADD+=	${LIBKRB5} ${LIBKAFS} ${LIBSL} ${LIBVERS}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
