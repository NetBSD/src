#	$NetBSD: Makefile,v 1.12.16.1 2008/05/18 12:36:23 yamt Exp $

LIBISPRIVATE=	yes

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c skipto.c

.PATH: ${.CURDIR}/../source

BUILDSYMLINKS=	${.CURDIR}/../source/netcryptvoid.c netcrypt.c

.include <bsd.lib.mk>
