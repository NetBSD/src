#	$NetBSD: Makefile,v 1.4 1999/03/13 23:22:56 lukem Exp $

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c salloc.c skipto.c \
	vprintf.c

.PATH: ${.CURDIR}/../source

CLEANFILES+=netcrypt.c

MKLINT=	no
MKPIC=	no
MKPROFILE=no

netcrypt.c: netcryptvoid.c
	rm -f netcrypt.c
	ln -s ${.CURDIR}/../source/netcryptvoid.c netcrypt.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
