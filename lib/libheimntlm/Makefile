# $NetBSD: Makefile,v 1.1.4.1 2008/09/18 04:39:23 wrstuden Exp $

USE_FORT?= yes	# cryptographic software

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/ntlm
WARNS?=		1

LIB=		heimntlm

LIBDPLIBS=	asn1    ${.CURDIR}/../libasn1 \
		krb5    ${.CURDIR}/../libkrb5 \
		roken  ${.CURDIR}/../libroken


SRCS=		ntlm.c

INCS=		heimntlm.h heimntlm-protos.h

INCSDIR=	/usr/include/krb5

ASN1!=          cd ${NETBSDSRCDIR}/lib/libasn1 && ${PRINTOBJDIR}
KRB5!=		cd ${NETBSDSRCDIR}/lib/libkrb5 && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/lib/ntlm		\
	-I${ASN1}				\
	-I${KRB5}				\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DIST}/heimdal/include		\
        -I${NETBSDSRCDIR}/include/heimdal	\
	-I${NETBSDSRCDIR}/lib/libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif          

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS+= -Wno-pointer-sign
.endif

.include <bsd.lib.mk>
