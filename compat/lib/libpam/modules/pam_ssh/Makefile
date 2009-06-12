#	$NetBSD: Makefile,v 1.4 2009/06/12 06:40:51 mrg Exp $

NOLINT= # defined
NOMAN=	# defined
NONLS=	# defined
NOINFO=	# defined
NOSHARE=	# defined
LIBDPLIBS=	ssh	${.CURDIR}/../../../../crypto/external/bsd/openssh/lib  crypt	${.CURDIR}/../../../libcrypt  crypto	${.CURDIR}/../../../libcrypto

NOCHECKVER=	# defined

.include <bsd.obj.mk>

# Resolve pathnames in variables.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD LIBDPLIBS LIB_ROOT_DIR
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

_CURDIR:= ${.CURDIR}

.PATH: ${NETBSDSRCDIR}/lib/libpam/modules/pam_ssh
.CURDIR:=${NETBSDSRCDIR}/lib/libpam/modules/pam_ssh

.include "${NETBSDSRCDIR}/compat/Makefile.compat"
.include "${.CURDIR}/Makefile"

# Resolve pathnames from "real" Makefile, and switch .CURDIR back.
_RESOLVE_VARS=  CFLAGS CPPFLAGS DPADD LDADD ARCHDIR COMPATDIR COMPATARCHDIR LIBCDIR RPC_INCS RPC_XDIR LIBEDITDIR MODOBJDIR RUMPTOP
.for var in ${_RESOLVE_VARS}
${var}:=        ${${var}}
.endfor

.CURDIR:=       ${_CURDIR}
.undef          _CURDIR
