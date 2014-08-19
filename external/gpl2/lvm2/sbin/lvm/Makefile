#	$NetBSD: Makefile,v 1.11.2.1 2014/08/19 23:52:23 tls Exp $

USE_FORT?=	no
NOLINT=		#defined
CWARNFLAGS.clang+=	-Wno-parentheses

.include "../../lvm2tools.mk"

.PATH:	${LVM2_DISTDIR}/man

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


CPPFLAGS+=	-I${LVM2_DISTDIR}/lib -I${LVM2_DISTDIR}/include \
		-I. -I${LIBDEVMAPPER_INCLUDE} -I${LVM2_DISTDIR}/tools \
		-I${LIBDEVMAPPER_DISTDIR}

CPPFLAGS+=	-DLVM_SHARED_PATH=\"$(BINDIR)/lvm\"

.include <bsd.own.mk>

# Some parts of the code can't be protected because if its
# alloca() usage.
#
.if defined(USE_SSP) && (${USE_SSP} != "no")
CPPFLAGS+=      -Wno-stack-protector
.endif

MAN= 		lvchange.8 lvconvert.8 lvcreate.8 lvdisplay.8 lvextend.8 \
		lvm.8 lvm.conf.5 lvmchange.8 lvmdiskscan.8 lvmdump.8 \
		lvreduce.8 lvremove.8 lvrename.8 lvresize.8 lvs.8 \
		lvscan.8 pvchange.8 pvck.8 pvcreate.8 pvdisplay.8 \
		pvmove.8 pvremove.8 pvresize.8 pvs.8 pvscan.8 \
		vgcfgbackup.8 vgcfgrestore.8 vgchange.8 vgck.8 vgconvert.8 \
		vgcreate.8 vgdisplay.8 vgexport.8 vgextend.8 vgimport.8 \
		vgmerge.8 vgmknodes.8 vgreduce.8 vgremove.8 vgrename.8 \
		vgs.8 vgscan.8 vgsplit.8


MKLINKS+=	lvchange.8 lvchange.1 \
                lvconvert.8 lvconvert.1 \
                lvcreate.8 lvcreate.1 \
                lvdisplay.8 lvdisplay.1 \
                lvextend.8 lvextend.1 \
                lvm.8 lvm.1 \
                lvm.conf lvm.1 \
                lvmchange.8 lvmchange.1 \
                lvmdiskscan.8 lvmdiskscan.1 \
                lvmdump.8 lvmdump.1 \
                lvreduce.8 lvreduce.1 \
                lvremove.8 lvremove.1 \
                lvrename.8 lvrename.1 \
                lvresize.8 lvresize.1 \
                lvs.8 lvs.1 \
                lvscan.8 lvscan.1 \
                pvchange.8 pvchange.1 \
                pvck.8 pvck.1 \
                pvcreate.8 pvcreate.1 \
                pvdisplay.8 pvdisplay.1 \
                pvmove.8 pvmove.1 \
                pvremove.8 pvremove.1 \
                pvresize.8 pvresize.1 \
                pvs.8 pvs.1 \
                pvscan.8 pvscan.1 \
                vgcfgbackup.8 vgcfgbackup.1 \
                vgcfgrestore.8 vgcfgrestore.1 \
                vgchange.8 vgchange.1 \
                vgck.8 vgck.1 \
                vgconvert.8 vgconvert.1 \
                vgcreate.8 vgcreate.1 \
                vgdisplay.8 vgdisplay.1 \
                vgexport.8 vgexport.1 \
                vgextend.8 vgextend.1 \
                vgimport.8 vgimport.1 \
                vgmerge.8 vgmerge.1 \
                vgmknodes.8 vgmknodes.1 \
                vgreduce.8 vgreduce.1 \
                vgremove.8 vgremove.1 \
                vgrename.8 vgrename.1 \
                vgs.8 vgs.1 \
                vgscan.8 vgscan.1 \
                vgsplit.8 vgsplit.1

LIBLVM_OBJDIR!= cd ${LVM2_SRCDIR}/lib/liblvm && ${PRINTOBJDIR}
LDADD+=         -L${LIBLVM_OBJDIR} -llvm
DPADD+=         ${LIBLVM_OBJDIR}/liblvm.a

LIBDEVMAPPER_OBJDIR!=  cd ${LIBDEVMAPPER_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBDEVMAPPER_OBJDIR} -ldevmapper
DPADD+=         ${LIBDEVMAPPER_OBJDIR}/libdevmapper.a

LIBDM_OBJDIR!=  cd ${LIBDM_SRCDIR} && ${PRINTOBJDIR}
#LDADD+=         -L${LIBDM_OBJDIR} -ldm
LDADD+=         ${LIBDM_OBJDIR}/libdm.a
DPADD+=         ${LIBDM_OBJDIR}/libdm.a

LDADD+=		-lprop -ledit -ltermcap

#LDSTATIC= 	-static

.PATH:	${LVM2_DISTDIR}/tools


.include <bsd.prog.mk>
