#	$NetBSD: Makefile,v 1.6 2017/01/29 19:23:52 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}asn1_compile
HOST_SRCDIR=	crypto/external/bsd/heimdal/lib/libasn1/asn1_compile
CPPFLAGS+=	-I.

.BEGIN:
	ln -sf ${NETBSDSRCDIR}/crypto/external/bsd/heimdal/include krb5

.include "${.CURDIR}/../Makefile.host"
