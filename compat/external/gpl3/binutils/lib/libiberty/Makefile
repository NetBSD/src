#	$NetBSD: Makefile,v 1.1 2009/09/11 16:01:58 mrg Exp $
#	Generated from: NetBSD: build-makefiles,v 1.7 2009/07/21 00:23:02 mrg Exp 

NOLINT= # defined
NOMAN=	# defined
NONLS=	# defined
NOINFO=	# defined
NOSHARE=	# defined

NOCHECKVER=	# defined

.include <bsd.obj.mk>

# Resolve pathnames in variables.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD LIBDPLIBS LIB_ROOT_DIR
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.include <bsd.own.mk>

_CURDIR:= ${.CURDIR}

.PATH: ${NETBSDSRCDIR}/external/gpl3/binutils/lib/libiberty
.CURDIR:=${NETBSDSRCDIR}/external/gpl3/binutils/lib/libiberty

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
.include "${.CURDIR}/Makefile"

# Resolve pathnames from "real" Makefile, and switch .CURDIR back.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD ARCHDIR COMPATDIR COMPATARCHDIR LIBCDIR RPC_INCS RPC_XDIR LIBEDITDIR MODOBJDIR RUMPTOP
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.CURDIR:=       ${_CURDIR}
.undef          _CURDIR
