#	$NetBSD: Makefile,v 1.3 1999/02/13 02:55:01 lukem Exp $

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c salloc.c skipto.c \
	vprintf.c

.PATH: ${.CURDIR}/../source

CLEANFILES+=netcrypt.c

MKPROFILE=no
MKPIC=	no

netcrypt.c: netcryptvoid.c
	rm -f netcrypt.c
	ln -s ${.CURDIR}/../source/netcryptvoid.c netcrypt.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
