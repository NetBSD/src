#	$NetBSD: Makefile,v 1.5 2001/12/12 01:48:56 tv Exp $

LIB=	sup

SRCS=	scm.c scmio.c stree.c log.c supmsg.c netcrypt.c
SRCS+=	atoo.c errmsg.c expand.c ffilecopy.c filecopy.c \
	nxtarg.c path.c quit.c run.c salloc.c skipto.c \
	vprintf.c

.PATH: ${.CURDIR}/../source

CLEANFILES+=netcrypt.c

NOLINT=	# defined
NOPIC=	# defined
NOPROFILE=# defined

netcrypt.c: netcryptvoid.c
	rm -f netcrypt.c
	ln -s ${.CURDIR}/../source/netcryptvoid.c netcrypt.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
