#	$NetBSD: Makefile,v 1.2 1997/10/16 08:26:50 mikel Exp $

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c salloc.c skipto.c \
	vprintf.c

.PATH: ${.CURDIR}/../source

CLEANFILES+=netcrypt.c

NOPROFILE=
NOPIC=

netcrypt.c: netcryptvoid.c
	rm -f netcrypt.c
	ln -s ${.CURDIR}/../source/netcryptvoid.c netcrypt.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
