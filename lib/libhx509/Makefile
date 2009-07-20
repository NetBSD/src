# $NetBSD: Makefile,v 1.6 2009/07/20 12:16:37 skrll Exp $

USE_FORT?= yes	# cryptographic software

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/hx509 ${DIST}/heimdal/lib/hx509/ref

WARNS?=		1

LIB=		hx509

LIBDPLIBS+=	crypto	${.CURDIR}/../../crypto/external/bsd/openssl/lib/libcrypto \
		asn1	${.CURDIR}/../libasn1 \
		roken	${.CURDIR}/../libroken

.if ${USETOOLS} != "yes"
COMPILEETOBJ!=		cd ${NETBSDSRCDIR}/lib/libcom_err/compile_et && ${PRINTOBJDIR}
TOOL_COMPILE_ET=	${COMPILEETOBJ}/compile_et

ASN1COMPILEOBJ!=	cd ${NETBSDSRCDIR}/lib/libasn1/asn1_compile && ${PRINTOBJDIR}
TOOL_ASN1_COMPILE=	${ASN1COMPILEOBJ}/asn1_compile
.endif

hx509_err.c hx509_err.h: hx509_err.et ${TOOL_COMPILE_ET}
	${TOOL_COMPILE_ET} ${DIST}/heimdal/lib/hx509/hx509_err.et

ERR_FILES=	hx509_err.c

gen_files_ocsp=					\
	   asn1_OCSPBasicOCSPResponse.x		\
	   asn1_OCSPCertID.x			\
	   asn1_OCSPCertStatus.x		\
	   asn1_OCSPInnerRequest.x		\
	   asn1_OCSPKeyHash.x			\
	   asn1_OCSPRequest.x			\
	   asn1_OCSPResponderID.x		\
	   asn1_OCSPResponse.x			\
	   asn1_OCSPResponseBytes.x		\
	   asn1_OCSPResponseData.x		\
	   asn1_OCSPResponseStatus.x		\
	   asn1_OCSPSignature.x			\
	   asn1_OCSPSingleResponse.x		\
	   asn1_OCSPTBSRequest.x		\
	   asn1_OCSPVersion.x			\
	   asn1_id_pkix_ocsp.x			\
	   asn1_id_pkix_ocsp_basic.x		\
	   asn1_id_pkix_ocsp_nonce.x

gen_files_pkcs10=				\
	   asn1_CertificationRequestInfo.x	\
	   asn1_CertificationRequest.x

gen_files_crmf=					\
	   asn1_CRMFRDNSequence.x		\
	   asn1_CertReqMessages.x		\
	   asn1_CertReqMsg.x			\
	   asn1_CertRequest.x			\
	   asn1_CertTemplate.x			\
	   asn1_Controls.x			\
	   asn1_PBMParameter.x			\
	   asn1_PKMACValue.x			\
	   asn1_POPOPrivKey.x			\
	   asn1_POPOSigningKey.x		\
	   asn1_POPOSigningKeyInput.x		\
	   asn1_ProofOfPossession.x		\
	   asn1_SubsequentMessage.x


DPSRCS=		ocsp_asn1.h pkcs10_asn1.h crmf_asn1.h hx509_err.h

BUILT_SOURCES=	\
	${gen_files_ocsp:.x=.c} \
	${gen_files_pkcs10:.x=.c} \
	${ERR_FILES}

SRCS= \
	    ca.c			\
	    cert.c			\
	    cms.c			\
	    collector.c			\
	    crypto.c			\
	    doxygen.c			\
	    error.c			\
	    env.c			\
	    file.c			\
	    keyset.c			\
	    ks_dir.c			\
	    ks_file.c			\
	    ks_mem.c			\
	    ks_null.c			\
	    ks_p11.c			\
	    ks_p12.c			\
	    ks_keychain.c		\
	    lock.c			\
	    name.c			\
	    peer.c			\
	    print.c			\
	    softp11.c			\
	    req.c			\
	    revoke.c			\
	    $(BUILT_SOURCES)

INCS=		hx509.h hx509_err.h \
		ocsp_asn1.h pkcs10_asn1.h crmf_asn1.h \
		hx509-protos.h hx509-private.h
INCSDIR=	/usr/include/krb5

ocsp_asn1_files: ocsp.asn1 ${TOOL_ASN1_COMPILE}
	${TOOL_ASN1_COMPILE} \
		--preserve-binary=OCSPTBSRequest \
		--preserve-binary=OCSPResponseData \
		${DIST}/heimdal/lib/hx509/ocsp.asn1 ocsp_asn1
pkcs10_asn1_files: pkcs10.asn1 ${TOOL_ASN1_COMPILE}
	${TOOL_ASN1_COMPILE} \
		 --preserve-binary=CertificationRequestInfo \
		${DIST}/heimdal/lib/hx509/pkcs10.asn1 pkcs10_asn1
crmf_asn1_files: crmf.asn1 ${TOOL_ASN1_COMPILE}
	${TOOL_ASN1_COMPILE} ${DIST}/heimdal/lib/hx509/crmf.asn1 crmf_asn1

ASN1!=		cd ${NETBSDSRCDIR}/lib/libasn1 && ${PRINTOBJDIR}
KRB5!=		cd ${NETBSDSRCDIR}/lib/libkrb5 && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/hx509		\
	 -I${DIST}/heimdal/lib/hx509/ref	\
	 -I${ASN1}				\
	 -I${KRB5}				\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${NETBSDSRCDIR}/lib/libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in mkey
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif

${gen_files_ocsp} ocsp_asn1.hx: ocsp_asn1_files
${gen_files_pkcs10} pkcs10_asn1.hx: pkcs10_asn1_files
${gen_files_crmf} crmf_asn1.hx: crmf_asn1_files

CLEANFILES=	${gen_files_ocsp} ${gen_files_pkcs10} ${gen_files_crmf} \
		${BUILT_SOURCES} \
		ocsp_asn1_files ocsp_asn1.h ocsp_asn1.hx \
		pkcs10_asn1_files pkcs10_asn1.h pkcs10_asn1.hx \
		crmf_asn1_files crmf_asn1.h crmf_asn1.hx \
		hx509_err.h

.include <bsd.lib.mk>

.SUFFIXES: .hx .x

.hx.h:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

.x.c:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

