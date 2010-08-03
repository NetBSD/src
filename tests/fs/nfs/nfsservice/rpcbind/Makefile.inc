#	$NetBSD: Makefile.inc,v 1.2 2010/08/03 17:24:45 drochner Exp $

SRCS.rumpnfsd+=	check_bound.c rpcb_stat.c rpcb_svc_4.c rpcbind.c pmap_svc.c \
		rpcb_svc.c rpcb_svc_com.c security.c util.c 

LIBRPCDIR=        /usr/src/lib/libc/rpc

CPPFLAGS+=	-I${LIBRPCDIR} -DPORTMAP -DLIBWRAP

# Uncomment these to get any useful output from 'rpcbind -d'
# CPPFLAGS+=	-DRPCBIND_DEBUG
# CPPFLAGS+=	-DSVC_RUN_DEBUG

LDADD+= -lwrap -lutil
DPADD+= ${LIBWRAP} ${LIBUTIL}
