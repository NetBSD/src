#	$NetBSD: Makefile,v 1.1.2.3 2008/07/23 13:44:02 lukem Exp $

USE_FORT?=	no
NOLINT=		#defined
NOMAN=		#defined

.include "../../lvm2tools.mk"

PROG=		lvm

BINDIR=		/sbin

SRCS=		dumpconfig.c formats.c lvchange.c lvconvert.c lvcreate.c \
		lvdisplay.c lvextend.c lvmchange.c lvmcmdline.c \
		lvmdiskscan.c lvreduce.c lvremove.c lvrename.c lvresize.c \
		lvscan.c polldaemon.c pvchange.c pvck.c pvcreate.c \
		pvdisplay.c pvmove.c pvremove.c pvresize.c pvscan.c \
		reporter.c segtypes.c toollib.c vgcfgbackup.c \
		vgcfgrestore.c vgchange.c vgck.c vgconvert.c vgcreate.c \
		vgdisplay.c vgexport.c vgextend.c vgimport.c vgmerge.c \
		vgmknodes.c vgreduce.c vgremove.c vgrename.c vgscan.c \
		vgsplit.c lvm.c

CPPFLAGS+=	-I${LVM2TOOLS_DISTDIR}/lib -I${LVM2TOOLS_DISTDIR}/include \
		-I. -I${LIBDM_INCLUDE}

CPPFLAGS+=	-DLVM_SHARED_PATH=\"$(BINDIR)/lvm\"

LDADD+=		-lprop

LIBDM_OBJDIR!=	cd ${LIBDM_SRCDIR}/lib/libdevmapper && ${PRINTOBJDIR}
LDADD+=		-L${LIBDM_OBJDIR} -ldevmapper
DPADD+=		${LIBDM_OBJDIR}/libdevmapper.a

LIBLVM_OBJDIR!=	cd ${LVM2TOOLS_SRCDIR}/lib/liblvm && ${PRINTOBJDIR}
LDADD+=		-L${LIBLVM_OBJDIR} -llvm
DPADD+=		${LIBLVM_OBJDIR}/liblvm.a

.PATH:	${LVM2TOOLS_DISTDIR}/tools
.PATH:	${LVM2TOOLS_DISTDIR}/man

.include <bsd.prog.mk>
