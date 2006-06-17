#	$NetBSD: Makefile.inc,v 1.7 2006/06/17 18:04:23 uwe Exp $

#	$OpenBSD: Makefile.inc,v 1.3 1999/12/25 06:02:10 mickey Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

#SRCS+=	memmove.S
#CLEANFILES+=	memmove.S
SRCS+= bcopy.c

#memmove.S:	${.CURDIR}/arch/hppa/string/bcopy.m4
#	@echo 'building ${.TARGET} from ${.ALLSRC}'
#	@m4 -DNAME=bcopy ${.ALLSRC} > ${.TARGET}
