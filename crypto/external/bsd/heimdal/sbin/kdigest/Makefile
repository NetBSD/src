# $NetBSD: Makefile,v 1.6 2023/06/22 22:39:38 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/kuser

PROG= kdigest
MAN= kdigest.8

HEIMSRCS+= kdigest.c kdigest-commands.in

PROGDPLIBS+= ${DPLIBHEIMNTLM} ${DPLIBSL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= -lutil -ledit -lterminfo ${KRB5LDADD}
DPADD+= ${LIBUTIL} ${LIBEDIT} ${LIBTERMINFO} ${KRB5DPADD}
