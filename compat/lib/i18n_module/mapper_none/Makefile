#	$NetBSD: Makefile,v 1.1 2008/10/26 07:38:29 mrg Exp $

NOLINT= # defined
NOMAN=	# defined
NONLS=	# defined
NOINFO=	# defined
NOSHARE=	# defined

NOCHECKVER=	# defined

# XXX
.if ${MACHINE_ARCH} == "sparc64"
ARCHSUBDIR=		sparc
COMMON_ARCHSUBDIR=	sparc
.elif ${MACHINE_ARCH} == "sparc64"
ARCHSUBDIR=		i386
COMMON_ARCHSUBDIR=	i386
.endif

.include <bsd.obj.mk>

# Resolve pathnames in variables.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

_CURDIR:= ${.CURDIR}

.PATH: ${NETBSDSRCDIR}/lib/i18n_module/mapper_none
.CURDIR:=${NETBSDSRCDIR}/lib/i18n_module/mapper_none

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
.include "${.CURDIR}/Makefile"

# Resolve pathnames from "real" Makefile, and switch .CURDIR back.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD ARCHDIR COMPATDIR COMPATARCHDIR LIBCDIR RPC_INCS RPC_XDIR LIBEDITDIR MODOBJDIR RUMPTOP
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.CURDIR:=       ${_CURDIR}
.undef          _CURDIR
