# $NetBSD: Makefile,v 1.5 2001/08/03 14:14:26 drochner Exp $

LIB    = omapi
SRCS   = protocol.c buffer.c alloc.c result.c connection.c errwarn.c \
         listener.c dispatch.c generic.c support.c handle.c message.c \
	 auth.c convert.c hash.c array.c inet_addr.c mrtrace.c trace.c \
	 toisc.c

DHCPSRCDIR=	omapip

MKLINT=	no
MKPIC=	no
MKPROFILE=no

# only needed during build
libinstall::

.include <bsd.lib.mk>
