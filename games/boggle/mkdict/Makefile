#	$NetBSD: Makefile,v 1.3 1997/04/19 07:08:39 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

HOST_CFLAGS+=-I${.CURDIR}/../boggle

mkdict: ${.CURDIR}/mkdict.c ${.CURDIR}/../boggle/bog.h
	${HOST_COMPILE.c} ${.CURDIR}/mkdict.c
	${HOST_LINK.c} -o ${.TARGET} mkdict.o

.include <bsd.sys.mk>
