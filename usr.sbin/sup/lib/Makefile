#	$NetBSD: Makefile,v 1.13 2008/05/03 14:48:32 lukem Exp $

LIBISPRIVATE=	yes

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c skipto.c

.PATH: ${.CURDIR}/../source

BUILDSYMLINKS=	${.CURDIR}/../source/netcryptvoid.c netcrypt.c

.include <bsd.lib.mk>
