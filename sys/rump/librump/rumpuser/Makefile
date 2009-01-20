#	$NetBSD: Makefile,v 1.14 2009/01/20 21:43:13 pooka Exp $
#

LIB=		rumpuser
WARNS=		4

SRCS=		rumpuser.c rumpuser_net.c
SRCS+=		rumpuser_pth.c

# Using this instead of the above makes it possible to link rump
# without involving pthreads.  Multithreaded operation will not
# of course work correctly in case.
#SRCS+=		rumpuser_pth_dummy.c

# no shlib_version because this is automatically in sync with lib/librumpuser
SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.lib.mk>
