#	$NetBSD: Makefile,v 1.2 2000/06/20 13:55:11 fvdl Exp $

PROG=	rpcbind
MAN=	rpcbind.8
SRCS=	check_bound.c rpcb_stat.c rpcb_svc_4.c rpcbind.c pmap_svc.c \
	rpcb_svc.c rpcb_svc_com.c security.c warmstart.c util.c \
	rpc_generic.c svc_vc.c

LIBCDIR=        ${.CURDIR}/../../lib/libc
LIBCRPCDIR=     ${LIBCDIR}/rpc
LIBCINCLUDE=    ${LIBCDIR}/include

CPPFLAGS+=      -I${LIBCRPCDIR} -I${LIBCINCLUDE} -DPORTMAP -DLIBWRAP -DINET6 -DRPCBIND_DEBUG

LDADD+= -lwrap -lutil
DPADD+= ${LIBWRAP} ${LIBUTIL}

CFLAGS+=	-g
LDFLAGS+=	-g

.PATH:  ${LIBCRPCDIR}

.include <bsd.prog.mk>
