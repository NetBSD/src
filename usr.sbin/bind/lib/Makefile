#	$NetBSD: Makefile,v 1.2.2.1 1999/12/04 17:11:01 he Exp $

LIB=bind

MKPIC=no
MKPROFILE=no
MKLINT=no

CPPFLAGS+=	${INCLUDE} -I${BIND_DIST_DIR}/lib/dst \
		-DHMAC_MD5 -DUSE_MD5

.include "${.CURDIR}/../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/lib/resolv \
	${BIND_DIST_DIR}/lib/isc \
	${BIND_DIST_DIR}/lib/inet \
	${BIND_DIST_DIR}/lib/nameser \
	${BIND_DIST_DIR}/lib/dst

# ${BIND_DIST_DIR}/lib/resolv
SRCS+=	res_debug.c res_data.c res_comp.c res_init.c \
	res_mkquery.c res_query.c res_send.c res_sendsigned.c \
	res_mkupdate.c res_update.c res_findzonecut.c
# ${BIND_DIST_DIR}/lib/isc
SRCS+=	tree.c base64.c bitncmp.c assertions.c memcluster.c \
	logging.c heap.c ctl_p.c ctl_srvr.c ctl_clnt.c \
	eventlib.c ev_connects.c ev_files.c ev_timers.c \
	ev_streams.c ev_waits.c
# ${BIND_DIST_DIR}/lib/inet
SRCS+=	nsap_addr.c inet_addr.c inet_ntop.c inet_pton.c \
	inet_ntoa.c inet_neta.c inet_net_ntop.c inet_net_pton.c \
	inet_cidr_ntop.c inet_cidr_pton.c \
	inet_lnaof.c inet_makeaddr.c inet_netof.c inet_network.c
# ${BIND_DIST_DIR}/lib/nameser
SRCS+=	ns_parse.c ns_print.c ns_netint.c ns_ttl.c ns_name.c \
	ns_sign.c ns_verify.c ns_date.c ns_samedomain.c
# ${BIND_DIST_DIR}/lib/dst
SRCS+=	dst_api.c prandom.c rsaref_link.c support.c bsafe_link.c \
	cylink_link.c hmac_link.c eay_dss_link.c

# only needed during build
libinstall::

port_ipv6.h:
	env CC="${CC}" /bin/sh ${BIND_DIST_DIR}/bin/probe_ipv6

CLEANFILES+=port_ipv6.h

# mention explicit dependency, so port_ipv6.h will appear timely
res_debug.c: port_ipv6.h

.include <bsd.lib.mk>

${OBJS}: port_ipv6.h
