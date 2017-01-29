#	$NetBSD: Makefile,v 1.7 2017/01/29 22:16:51 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}asn1_compile
HOST_SRCDIR=	crypto/external/bsd/heimdal/lib/libasn1/asn1_compile
CPPFLAGS+=	-I.

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	ln -sf ${NETBSDSRCDIR}/crypto/external/bsd/heimdal/include krb5
.endif

.include "${.CURDIR}/../Makefile.host"
