# $NetBSD: Makefile,v 1.5 2023/06/20 17:23:05 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/kuser

PROG= kdigest
MAN= kdigest.8

HEIMSRCS+= kdigest.c kdigest-commands.in

PROGDPLIBS+= ${DPLIBHEIMNTLM} ${DPLIBSL}
LDADD+= -lcrypto -lutil -ledit -lterminfo
DPADD+= ${LIBCRYPTO} ${LIBUTIL} ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
