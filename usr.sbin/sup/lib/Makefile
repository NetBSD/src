#	$NetBSD: Makefile,v 1.10 2004/05/23 02:24:52 lukem Exp $

MKPRIVATELIB=	yes

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c salloc.c skipto.c

.PATH: ${.CURDIR}/../source

BUILDSYMLINKS=	${.CURDIR}/../source/netcryptvoid.c netcrypt.c

.include <bsd.lib.mk>
