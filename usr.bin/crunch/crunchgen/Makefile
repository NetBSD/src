#	$NetBSD: Makefile,v 1.3 1997/08/02 21:30:09 perry Exp $

PROG=crunchgen
SRCS=crunchgen.c crunched_skel.c
#CFLAGS+=-g
CLEANFILES+= crunched_skel.c

crunched_skel.c: crunched_main.c
	sh ${.CURDIR}/mkskel.sh ${.CURDIR}/crunched_main.c > crunched_skel.c

.include <bsd.prog.mk>
