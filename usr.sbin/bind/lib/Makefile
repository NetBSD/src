#	$NetBSD: Makefile,v 1.8 2000/03/01 10:50:02 itojun Exp $

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
	${BIND_DIST_DIR}/lib/dst \
	${BIND_DIST_DIR}/lib/irs

# ${BIND_DIST_DIR}/lib/resolv
SRCS+=	herror.c res_debug.c res_data.c res_comp.c res_init.c \
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
# ${BIND_DIST_DIR}/lib/irs
SRCS+=	dns.c dns_gr.c dns_ho.c dns_nw.c dns_pr.c dns_pw.c \
	dns_sv.c gai_strerror.c gen.c gen_gr.c gen_ho.c \
	gen_ng.c gen_nw.c gen_pr.c gen_pw.c gen_sv.c \
	getaddrinfo.c getgrent.c getgrent_r.c gethostent.c \
	gethostent_r.c getnameinfo.c getnetent.c getnetent_r.c \
	getnetgrent.c getnetgrent_r.c getprotoent.c \
	getprotoent_r.c getpwent.c getpwent_r.c getservent.c \
	getservent_r.c hesiod.c irp.c irp_gr.c irp_ho.c \
	irp_ng.c irp_nw.c irp_pr.c irp_pw.c irp_sv.c \
	irpmarshall.c irs_data.c lcl.c lcl_gr.c lcl_ho.c \
	lcl_ng.c lcl_nw.c lcl_pr.c lcl_pw.c lcl_sv.c nis.c \
	nis_gr.c nis_ho.c nis_ng.c nis_nw.c nis_pr.c nis_pw.c \
	nis_sv.c nul_ng.c util.c

# only needed during build
libinstall::

port_ipv6.h:
	env CC="${CC}" CPPFLAGS="${CPPFLAGS}" /bin/sh \
	${BIND_DIST_DIR}/bin/probe_ipv6

CLEANFILES+=port_ipv6.h

# mention explicit dependency, so port_ipv6.h will appear timely
res_debug.c: port_ipv6.h

.include <bsd.lib.mk>

${OBJS}: port_ipv6.h
