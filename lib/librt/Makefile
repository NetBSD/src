#	$NetBSD: Makefile,v 1.1 2003/01/24 01:52:43 thorpej Exp $
#

WARNS=	2

LIB=	rt
SRCS=	sem.c

MAN+=	sem_destroy.3 sem_getvalue.3 sem_init.3 sem_open.3 sem_post.3 \
	sem_wait.3

MLINKS+=	sem_open.3 sem_close.3
MLINKS+=	sem_open.3 sem_unlink.3
MLINKS+=	sem_wait.3 sem_trywait.3

.include <bsd.lib.mk>
