# $NetBSD: Makefile,v 1.13 2001/10/06 21:51:53 perry Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/asn1

WARNS?=  1
MKLINT= no

LIB=	asn1

.if !defined(USETOOLS) || (${USETOOLS} == "no")
COMPILEETOBJ!=	cd ${.CURDIR}/../libcom_err/compile_et && ${PRINTOBJDIR}
COMPILE_ET=	${COMPILEETOBJ}/compile_et
.endif

asn1_err.h asn1_err.c: asn1_err.et
	${COMPILE_ET} ${DIST}/heimdal/lib/asn1/asn1_err.et

DPSRCS=	asn1_err.h

INCS=	krb5_asn1.h asn1_err.h

INCSDIR= /usr/include/krb5

BUILT_SOURCES =			\
	${gen_files:.x=.c}	\
	asn1_err.h		\
	asn1_err.c

gen_files =				\
	asn1_APOptions.x		\
	asn1_AP_REP.x			\
	asn1_AP_REQ.x			\
	asn1_AS_REP.x			\
	asn1_AS_REQ.x			\
	asn1_Authenticator.x		\
	asn1_AuthorizationData.x	\
	asn1_CKSUMTYPE.x		\
	asn1_Checksum.x			\
	asn1_ENCTYPE.x			\
	asn1_ETYPE_INFO.x		\
	asn1_ETYPE_INFO_ENTRY.x		\
	asn1_EncAPRepPart.x		\
	asn1_EncASRepPart.x		\
	asn1_EncKDCRepPart.x		\
	asn1_EncKrbCredPart.x		\
	asn1_EncKrbPrivPart.x		\
	asn1_EncTGSRepPart.x		\
	asn1_EncTicketPart.x		\
	asn1_EncryptedData.x		\
	asn1_EncryptionKey.x		\
	asn1_HostAddress.x		\
	asn1_HostAddresses.x		\
	asn1_KDCOptions.x		\
	asn1_KDC_REP.x			\
	asn1_KDC_REQ.x			\
	asn1_KDC_REQ_BODY.x		\
	asn1_KRB_CRED.x			\
	asn1_KRB_ERROR.x		\
	asn1_KRB_PRIV.x			\
	asn1_KRB_SAFE.x			\
	asn1_KRB_SAFE_BODY.x		\
	asn1_KerberosTime.x		\
	asn1_KrbCredInfo.x		\
	asn1_LastReq.x			\
	asn1_LR_TYPE.x			\
	asn1_MESSAGE_TYPE.x		\
	asn1_METHOD_DATA.x		\
	asn1_NAME_TYPE.x		\
	asn1_PADATA_TYPE.x		\
	asn1_PA_DATA.x			\
	asn1_PA_ENC_TS_ENC.x		\
	asn1_Principal.x		\
	asn1_PrincipalName.x		\
	asn1_Realm.x			\
	asn1_TGS_REP.x			\
	asn1_TGS_REQ.x			\
	asn1_Ticket.x			\
	asn1_TicketFlags.x		\
	asn1_TransitedEncoding.x	\
	asn1_UNSIGNED.x

SRCS=	der_get.c \
	der_put.c \
	der_free.c \
	der_length.c \
	der_copy.c \
	timegm.c \
	${BUILT_SOURCES}

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -DHAVE_CONFIG_H

${gen_files} krb5_asn1.hx: asn1_files

ASN1COMPILE!=	cd ${.CURDIR}/asn1_compile && ${PRINTOBJDIR}
ASN1_COMPILE=	${ASN1COMPILE}/asn1_compile

asn1_files: k5.asn1 all-asn1_compile
	${ASN1_COMPILE} ${DIST}/heimdal/lib/asn1/k5.asn1 krb5_asn1

${SRCS}: krb5_asn1.h

CLEANFILES = ${BUILT_SOURCES} ${gen_files} asn1_files krb5_asn1.h krb5_asn1.hx

SUBDIR=	asn1_compile

.include <bsd.lib.mk>
.include <bsd.subdir.mk>

.SUFFIXES: .hx .x

.x.c:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

.hx.h:
	@cmp -s $< $@ 2> /dev/null || cp $< $@
