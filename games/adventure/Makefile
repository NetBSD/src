#	$NetBSD: Makefile,v 1.11 2003/10/19 04:45:01 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	adventure
SRCS=	main.c init.c done.c save.c subr.c vocab.c wizard.c io.c data.c crc.c
MAN=	adventure.6
HIDEGAME=hidegame
CLEANFILES+=setup data.c

data.c: glorkz setup
	${_MKMSGCREATE}
	${_MKCMD}\
	./setup ${.CURDIR}/glorkz > data.c

setup: setup.c hdr.h
	${_MKMSGLINK}
	${_MKCMD}\
	${HOST_LINK.c} -o setup ${.CURDIR}/setup.c

.include <bsd.prog.mk>
