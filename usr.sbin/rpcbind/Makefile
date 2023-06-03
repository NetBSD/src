#	$NetBSD: Makefile,v 1.18 2023/06/03 09:09:23 lukem Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client

PROG=	rpcbind
MAN=	rpcbind.8
SRCS=	check_bound.c rpcb_stat.c rpcb_svc_4.c rpcbind.c pmap_svc.c \
	rpcb_svc.c rpcb_svc_com.c security.c warmstart.c util.c 

LIBCDIR=        ${NETBSDSRCDIR}/lib/libc
LIBCRPCDIR=     ${LIBCDIR}/rpc

CPPFLAGS+=	-I${LIBCRPCDIR} -DPORTMAP -DLIBWRAP -DWARMSTART

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.if (${USE_YP} != "no")
CPPFLAGS+=	-DYP
.endif

# Uncomment these to get any useful output from 'rpcbind -d'
# CPPFLAGS+=	-DRPCBIND_DEBUG
# CPPFLAGS+=	-DDEBUG_RMTCALL
# CPPFLAGS+=	-DSVC_RUN_DEBUG

LDADD+= -lwrap -lblocklist -lutil
DPADD+= ${LIBWRAP} ${LIBBLOCKLIST} ${LIBUTIL}

COPTS.rpcb_svc_4.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.pmap_svc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.rpcb_svc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.rpcb_svc_com.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.PATH:  ${LIBCRPCDIR}

.include <bsd.prog.mk>
