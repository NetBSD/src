# $NetBSD: Makefile,v 1.4 2023/06/19 23:11:45 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# network client

.PATH: ${HEIMDIST}/admin ${HEIMDIST}/lib/hcrypto

PROG= ktutil

MAN= ktutil.8

HEIMSRCS= ktutil-commands.in
CPPFLAGS+=-I${HEIMDIST}/lib

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
	rename.c	\
	ui.c

LDADD+= -lkadm5clnt -lsl -ledit -lterminfo -lutil
DPADD+= ${LIBKADM5CLNT} ${LIBSL} ${LIBEDIT} ${LIBTERMINFO} ${LIBUTIL}

ktutil.8: ktutil.1
	@${TOOL_SED} -e 's/KTUTIL 1/KTUTIL 8/' < ${.ALLSRC} > ${.TARGET}

CLEANFILES+=ktutil.8

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
