#	$NetBSD: Makefile,v 1.14 2020/05/16 12:46:57 christos Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= \
    -Wl,--whole-archive \
	-lrumpkern_sysproxy -lrump -lrumpvfs_nofifofs -lrumpvfs -lrumpuser \
    -Wl,--no-whole-archive
LDADD+= -lpthread

.include <bsd.prog.mk>
