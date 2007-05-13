#	$NetBSD: Makefile,v 1.7 2007/05/13 20:03:46 christos Exp $

.include <bsd.own.mk>

WARNS=4
PROG=	rpcbind
MAN=	rpcbind.8
SRCS=	check_bound.c rpcb_stat.c rpcb_svc_4.c rpcbind.c pmap_svc.c \
	rpcb_svc.c rpcb_svc_com.c security.c warmstart.c util.c \
	rpc_generic.c

LIBCDIR=        ${NETBSDSRCDIR}/lib/libc
LIBCRPCDIR=     ${LIBCDIR}/rpc
LIBCINCLUDE=    ${LIBCDIR}/include

CPPFLAGS+=      -I${LIBCRPCDIR} -I${LIBCINCLUDE} -DPORTMAP -DLIBWRAP

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

LDADD+= -lwrap -lutil
DPADD+= ${LIBWRAP} ${LIBUTIL}

.PATH:  ${LIBCRPCDIR}

.include <bsd.prog.mk>
