# $NetBSD: Makefile,v 1.6 2000/08/09 02:16:20 thorpej Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/krb5
.PATH: ${DIST}/heimdal/doc

WARNS?=  1
MKLINT= no

LIB=	krb5

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && make print-objdir

krb5_err.c krb5_err.h: krb5_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/krb5/krb5_err.et

heim_err.c heim_err.h: heim_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/krb5/heim_err.et

ERR_FILES= krb5_err.c heim_err.c

DPSRCS=	krb5_err.h heim_err.h

SRCS=						\
	acl.c					\
	add_et_list.c				\
	addr_families.c				\
	address.c				\
	aname_to_localname.c			\
	appdefault.c				\
	asn1_glue.c				\
	auth_context.c				\
	build_ap_req.c				\
	build_auth.c				\
	cache.c					\
	changepw.c				\
	codec.c					\
	config_file.c				\
	config_file_netinfo.c			\
	convert_creds.c				\
	constants.c				\
	context.c				\
	copy_host_realm.c			\
	crc.c					\
	creds.c					\
	crypto.c				\
	data.c					\
	eai_to_heim_errno.c			\
	expand_hostname.c			\
	fcache.c				\
	free.c					\
	free_host_realm.c			\
	generate_seq_number.c			\
	generate_subkey.c			\
	get_addrs.c				\
	get_cred.c				\
	get_default_principal.c			\
	get_default_realm.c			\
	get_for_creds.c				\
	get_host_realm.c			\
	get_in_tkt.c				\
	get_in_tkt_pw.c				\
	get_in_tkt_with_keytab.c		\
	get_in_tkt_with_skey.c			\
	get_port.c				\
	init_creds.c				\
	init_creds_pw.c				\
	keyblock.c				\
	keytab.c				\
	keytab_file.c				\
	keytab_memory.c				\
	keytab_keyfile.c			\
	keytab_krb4.c				\
	krbhst.c				\
	kuserok.c				\
	log.c					\
	mcache.c				\
	misc.c					\
	mk_error.c				\
	mk_priv.c				\
	mk_rep.c				\
	mk_req.c				\
	mk_req_ext.c				\
	mk_safe.c				\
	net_read.c				\
	net_write.c				\
	n-fold.c				\
	padata.c				\
	principal.c				\
	prog_setup.c				\
	prompter_posix.c			\
	rd_cred.c				\
	rd_error.c				\
	rd_priv.c				\
	rd_rep.c				\
	rd_req.c				\
	rd_safe.c				\
	read_message.c				\
	recvauth.c				\
	send_to_kdc.c				\
	sendauth.c				\
	set_default_realm.c			\
	sock_principal.c			\
	store.c					\
	store_emem.c				\
	store_fd.c				\
	store_mem.c				\
	ticket.c				\
	time.c					\
	transited.c				\
	verify_init.c				\
	verify_user.c				\
	version.c				\
	warn.c					\
	write_message.c				\
	$(ERR_FILES)

INCS=	krb5.h					\
	krb5-protos.h				\
	krb5-private.h				\
	krb5_err.h				\
	heim_err.h				\
	krb5-types.h

MAN=						\
	krb5.conf.5				\
	krb5_425_conv_principal.3		\
	krb5_appdefault.3			\
	krb5_build_principal.3			\
	krb5_config.3				\
	krb5_free_principal.3			\
	krb5_openlog.3				\
	krb5_parse_name.3			\
	krb5_sname_to_principal.3		\
	krb5_unparse_name.3			\
	krb5_warn.3				\
	verify_krb5_conf.8

krb5-types.h:
	cp ${.CURDIR}/../../include/heimdal/$@ $@

INCSDIR= /usr/include/krb5

ASN1!=	cd ${.CURDIR}/../libasn1 && ${MAKE} print-objdir

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/krb5		\
	 -I${DIST}/krb4/lib/krb			\
	 -I${DIST}/openssl/crypto/md4		\
	 -I${DIST}/openssl/crypto/md5		\
	 -I${DIST}/openssl/crypto/sha		\
	 -I${DIST}/openssl/crypto/rc4		\
	 -I${DIST}/openssl/crypto/des		\
	 -I${ASN1}				\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

CLEANFILES = $(ERR_FILES) krb5_err.h heim_err.h krb5-types.h

$(SRCS:.c=.o): krb5_err.h heim_err.h

TEXINFO=	heimdal.texi
INFOFLAGS=	-I${DIST}/heimdal/doc

# Prevent collision with old MIT Kerberos includes -- require manual
# intervention of the operator.
.BEGIN:
.ifmake includes
	@if [ -f ${DESTDIR}${INCSDIR}/osconf.h ]; then \
		echo "Error: you must first remove the MIT headers from ${DESTDIR}${INCSDIR}" >&2; \
		false; \
	fi
.endif

.include <bsd.lib.mk>
.include <bsd.info.mk>
