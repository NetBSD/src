#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 10:36:51 lukem Exp $

PROG=crunchgen
SRCS=crunchgen.c crunched_skel.c
CLEANFILES+= crunched_skel.c

crunched_skel.c: crunched_main.c
	sh ${.CURDIR}/mkskel.sh ${.CURDIR}/crunched_main.c > crunched_skel.c

.include <bsd.prog.mk>
