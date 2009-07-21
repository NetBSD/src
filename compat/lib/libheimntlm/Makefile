#	$NetBSD: Makefile,v 1.5 2009/07/21 01:00:45 mrg Exp $
#	Generated from: NetBSD: build-makefiles,v 1.7 2009/07/21 00:23:02 mrg Exp 

NOLINT= # defined
NOMAN=	# defined
NONLS=	# defined
NOINFO=	# defined
NOSHARE=	# defined
LIBDPLIBS=	asn1    ${.CURDIR}/../libasn1  krb5    ${.CURDIR}/../libkrb5  roken  ${.CURDIR}/../libroken

NOCHECKVER=	# defined

.include <bsd.obj.mk>

# Resolve pathnames in variables.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD LIBDPLIBS LIB_ROOT_DIR
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.include <bsd.own.mk>

_CURDIR:= ${.CURDIR}

.PATH: ${NETBSDSRCDIR}/lib/libheimntlm
.CURDIR:=${NETBSDSRCDIR}/lib/libheimntlm

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
.include "${.CURDIR}/Makefile"

# Resolve pathnames from "real" Makefile, and switch .CURDIR back.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD ARCHDIR COMPATDIR COMPATARCHDIR LIBCDIR RPC_INCS RPC_XDIR LIBEDITDIR MODOBJDIR RUMPTOP
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.CURDIR:=       ${_CURDIR}
.undef          _CURDIR
