#	$NetBSD: Makefile,v 1.9 2003/10/14 11:49:52 tron Exp $

NOLINT=	# defined
NOPIC=	# defined
NOPROFILE=# defined

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c salloc.c skipto.c

.PATH: ${.CURDIR}/../source

BUILDSYMLINKS=	${.CURDIR}/../source/netcryptvoid.c netcrypt.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
