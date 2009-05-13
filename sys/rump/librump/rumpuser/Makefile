#	$NetBSD: Makefile,v 1.14.2.1 2009/05/13 17:22:58 jym Exp $
#

LIB=		rumpuser
WARNS=		4

SRCS=		rumpuser.c rumpuser_net.c
SRCS+=		rumpuser_pth.c

CPPFLAGS+=	-D_REENTRANT

# Using this instead of the above makes it possible to link rump
# without involving pthreads.  Multithreaded operation will not
# of course work correctly in case.
#SRCS+=		rumpuser_pth_dummy.c

# no shlib_version because this is automatically in sync with lib/librumpuser
SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.lib.mk>
