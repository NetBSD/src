#	$NetBSD: Makefile,v 1.3 2009/11/09 15:49:35 njoly Exp $
#	Generated from: NetBSD: build-makefiles,v 1.10 2009/11/09 15:46:04 njoly Exp 

NOLINT= # defined
NOMAN=	# defined
NONLS=	# defined
NOINFO=	# defined
NOSHARE=	# defined
NOLINKLIB=	# defined
LIBDPLIBS=	z	${.CURDIR}/../../../../../lib/libz

NOCHECKVER=	# defined

.include <bsd.obj.mk>

# Resolve pathnames in variables.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD LIBDPLIBS LIB_ROOT_DIR
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.include <bsd.own.mk>

_CURDIR:= ${.CURDIR}

.PATH: ${NETBSDSRCDIR}/external/gpl3/binutils/lib/libbfd
.CURDIR:=${NETBSDSRCDIR}/external/gpl3/binutils/lib/libbfd

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
.include "${.CURDIR}/Makefile"

# Resolve pathnames from "real" Makefile, and switch .CURDIR back.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD ARCHDIR COMPATDIR COMPATARCHDIR LIBCDIR RPC_INCS RPC_XDIR LIBEDITDIR MODOBJDIR RUMPTOP
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.CURDIR:=       ${_CURDIR}
.undef          _CURDIR
