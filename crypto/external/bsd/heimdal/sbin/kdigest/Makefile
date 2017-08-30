# $NetBSD: Makefile,v 1.2.12.1 2017/08/30 06:54:38 snj Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/kuser

PROG= kdigest
MAN= kdigest.8

HEIMSRCS+= kdigest.c kdigest-commands.in

LDADD+= -lheimntlm -lsl -lutil -ledit -lterminfo
DPADD+= ${LIBHEIMNTLM} ${LIBSL} ${LIBUTIL} ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
