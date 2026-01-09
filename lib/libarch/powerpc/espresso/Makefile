#	$NetBSD: Makefile,v 1.1 2026/01/09 22:54:27 jmcneill Exp $

NOLINKLIB=	# defined
NOPROFILE=	# defined
NOLINT=		# defined
USE_SHLIBDIR=	yes
LIB=		powerpc_espresso

# Avoid linking against libc.
DPLIBC=		# nothing
LDLIBC=		-nodefaultlibs

AFLAGS+=	-mcpu=750 -DPPC_IBMESPRESSO

_ATOMIC_PATH=	${NETBSDSRCDIR}/common/lib/libc/arch/powerpc/atomic
.PATH: ${_ATOMIC_PATH}
.include "${_ATOMIC_PATH}/Makefile.inc"

.include <bsd.lib.mk>
