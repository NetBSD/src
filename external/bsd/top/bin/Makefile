#	$NetBSD: Makefile,v 1.5 2010/02/03 15:34:38 roy Exp $

.include <bsd.own.mk>

PROG=	top
BINDIR=	/usr/bin

TOPDIR=${.CURDIR}/../dist
.PATH:	${TOPDIR} ${TOPDIR}/machine

WARNS=	4

CPPFLAGS+=-I${.CURDIR} -I${TOPDIR} -I.
SRCS=	color.c commands.c display.c hash.c screen.c \
	top.c username.c utils.c version.c m_netbsd.c
DPSRCS+=sigdesc.h config.h

LDADD+=	-lterminfo -lm -lkvm -lutil
DPADD+=	${LIBTERMINFO} ${LIBM} ${LIBKVM} ${LIBUTIL}

sigdesc.h: ${TOPDIR}/sigconv.awk ${DESTDIR}/usr/include/sys/signal.h
	${_MKTARGET_CREATE}
	${TOOL_AWK} -f ${TOPDIR}/sigconv.awk \
	    ${DESTDIR}/usr/include/sys/signal.h > ${.TARGET}
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
