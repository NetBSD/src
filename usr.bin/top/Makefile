#	$NetBSD: Makefile,v 1.3 1999/03/24 11:57:08 christos Exp $

PROG=	top

.PATH:	${.CURDIR}/machine

CPPFLAGS+=-I${.CURDIR} -I. -DORDER -DHAVE_STRERROR
SRCS=	top.c commands.c display.c screen.c username.c utils.c version.c \
	m_netbsd13.c
DPSRCS+=sigdesc.h

LDADD+=	-ltermcap -lm -lkvm
DPADD+=	${LIBTERMCAP} ${LIBM} ${LIBKVM}
BINGRP=	kmem
BINMODE=2555

sigdesc.h: ${.CURDIR}/sigconv.awk ${DESTDIR}/usr/include/sys/signal.h
	awk -f ${.CURDIR}/sigconv.awk ${DESTDIR}/usr/include/sys/signal.h > \
	    ${.TARGET}

.include <bsd.prog.mk>
