# $NetBSD: Makefile,v 1.1 2007/11/16 18:06:47 jmmv Exp $

.include <bsd.own.mk>

# XXX This installs too much "garbage" in the destination directory, such as
# the libfake_pic.a and libfake.so files.  We only need a shared object that
# we can later LD_PRELOAD, but I don't know how to do this from our current
# bsd.lib.mk (or, for that matter, any other .mk file).

LIB=	fake
SRCS=	pwgr.c

MKLINKLIB=	no
MKSTATICLIB=	no
MKPIC=		yes
MKPROFILE=	no
SHLIB_MAJOR=	0
SHLIB_MINOR=	0
LIBDIR=		/usr/tests/util/id

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
