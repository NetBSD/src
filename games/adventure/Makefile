#	$NetBSD: Makefile,v 1.7 1998/08/01 23:02:35 hubertf Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	adventure
SRCS=	main.c init.c done.c save.c subr.c vocab.c wizard.c io.c data.c crc.c
MAN=	adventure.6
HIDEGAME=hidegame
CLEANFILES+=setup data.c

data.c: glorkz setup
	${.OBJDIR}/setup ${.CURDIR}/glorkz > data.c

setup: setup.c hdr.h
	${HOST_CC} -o setup ${.CURDIR}/setup.c

.include <bsd.prog.mk>
