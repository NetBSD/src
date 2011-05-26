#	$NetBSD: Makefile,v 1.8 2011/05/26 12:56:26 joerg Exp $

USE_FORT?=	no
NOLINT=		#defined
CWARNFLAGS.clang+=	-Wno-parentheses -Wno-unused-value

.include "../../lvm2tools.mk"

.PATH:  ${LVM2_DISTDIR}/man

PROG=		dmsetup

MAN=		dmsetup.8

BINDIR=		/sbin

#CFLAGS+= -g -O0

CPPFLAGS+=      -I${LIBDEVMAPPER_DISTDIR}/ -I${LIBDEVMAPPER_DISTDIR}/misc \
		-I. -I${LIBDEVMAPPER_DISTDIR}/ioctl -I${LVM2_DISTDIR}/include

CPPFLAGS+=	-D__LIB_DEVMAPPER__

LIBDEVMAPPER_OBJDIR!=	cd ${LIBDEVMAPPER_SRCDIR} && ${PRINTOBJDIR}
LDADD+=		-L${LIBDEVMAPPER_OBJDIR} -ldevmapper
DPADD+=		${LIBDEVMAPPER_OBJDIR}/libdevmapper.a

LIBDM_OBJDIR!=	cd ${LIBDM_SRCDIR} && ${PRINTOBJDIR}
#LDADD+=		-L${LIBDM_OBJDIR} -ldm
LDADD+=		${LIBDM_OBJDIR}/libdm.a
DPADD+=		${LIBDM_OBJDIR}/libdm.a

LDADD+=		-lprop

#LDSTATIC= -static
.PATH:	${LVM2_DISTDIR}/tools

.include <bsd.prog.mk>
