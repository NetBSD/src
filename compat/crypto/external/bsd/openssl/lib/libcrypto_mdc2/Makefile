#	$NetBSD: Makefile,v 1.1 2009/07/19 23:36:49 christos Exp $

NOLINT= # defined
NOMAN=	# defined
NONLS=	# defined
NOINFO=	# defined
NOSHARE=	# defined
NOPIC=	# defined

NOCHECKVER=	# defined

.include <bsd.obj.mk>

# Resolve pathnames in variables.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD LIBDPLIBS LIB_ROOT_DIR
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

_CURDIR:= ${.CURDIR}

.PATH: ${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto_mdc2
.CURDIR:=${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto_mdc2

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
.include "${.CURDIR}/Makefile"

# Resolve pathnames from "real" Makefile, and switch .CURDIR back.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD ARCHDIR COMPATDIR COMPATARCHDIR LIBCDIR RPC_INCS RPC_XDIR LIBEDITDIR MODOBJDIR RUMPTOP
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.CURDIR:=       ${_CURDIR}
.undef          _CURDIR
