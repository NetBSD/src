# $NetBSD: Makefile.inc,v 1.7 2023/06/20 17:23:01 christos Exp $

HEIMBASE?=	${NETBSDSRCDIR}/crypto/external/bsd/heimdal
HEIMDIST=	${HEIMBASE}/dist

SSLBASE?=	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}

DPLIBASN1=	asn1 ${HEIMBASE}/lib/libasn1
DPLIBCOM_ERR=	com_err ${HEIMBASE}/lib/libcom_err
DPLIBGSSAPI=	gssapi ${HEIMBASE}/lib/libgssapi
DPLIBHDB=	hdb ${HEIMBASE}/lib/libhdb
DPLIBHEIMBASE=	heimbase ${HEIMBASE}/lib/libheimbase
DPLIBHEIMNTLM=	heimntlm ${HEIMBASE}/lib/libheimntlm
DPLIBHX509=	hx509 ${HEIMBASE}/lib/libhx509
DPLIBIPC=	ipc ${HEIMBASE}/lib/libipc
DPLIBKADM5CLNT=	kadm5clnt ${HEIMBASE}/lib/libkadm5clnt
DPLIBKADM5SRV=	kadm5srv ${HEIMBASE}/lib/libkadm5srv
DPLIBKAFS=	kafs ${HEIMBASE}/lib/libkafs
DPLIBKDC=	kdc ${HEIMBASE}/lib/libkdc
DPLIBKRB5=	krb5 ${HEIMBASE}/lib/libkrb5
DPLIBROKEN=	roken ${HEIMBASE}/lib/libroken
DPLIBSL=	sl ${HEIMBASE}/lib/libsl
DPLIBVERS=	vers ${HEIMBASE}/lib/libvers
DPLIBWIND=	wind ${HEIMBASE}/lib/libwind

.if ${USETOOLS} != "yes"
COMPILEETOBJ!=     cd ${HEIMBASE}/lib/libcom_err/compile_et && ${PRINTOBJDIR}
TOOL_COMPILE_ET=   ${COMPILEETOBJ}/compile_et

ASN1COMPILEOBJ!=   cd ${HEIMBASE}/lib/libasn1/asn1_compile && ${PRINTOBJDIR}
TOOL_ASN1_COMPILE= ${ASN1COMPILEOBJ}/asn1_compile

SLCOBJ!=	cd ${HEIMBASE}/lib/libsl/slc && ${PRINTOBJDIR}
TOOL_SLC=	${SLCOBJ}/slc
.endif

#
# We now define what are standard CPP flags across Heimdal:

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
HOST_CPPFLAGS+=-DHAVE_IPV6
.endif

.if ${MKPIC} == "no"
CPPFLAGS+=	-DNO_DLOPEN
.endif

CPPFLAGS+=	-DHAVE_CONFIG_H
HOST_CPPFLAGS+=	-DHAVE_CONFIG_H

CPPFLAGS+= ${.PATH:S/^/-I/}
CPPFLAGS+= -I${HEIMBASE}/include
CPPFLAGS+= -I${HEIMDIST}/include
CWARNFLAGS.clang+=	-Wno-error=absolute-value
