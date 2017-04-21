# $NetBSD: Makefile,v 1.2.32.1 2017/04/21 16:50:55 bouyer Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# network client

.PATH: ${HEIMDIST}/admin

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

LDADD+= -lkadm5clnt -lsl -ledit -lterminfo -lutil
DPADD+= ${LIBKADM5CLNT} ${LIBSL} ${LIBEDIT} ${LIBTERMINFO} ${LIBUTIL}

ktutil.8: ktutil.1
	@${TOOL_SED} -e 's/KTUTIL 1/KTUTIL 8/' < ${.ALLSRC} > ${.TARGET}

CLEANFILES+=ktutil.8

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
