#	$NetBSD: Makefile,v 1.8 2003/09/20 06:20:43 lukem Exp $

NOLINT=	# defined
NOPIC=	# defined
NOPROFILE=# defined

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c salloc.c skipto.c \
	vprintf.c

.PATH: ${.CURDIR}/../source

BUILDSYMLINKS=	${.CURDIR}/../source/netcryptvoid.c netcrypt.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
