#	$NetBSD: Makefile,v 1.8 2000/04/24 15:15:05 pk Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	adventure
SRCS=	main.c init.c done.c save.c subr.c vocab.c wizard.c io.c data.c crc.c
MAN=	adventure.6
HIDEGAME=hidegame
CLEANFILES+=setup data.c

data.c: glorkz setup
	${.OBJDIR}/setup ${.CURDIR}/glorkz > data.c

setup: setup.c hdr.h
	${HOST_LINK.c} -o setup ${.CURDIR}/setup.c

.include <bsd.prog.mk>
