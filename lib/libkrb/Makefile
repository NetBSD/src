# $NetBSD: Makefile,v 1.11 2001/09/21 22:59:01 tv Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/krb4/lib/krb
.PATH: ${DIST}/krb4/doc
.PATH: ${DIST}/openssl/crypto/des
.PATH: ${DIST}/krb4/man

MAN=	kerberos.3 krb_realmofhost.3 krb_sendauth.3 krb_set_tkt_string.3 \
	krb.conf.5 krb.equiv.5 krb.extra.5 krb.realms.5
MLINKS=	kerberos.3 krb_mk_req.3 kerberos.3 krb_rd_req.3 \
	kerberos.3 krb_kntoln.3 kerberos.3 krb_set_key.3 \
	kerberos.3 krb_get_cred.3 kerberos.3 krb_mk_priv.3 \
	kerberos.3 krb_rd_priv.3 kerberos.3 krb_mk_safe.3 \
	kerberos.3 krb_rd_safe.3 kerberos.3 krb_mk_err.3 \
	kerberos.3 krb_rd_err.3 kerberos.3 krb_ck_repl.3 \
	krb_realmofhost.3 krb_get_krbhst.3 \
	krb_realmofhost.3 krb_get_lrealm.3 \
	krb_realmofhost.3 krb_get_phost.3

WARNS?=  1
MKLINT= no

LIB=	krb

.ifndef USE_NEW_TOOLCHAIN
COMPILEETOBJ!=	cd ${.CURDIR}/../libcom_err/compile_et && ${PRINTOBJDIR}
COMPILE_ET=	${COMPILEETOBJ}/compile_et
.endif

krb_err.c krb_err.h: krb_err.et
	${COMPILE_ET} ${DIST}/krb4/lib/krb/krb_err.et

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
	random_block.c \
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
	prot.h					\
	des.h

CPPFLAGS+= -I.					\
	-I${DIST}/krb4/lib/krb			\
	-I${DIST}/openssl/crypto		\
	-I${DIST}/heimdal/lib/com_err		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${.CURDIR}/../libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

CLEANFILES = $(ERR_FILES) krb_err.h ktypes.h krb.h

krb.h:	krb.h.in
	sed 's,@DES_H@,openssl/des.h,' < ${.ALLSRC} > ${.TARGET}

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
