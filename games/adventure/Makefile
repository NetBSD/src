#	$NetBSD: Makefile,v 1.12 2003/10/21 10:01:19 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	adventure
SRCS=	main.c init.c done.c save.c subr.c vocab.c wizard.c io.c data.c crc.c
MAN=	adventure.6
HIDEGAME=hidegame
CLEANFILES+=setup data.c

data.c: glorkz setup
	${_MKTARGET_CREATE}
	./setup ${.CURDIR}/glorkz > data.c

setup: setup.c hdr.h
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o setup ${.CURDIR}/setup.c

.include <bsd.prog.mk>
