# $NetBSD: Makefile,v 1.6.2.2 2000/06/23 16:16:43 minoura Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/krb4/lib/krb
.PATH: ${DIST}/krb4/doc

WARNS?=  1
MKLINT= no

LIB=	krb

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && make print-objdir

krb_err.c krb_err.h: krb_err.et
	${COMPILEET}/compile_et ${DIST}/krb4/lib/krb/krb_err.et

ERR_FILES=	krb_err.c

DPSRCS=		krb_err.h

SRCS=	\
	check_time.c \
	cr_err_reply.c \
	create_auth_reply.c \
	create_ciph.c \
	create_death_packet.c \
	create_ticket.c \
	debug_decl.c \
	decomp_ticket.c \
	defaults.c \
	dest_tkt.c \
	encrypt_ktext.c \
	extra.c \
	get_ad_tkt.c \
	getfile.c \
	get_cred.c \
	get_default_principal.c \
	get_host.c \
	get_in_tkt.c \
	get_krbrlm.c \
	get_svc_in_tkt.c \
	get_tf_fullname.c \
	get_tf_realm.c \
	getaddrs.c \
	getrealm.c \
	getst.c \
	k_getport.c \
	k_getsockinst.c \
	k_localtime.c \
	kdc_reply.c \
	kntoln.c \
	krb_check_auth.c \
	krb_equiv.c \
	krb_err.c \
	krb_err_txt.c \
	krb_get_in_tkt.c \
	kuserok.c \
	lifetime.c \
	logging.c \
	lsb_addr_comp.c \
	mk_auth.c \
	mk_err.c \
	mk_priv.c \
	mk_req.c \
	mk_safe.c \
	month_sname.c \
	name2name.c \
	krb_net_read.c \
	krb_net_write.c \
	one.c \
	parse_name.c \
	rd_err.c \
	rd_priv.c \
	rd_req.c \
	rd_safe.c \
	read_service_key.c \
	realm_parse.c \
	recvauth.c \
	rw.c \
	save_credentials.c \
	send_to_kdc.c \
	sendauth.c \
	solaris_compat.c \
	stime.c \
	str2key.c \
	tf_util.c \
	time.c \
	tkt_string.c \
	unparse_name.c \
	verify_user.c \
	krb_ip_realm.c

INCSDIR= /usr/include/kerberosIV

INCS=	krb.h					\
	krb_err.h				\
	ktypes.h				\
	klog.h					\
	krb-protos.h				\
	krb-archaeology.h			\
	prot.h

LINKS+=	/usr/include/des.h ${INCSDIR}/des.h

CPPFLAGS+= -I.					\
	-I${DIST}/krb4/lib/krb			\
	-I${DIST}/openssl/crypto/des		\
	-I${DIST}/heimdal/lib/com_err		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${.CURDIR}/../libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

CLEANFILES = $(ERR_FILES) krb_err.h ktypes.h

ktypes.h:
	cp ${.CURDIR}/../../include/heimdal/$@ $@

$(SRCS:.c=.o): krb_err.h

TEXINFO=	kth-krb.texi
INFOFLAGS=	-I${DIST}/krb4/doc

# Prevent collisions with old MIT Kerberos includes -- require manual
# intervention of the operator.
.BEGIN:
.ifmake includes
	@if [ -f ${DESTDIR}${INCSDIR}/mit-copyright.h ]; then \
		echo "Error: you must first remove the MIT headers from ${DESTDIR}${INCSDIR}" >&2; \
		false; \
	fi
.endif

.include <bsd.lib.mk>
.include <bsd.info.mk>
