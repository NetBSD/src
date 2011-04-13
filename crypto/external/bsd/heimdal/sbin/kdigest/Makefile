# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:59 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/kuser

PROG= kdigest
# no man-page yet
MAN=

HEIMSRCS+= kdigest.c kdigest-commands.in

LDADD+=	-lkrb5 -lhx509 -lheimntlm -lcrypto -lasn1 -lcom_err -lsl \
	${LIBVERS} -lroken -lcrypt -lutil -ledit -lterminfo
DPADD+=	${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${LIBSL} ${LIBVERS} ${LIBROKEN} ${LIBCRYPT} \
	${LIBUTIL} ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
