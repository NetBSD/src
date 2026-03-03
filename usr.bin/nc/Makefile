#	$OpenBSD: Makefile,v 1.7 2015/09/11 21:07:01 beck Exp $

.include <bsd.own.mk>

PROG=	nc
SRCS=	netcat.c atomicio.c socks.c

# If this is ever made to work, change ".nr C 0" to ".nr C 1"
# early in nc.1 to return all the cryto options to the man page.
#CPPFLAGS+=-DCRYPTO
#LDADD+= -ltls -lssl -lcrypto
#DPADD+=  ${LIBTLS} ${LIBSSL} ${LIBCRYPTO}

.include <bsd.prog.mk>
