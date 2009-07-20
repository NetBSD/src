# $NetBSD: Makefile,v 1.22 2009/07/20 17:28:51 christos Exp $

USE_FORT?= yes        # network protocol library

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/gssapi \
		${DIST}/heimdal/lib/gssapi/krb5 \
		${DIST}/heimdal/lib/gssapi/mech \
		${DIST}/heimdal/lib/gssapi/spnego \
		${DIST}/heimdal/lib/gssapi/ntlm

WARNS?=		1

LIB=		gssapi

LIBDPLIBS+=	krb5	${.CURDIR}/../libkrb5 \
		asn1	${.CURDIR}/../libasn1 \
		com_err	${.CURDIR}/../libcom_err \
		roken	${.CURDIR}/../libroken \
		crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto \
		heimntlm	${.CURDIR}/../libheimntlm

.if ${USETOOLS} != "yes"
COMPILEETOBJ!=		cd ${NETBSDSRCDIR}/lib/libcom_err/compile_et && ${PRINTOBJDIR}
TOOL_COMPILE_ET=	${COMPILEETOBJ}/compile_et

ASN1COMPILEOBJ!=	cd ${NETBSDSRCDIR}/lib/libasn1/asn1_compile && ${PRINTOBJDIR}
TOOL_ASN1_COMPILE=	${ASN1COMPILEOBJ}/asn1_compile
.endif  

gkrb5_err.c gkrb5_err.h: krb5/gkrb5_err.et ${TOOL_COMPILE_ET}
	${TOOL_COMPILE_ET} ${DIST}/heimdal/lib/gssapi/krb5/gkrb5_err.et

gssapi_files = asn1_GSSAPIContextToken.x
spnego_files = \
	asn1_ContextFlags.x		\
	asn1_MechType.x			\
	asn1_MechTypeList.x		\
	asn1_NegotiationToken.x		\
	asn1_NegotiationTokenWin.x	\
	asn1_NegHints.x			\
	asn1_NegTokenInit.x		\
	asn1_NegTokenInitWin.x		\
	asn1_NegTokenResp.x

DPSRCS=         spnego_asn1.h gssapi_asn1.h gkrb5_err.h

BUILT_SOURCES= \
	${gssapi_files:.x=.c}		\
	${spnego_files:.x=.c}		\
	gkrb5_err.c

krb5src= \
	krb5__8003.c				\
	krb5__accept_sec_context.c		\
	krb5__acquire_cred.c			\
	krb5__add_cred.c				\
	krb5__address_to_krb5addr.c		\
	krb5__arcfour.c				\
	krb5__canonicalize_name.c		\
	krb5__ccache_name.c			\
	krb5__cfx.c				\
	krb5__compare_name.c			\
	krb5__compat.c				\
	krb5__context_time.c			\
	krb5__copy_ccache.c			\
	krb5__decapsulate.c			\
	krb5__delete_sec_context.c		\
	krb5__display_name.c			\
	krb5__display_status.c			\
	krb5__duplicate_name.c			\
	krb5__encapsulate.c			\
	krb5__export_name.c			\
	krb5__export_sec_context.c		\
	krb5__external.c				\
	krb5__get_mic.c				\
	krb5__import_name.c			\
	krb5__import_sec_context.c		\
	krb5__indicate_mechs.c			\
	krb5__init.c				\
	krb5__init_sec_context.c			\
	krb5__inquire_context.c			\
	krb5__inquire_cred.c			\
	krb5__inquire_cred_by_mech.c		\
	krb5__inquire_cred_by_oid.c		\
	krb5__inquire_mechs_for_name.c		\
	krb5__inquire_names_for_mech.c		\
	krb5__inquire_sec_context_by_oid.c	\
	krb5__process_context_token.c		\
	krb5__prf.c				\
	krb5__release_buffer.c			\
	krb5__release_cred.c			\
	krb5__release_name.c			\
	krb5__sequence.c				\
	krb5__set_cred_option.c			\
	krb5__set_sec_context_option.c		\
	krb5__ticket_flags.c			\
	krb5__unwrap.c				\
	krb5__v1.c				\
	krb5__verify_mic.c			\
	krb5__wrap.c

mechsrc= \
	mech__context.c \
	mech__gss_accept_sec_context.c \
	mech__gss_acquire_cred.c \
	mech__gss_add_cred.c \
	mech__gss_add_oid_set_member.c \
	mech__gss_buffer_set.c \
	mech__gss_canonicalize_name.c \
	mech__gss_compare_name.c \
	mech__gss_context_time.c \
	mech__gss_create_empty_oid_set.c \
	mech__gss_decapsulate_token.c \
	mech__gss_delete_sec_context.c \
	mech__gss_display_name.c \
	mech__gss_display_status.c \
	mech__gss_duplicate_name.c \
	mech__gss_duplicate_oid.c \
	mech__gss_encapsulate_token.c \
	mech__gss_export_name.c \
	mech__gss_export_sec_context.c \
	mech__gss_get_mic.c \
	mech__gss_import_name.c \
	mech__gss_import_sec_context.c \
	mech__gss_indicate_mechs.c \
	mech__gss_init_sec_context.c \
	mech__gss_inquire_context.c \
	mech__gss_inquire_cred.c \
	mech__gss_inquire_cred_by_mech.c \
	mech__gss_inquire_cred_by_oid.c \
	mech__gss_inquire_mechs_for_name.c \
	mech__gss_inquire_names_for_mech.c \
	mech__gss_krb5.c \
	mech__gss_mech_switch.c \
	mech__gss_names.c \
	mech__gss_oid_equal.c \
	mech__gss_oid_to_str.c \
	mech__gss_process_context_token.c \
	mech__gss_pseudo_random.c \
	mech__gss_release_buffer.c \
	mech__gss_release_cred.c \
	mech__gss_release_name.c \
	mech__gss_release_oid.c \
	mech__gss_release_oid_set.c \
	mech__gss_seal.c \
	mech__gss_set_cred_option.c \
	mech__gss_set_sec_context_option.c \
	mech__gss_sign.c \
	mech__gss_test_oid_set_member.c \
	mech__gss_unseal.c \
	mech__gss_unwrap.c \
	mech__gss_utils.c \
	mech__gss_verify.c \
	mech__gss_verify_mic.c \
	mech__gss_wrap.c \
	mech__gss_wrap_size_limit.c \
	mech__gss_inquire_sec_context_by_oid.c

spnegosrc= \
	spnego__accept_sec_context.c \
	spnego__compat.c \
	spnego__context_stubs.c \
	spnego__cred_stubs.c \
	spnego__external.c \
	spnego__init_sec_context.c

ntlmsrc= \
	ntlm__accept_sec_context.c \
	ntlm__acquire_cred.c \
	ntlm__add_cred.c \
	ntlm__canonicalize_name.c \
	ntlm__compare_name.c \
	ntlm__context_time.c \
	ntlm__crypto.c \
	ntlm__delete_sec_context.c \
	ntlm__display_name.c \
	ntlm__display_status.c \
	ntlm__duplicate_name.c \
	ntlm__export_name.c \
	ntlm__export_sec_context.c \
	ntlm__external.c \
	ntlm__import_name.c \
	ntlm__import_sec_context.c \
	ntlm__indicate_mechs.c \
	ntlm__init_sec_context.c \
	ntlm__inquire_context.c \
	ntlm__inquire_cred.c \
	ntlm__inquire_cred_by_mech.c \
	ntlm__inquire_mechs_for_name.c \
	ntlm__inquire_names_for_mech.c \
	ntlm__process_context_token.c \
	ntlm__release_cred.c \
	ntlm__release_name.c \
	ntlm__digest.c

SRCS= ${krb5src} ${mechsrc} ${spnegosrc} ${ntlmsrc} ${BUILT_SOURCES}

.for i in ${krb5src} ${mechsrc} ${spnegosrc} ${ntlmsrc}
BUILDSYMLINKS+= ${i:S@__@/@} ${i}
.endfor

MAN=				\
	gssapi.3		\
	gss_acquire_cred.3	\
	mech.5

MLINKS= \
	gss_acquire_cred.3		gss_accept_sec_context.3	\
	gss_acquire_cred.3		gss_add_cred.3			\
	gss_acquire_cred.3		gss_add_oid_set_member.3	\
	gss_acquire_cred.3		gss_canonicalize_name.3		\
	gss_acquire_cred.3		gss_compare_name.3		\
	gss_acquire_cred.3		gss_context_time.3		\
	gss_acquire_cred.3		gss_create_empty_oid_set.3	\
	gss_acquire_cred.3		gss_delete_sec_context.3	\
	gss_acquire_cred.3		gss_display_name.3		\
	gss_acquire_cred.3		gss_display_status.3		\
	gss_acquire_cred.3		gss_duplicate_name.3		\
	gss_acquire_cred.3		gss_export_name.3		\
	gss_acquire_cred.3		gss_export_sec_context.3	\
	gss_acquire_cred.3		gss_get_mic.3			\
	gss_acquire_cred.3		gss_import_name.3		\
	gss_acquire_cred.3		gss_import_sec_context.3	\
	gss_acquire_cred.3		gss_indicate_mechs.3		\
	gss_acquire_cred.3		gss_init_sec_context.3		\
	gss_acquire_cred.3		gss_inquire_context.3		\
	gss_acquire_cred.3		gss_inquire_cred.3		\
	gss_acquire_cred.3		gss_inquire_cred_by_mech.3	\
	gss_acquire_cred.3		gss_inquire_mechs_for_name.3	\
	gss_acquire_cred.3		gss_inquire_names_for_mech.3	\
	gss_acquire_cred.3		gss_krb5_ccache_name.3		\
	gss_acquire_cred.3		gss_krb5_compat_des3_mic.3	\
	gss_acquire_cred.3		gss_krb5_copy_ccache.3		\
	gss_acquire_cred.3		gss_krb5_get_tkt_flags.3	\
	gss_acquire_cred.3		gss_krb5_import_ccache.3	\
	gss_acquire_cred.3		gss_krb5_import_cred.3		\
	gss_acquire_cred.3		gss_process_context_token.3	\
	gss_acquire_cred.3		gss_release_buffer.3		\
	gss_acquire_cred.3		gss_release_cred.3		\
	gss_acquire_cred.3		gss_release_name.3		\
	gss_acquire_cred.3		gss_release_oid_set.3		\
	gss_acquire_cred.3		gss_seal.3			\
	gss_acquire_cred.3		gss_sign.3			\
	gss_acquire_cred.3		gss_test_oid_set_member.3	\
	gss_acquire_cred.3		gss_unseal.3			\
	gss_acquire_cred.3		gss_unwrap.3			\
	gss_acquire_cred.3		gss_verify.3			\
	gss_acquire_cred.3		gss_verify_mic.3		\
	gss_acquire_cred.3		gss_wrap.3			\
	gss_acquire_cred.3		gss_wrap_size_limit.3		\
	gss_acquire_cred.3		gsskrb5_extract_authz_data_from_sec_context.3 \
	gss_acquire_cred.3		gsskrb5_register_acceptor_identity.3

INCS=		gssapi.h
INCS+=		gssapi/gssapi.h gssapi/gssapi_krb5.h gssapi/gssapi_spnego.h
INCSDIR=	/usr/include

CLEANFILES+= ${BUILT_SOURCES} \
		${spnego_files} spnego_asn1_files \
		spnego_asn1.h spnego_asn1.hx \
		${gssapi_files} gssapi_asn1_files \
		gssapi_asn1.h gssapi_asn1.hx \
		gkrb5_err.h  \
		${krb5src} ${mechsrc} ${spnegosrc} ${ntlmsrc}

${spnego_files} spnego_asn1.hx: spnego_asn1_files
${gssapi_files} gssapi_asn1.hx: gssapi_asn1_files

spnego_asn1_files: spnego/spnego.asn1 ${TOOL_ASN1_COMPILE}
	${TOOL_ASN1_COMPILE} \
		 --sequence=MechTypeList \
		${DIST}/heimdal/lib/gssapi/spnego/spnego.asn1 spnego_asn1

gssapi_asn1_files: mech/gssapi.asn1 ${TOOL_ASN1_COMPILE}
	${TOOL_ASN1_COMPILE} \
		 ${DIST}/heimdal/lib/gssapi/mech/gssapi.asn1 gssapi_asn1

ASN1!=		cd ${NETBSDSRCDIR}/lib/libasn1 && ${PRINTOBJDIR}
KRB5!=		cd ${NETBSDSRCDIR}/lib/libkrb5 && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${DIST}/heimdal/include		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${DIST}/heimdal/lib/roken		\
	 -I${ASN1}				\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${KRB5}				\
	 -I${DIST}/heimdal/lib/krb5		\
	 -I${DIST}/heimdal/lib/gssapi		\
	 -I${DIST}/heimdal/lib/gssapi/mech	\
	 -I${DIST}/heimdal/lib/ntlm		\
	 -DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS+=	-Wno-pointer-sign
.endif

.include <bsd.lib.mk>

.SUFFIXES: .hx .x

.hx.h:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

.x.c:       
	@cmp -s $< $@ 2> /dev/null || cp $< $@

