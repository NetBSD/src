#	$NetBSD: Makefile,v 1.4 1997/10/24 09:00:26 lukem Exp $

PROG=crunchgen
SRCS=crunchgen.c crunched_skel.c
CLEANFILES+= crunched_skel.c

crunched_skel.c: crunched_main.c
	sh ${.CURDIR}/mkskel.sh ${.CURDIR}/crunched_main.c > crunched_skel.c

.include <bsd.prog.mk>
