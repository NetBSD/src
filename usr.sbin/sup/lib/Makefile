#	$NetBSD: Makefile,v 1.15 2019/10/13 07:28:21 mrg Exp $

LIBISPRIVATE=	yes

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c skipto.c

.PATH: ${.CURDIR}/../source

BUILDSYMLINKS=	${.CURDIR}/../source/netcryptvoid.c netcrypt.c

COPTS.log.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.lib.mk>
