# $NetBSD: Makefile,v 1.2 2000/06/16 22:46:45 thorpej Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/asn1

WARNS?=  1
MKLINT= no

LIB=	asn1

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && make print-objdir

asn1_err.h asn1_err.c: asn1_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/asn1/asn1_err.et

DPSRCS=	asn1_err.h

INCS=	asn1.h asn1_err.h

INCSDIR= /usr/include/krb5

BUILT_SOURCES =			\
	$(gen_files:.x=.c)	\
	asn1_err.h		\
	asn1_err.c

gen_files =				\
	asn1_UNSIGNED.x			\
	asn1_APOptions.x		\
	asn1_AP_REP.x			\
	asn1_AP_REQ.x			\
	asn1_AS_REP.x			\
	asn1_AS_REQ.x			\
	asn1_Authenticator.x		\
	asn1_AuthorizationData.x	\
	asn1_Checksum.x			\
	asn1_EncAPRepPart.x		\
	asn1_EncASRepPart.x		\
	asn1_EncKDCRepPart.x		\
	asn1_EncKrbCredPart.x		\
	asn1_EncKrbPrivPart.x		\
	asn1_EncTGSRepPart.x		\
	asn1_EncTicketPart.x		\
	asn1_EncryptedData.x		\
	asn1_EncryptionKey.x		\
	asn1_ETYPE_INFO.x		\
	asn1_ETYPE_INFO_ENTRY.x		\
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
	asn1_METHOD_DATA.x		\
	asn1_PA_DATA.x			\
	asn1_PA_ENC_TS_ENC.x		\
	asn1_Principal.x		\
	asn1_PrincipalName.x		\
	asn1_Realm.x			\
	asn1_TGS_REP.x			\
	asn1_TGS_REQ.x			\
	asn1_Ticket.x			\
	asn1_TicketFlags.x		\
	asn1_TransitedEncoding.x

SRCS=	der_get.c \
	der_put.c \
	der_free.c \
	der_length.c \
	der_copy.c \
	timegm.c \
	$(BUILT_SOURCES)

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -DHAVE_CONFIG_H

$(gen_files) asn1.hx: asn1_files

ASN1COMPILE!=	cd ${.CURDIR}/asn1_compile && ${MAKE} print-objdir

asn1_files: k5.asn1 all-asn1_compile
	${ASN1COMPILE}/asn1_compile ${DIST}/heimdal/lib/asn1/k5.asn1

asn1.h:	asn1.hx
${SRCS}: asn1.h

CLEANFILES = $(BUILT_SOURCES) $(gen_files) asn1_files asn1.h asn1.hx

SUBDIR=	asn1_compile

.include <bsd.lib.mk>
.include <bsd.subdir.mk>

.SUFFIXES: .hx .x

.x.c:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

.hx.h:
	@cmp -s $< $@ 2> /dev/null || cp $< $@
