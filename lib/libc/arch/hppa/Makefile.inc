#	$NetBSD: Makefile.inc,v 1.8.4.2 2008/10/26 07:43:08 mrg Exp $

#	$OpenBSD: Makefile.inc,v 1.3 1999/12/25 06:02:10 mickey Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

#SRCS+=	memmove.S
#CLEANFILES+=	memmove.S
SRCS+= bcopy.c

#memmove.S:	${LIBCDIR}/arch/hppa/string/bcopy.m4
#	@echo 'building ${.TARGET} from ${.ALLSRC}'
#	@m4 -DNAME=bcopy ${.ALLSRC} > ${.TARGET}
