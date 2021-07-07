#	$NetBSD: Makefile,v 1.15 2021/07/07 11:51:46 martin Exp $
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

.if ${RUMP_SANITIZE:Uno} != "no"
LDADD+=	-fsanitize=${RUMP_SANITIZE}
.endif

.include <bsd.prog.mk>
