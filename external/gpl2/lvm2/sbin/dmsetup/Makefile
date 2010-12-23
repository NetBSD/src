#	$NetBSD: Makefile,v 1.5 2010/12/23 17:46:55 christos Exp $

USE_FORT?=	no
NOLINT=		#defined

.include "../../lvm2tools.mk"

.PATH:  ${LVM2_DISTDIR}/man

PROG=		dmsetup

MAN=		dmsetup.8

BINDIR=		/sbin

CPPFLAGS+=      -I${LIBDM_DISTDIR}/ -I${LIBDM_DISTDIR}/misc \
		-I. -I${LIBDM_DISTDIR}/ioctl -I${LVM2_DISTDIR}/include

CPPFLAGS+=	-D__LIB_DEVMAPPER__

LIBDM_OBJDIR!=	cd ${LIBDM_SRCDIR} && ${PRINTOBJDIR}
LDADD+=		-L${LIBDM_OBJDIR} -ldevmapper
DPADD+=		${LIBDM_OBJDIR}/libdevmapper.a

LDADD+=		-lprop

.PATH:	${LVM2_DISTDIR}/tools

.include <bsd.prog.mk>
