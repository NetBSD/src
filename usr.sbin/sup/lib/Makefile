#	$NetBSD: Makefile,v 1.14 2009/01/16 07:09:09 junyoung Exp $

LIBISPRIVATE=	yes

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c skipto.c

.PATH: ${.CURDIR}/../source

BUILDSYMLINKS=	${.CURDIR}/../source/netcryptvoid.c netcrypt.c

.include <bsd.lib.mk>
