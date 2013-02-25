#	$NetBSD: Makefile,v 1.13.60.1 2013/02/25 00:27:43 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/12/93

PROG=	adventure
SRCS=	main.c init.c done.c save.c subr.c vocab.c wizard.c io.c data.c crc.c
MAN=	adventure.6
HIDEGAME=hidegame
CLEANFILES+=mkdata setup.lo data.c

data.c: glorkz mkdata
	${_MKTARGET_CREATE}
	./mkdata ${.CURDIR}/glorkz > data.c

setup.lo: hdr.h
mkdata: setup.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC}

.include <bsd.prog.mk>
