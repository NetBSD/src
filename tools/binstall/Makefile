#	$NetBSD: Makefile,v 1.11.22.1 2018/05/05 19:25:56 martin Exp $

NOMAN=
.include <bsd.init.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-install
HOST_SRCDIR=	usr.bin/xinstall
CPPFLAGS+=	-I${.CURDIR}/../compat/sys
CPPFLAGS+=	-DTARGET_STRIP=\"${STRIP}\"

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

# Use uninstalled copy of the install program
INSTALL=	./xinstall

