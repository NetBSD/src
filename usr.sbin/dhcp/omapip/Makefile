# $NetBSD: Makefile,v 1.6 2001/12/12 01:48:50 tv Exp $

LIB    = omapi
SRCS   = protocol.c buffer.c alloc.c result.c connection.c errwarn.c \
         listener.c dispatch.c generic.c support.c handle.c message.c \
	 auth.c convert.c hash.c array.c inet_addr.c mrtrace.c trace.c \
	 toisc.c

DHCPSRCDIR=	omapip

NOLINT=	# defined
NOPIC=	# defined
NOPROFILE=# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
