#	$NetBSD: Makefile,v 1.7 2003/10/19 04:30:30 lukem Exp $

PROG=	top

.PATH:	${.CURDIR}/machine

CPPFLAGS+=-I${.CURDIR} -I. -DORDER -DHAVE_STRERROR
SRCS=	top.c commands.c display.c screen.c username.c utils.c version.c \
	m_netbsd15.c
DPSRCS+=sigdesc.h

LDADD+=	-ltermcap -lm -lkvm
DPADD+=	${LIBTERMCAP} ${LIBM} ${LIBKVM}

sigdesc.h: ${.CURDIR}/sigconv.awk ${DESTDIR}/usr/include/sys/signal.h
	${_MKMSGCREATE}
	${_MKCMD}\
	awk -f ${.CURDIR}/sigconv.awk ${DESTDIR}/usr/include/sys/signal.h > \
	    ${.TARGET}
CLEANFILES+= sigdesc.h

commands.c: sigdesc.h

.include <bsd.prog.mk>
