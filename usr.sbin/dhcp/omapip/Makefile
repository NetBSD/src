# $NetBSD: Makefile,v 1.7 2002/09/18 13:31:54 lukem Exp $

NOLINT=	# defined
NOPIC=	# defined
NOPROFILE=# defined

LIB    = omapi
SRCS   = protocol.c buffer.c alloc.c result.c connection.c errwarn.c \
         listener.c dispatch.c generic.c support.c handle.c message.c \
	 auth.c convert.c hash.c array.c inet_addr.c mrtrace.c trace.c \
	 toisc.c

DHCPSRCDIR=	omapip

# only needed during build
libinstall::

.include <bsd.lib.mk>
