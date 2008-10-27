#	$NetBSD: Makefile,v 1.3 2008/10/27 07:22:54 mrg Exp $

NOLINT= # defined
NOMAN=	# defined
NONLS=	# defined
NOINFO=	# defined
NOSHARE=	# defined
LIBDPLIBS=	hx509	${.CURDIR}/../libhx509  crypto	${.CURDIR}/../libcrypto  asn1	${.CURDIR}/../libasn1  com_err	${.CURDIR}/../libcom_err  roken	${.CURDIR}/../libroken

NOCHECKVER=	# defined

.include <bsd.obj.mk>

# Resolve pathnames in variables.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD LIBDPLIBS
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

_CURDIR:= ${.CURDIR}

.PATH: ${NETBSDSRCDIR}/lib/libkrb5
.CURDIR:=${NETBSDSRCDIR}/lib/libkrb5

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
.include "${.CURDIR}/Makefile"

# Resolve pathnames from "real" Makefile, and switch .CURDIR back.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD ARCHDIR COMPATDIR COMPATARCHDIR LIBCDIR RPC_INCS RPC_XDIR LIBEDITDIR MODOBJDIR RUMPTOP
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.CURDIR:=       ${_CURDIR}
.undef          _CURDIR
