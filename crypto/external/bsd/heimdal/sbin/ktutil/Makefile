# $NetBSD: Makefile,v 1.6 2023/06/22 22:39:39 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# network client

.PATH: ${HEIMDIST}/admin ${HEIMDIST}/lib/hcrypto

PROG= ktutil

MAN= ktutil.8

HEIMSRCS= ktutil-commands.in

SRCS=			\
	add.c		\
	change.c	\
	copy.c		\
	destroy.c	\
	get.c		\
	ktutil.c	\
	ktutil_locl.h	\
	list.c		\
	purge.c		\
	remove.c	\
	rename.c

PROGDPLIBS+= ${DPLIBKADM5CLNT} ${DPLIBSL}

ktutil.8: ktutil.1
	@${TOOL_SED} -e 's/KTUTIL 1/KTUTIL 8/' < ${.ALLSRC} > ${.TARGET}

CLEANFILES+=ktutil.8

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= -ledit -lterminfo -lutil ${KRB5LDADD}
DPADD+= ${LIBEDIT} ${LIBTERMINFO} ${LIBUTIL} ${KRB5DPADD}
