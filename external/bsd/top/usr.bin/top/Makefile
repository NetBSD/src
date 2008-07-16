#	$NetBSD: Makefile,v 1.4 2008/07/16 14:29:11 christos Exp $

.include <bsd.own.mk>
PROG=	top

TOPDIR=${.CURDIR}/../../dist
.PATH:	${TOPDIR} ${TOPDIR}/machine

CPPFLAGS+=-I${.CURDIR} -I${TOPDIR} -I.
SRCS=	color.c commands.c display.c hash.c screen.c \
	top.c username.c utils.c version.c m_netbsd.c
DPSRCS+=sigdesc.h config.h

LDADD+=	-ltermcap -lm -lkvm
DPADD+=	${LIBTERMCAP} ${LIBM} ${LIBKVM}

sigdesc.h: ${TOPDIR}/sigconv.awk ${DESTDIR}/usr/include/sys/signal.h
	${_MKTARGET_CREATE}
	awk -f ${TOPDIR}/sigconv.awk ${DESTDIR}/usr/include/sys/signal.h > \
	    ${.TARGET}
CLEANFILES+= sigdesc.h top.1 config.h

top.1: top.1.in
	${TOOL_SED} -e s/@DEFAULT_TOPN@/-1/ \
		-e s/@DEFAULT_DELAY@/5/ \
		-e s/@HAVE_GETOPT_LONG@/1/ \
		-e s/@ENABLE_KILL@/1/ \
		-e s/@MAN_SUPPLEMENT@// < $? > $@

commands.c: sigdesc.h

config.h: config.h.in
	${TOOL_SED} -e s/@MACHINE@/${MACHINE}/ < $? > $@

.include <bsd.prog.mk>
