#	$NetBSD: Makefile.inc,v 1.2.12.1 2013/06/23 06:28:55 tls Exp $

SRCS.rumpnfsd+=	check_bound.c rpcb_stat.c rpcb_svc_4.c rpcbind.c pmap_svc.c \
		rpcb_svc.c rpcb_svc_com.c security.c util.c 

LIBRPCDIR=        ${NETBSDSRCDIR}/lib/libc/rpc

CPPFLAGS+=	-I${LIBRPCDIR} -DPORTMAP -DLIBWRAP

# Uncomment these to get any useful output from 'rpcbind -d'
# CPPFLAGS+=	-DRPCBIND_DEBUG
# CPPFLAGS+=	-DSVC_RUN_DEBUG

LDADD+= -lwrap -lutil
DPADD+= ${LIBWRAP} ${LIBUTIL}
