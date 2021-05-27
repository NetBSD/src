#	$NetBSD: Makefile,v 1.2 2021/05/27 08:41:35 cjep Exp $

.include <bsd.hostinit.mk>

HOSTPROGNAME=	${_TOOL_PREFIX}date
HOST_SRCDIR=	bin/date

# Prevent Makefile.host from trying to use the version of libnbcompat
# from ${TOOLDIR}.
NOCOMPATLIB=

# Use uninstalled copy of host-mkdep
HOST_MKDEP_OBJ!= cd ${.CURDIR}/../host-mkdep && ${PRINTOBJDIR}
HOST_MKDEP=	${HOST_MKDEP_OBJ}/host-mkdep
MKDEP=		${HOST_MKDEP}

# Use uninstalled copy of libnbcompat and associated *.h files
COMPATLIB_UNINSTALLED= yes
COMPATOBJ!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
.-include	"${COMPATOBJ}/defs.mk"

.include "${.CURDIR}/../Makefile.host"
