#	$NetBSD: Makefile,v 1.8 2017/01/29 19:23:52 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}compile_et
HOST_SRCDIR=	crypto/external/bsd/heimdal/lib/libcom_err/compile_et
CPPFLAGS+=	-I.

.BEGIN:
	ln -sf ${NETBSDSRCDIR}/crypto/external/bsd/heimdal/include krb5

.include "${.CURDIR}/../Makefile.host"
