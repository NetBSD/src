#	$NetBSD: Makefile,v 1.13 2003/11/16 14:14:18 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	adventure
SRCS=	main.c init.c done.c save.c subr.c vocab.c wizard.c io.c data.c crc.c
MAN=	adventure.6
HIDEGAME=hidegame
CLEANFILES+=setup setup.lo data.c

data.c: glorkz setup
	${_MKTARGET_CREATE}
	./setup ${.CURDIR}/glorkz > data.c

setup.lo: hdr.h
setup: setup.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC}

.include <bsd.prog.mk>
