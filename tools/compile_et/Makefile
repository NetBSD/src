#	$NetBSD: Makefile,v 1.9 2017/01/29 22:16:51 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}compile_et
HOST_SRCDIR=	crypto/external/bsd/heimdal/lib/libcom_err/compile_et
CPPFLAGS+=	-I.

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	ln -sf ${NETBSDSRCDIR}/crypto/external/bsd/heimdal/include krb5
.endif

.include "${.CURDIR}/../Makefile.host"
