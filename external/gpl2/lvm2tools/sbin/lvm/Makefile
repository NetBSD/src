

USE_FORT?=	no
NOLINT=		#defined

.include "../../lvm2tools.mk"

.PATH:	${LVM2TOOLS_DISTDIR}/man

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


MAN= 		lvchange.8 lvconvert.8 lvcreate.8 lvdisplay.8 lvextend.8 \
		lvm.8 lvm.conf.5 lvmchange.8 lvmdiskscan.8 lvmdump.8 \
		lvreduce.8 lvremove.8 lvrename.8 lvresize.8 lvs.8 \
		lvscan.8 pvchange.8 pvck.8 pvcreate.8 pvdisplay.8 \
		pvmove.8 pvremove.8 pvresize.8 pvs.8 pvscan.8 \
		vgcfgbackup.8 vgcfgrestore.8 vgchange.8 vgck.8 vgconvert.8 \
		vgcreate.8 vgdisplay.8 vgexport.8 vgextend.8 vgimport.8 \
		vgmerge.8 vgmknodes.8 vgreduce.8 vgremove.8 vgrename.8 \
		vgs.8 vgscan.8 vgsplit.8

CFLAGS+= -fno-inline

CPPFLAGS+=	-I${LVM2TOOLS_DISTDIR}/lib -I${LVM2TOOLS_DISTDIR}/include \
		-I. -I${LIBDM_INCLUDE}

CPPFLAGS+=	-DLVM_SHARED_PATH=\"$(BINDIR)/lvm\"

LIBLVM_OBJDIR!=	cd ${LVM2TOOLS_SRCDIR}/lib/liblvm && ${PRINTOBJDIR}
LDADD+=		-L${LIBLVM_OBJDIR} -llvm
DPADD+=		${LIBLVM_OBJDIR}/liblvm.a

LIBDM_OBJDIR!=	cd ${LIBDM_SRCDIR}/lib/libdevmapper && ${PRINTOBJDIR}
LDADD+=		-L${LIBDM_OBJDIR} -ldevmapper
DPADD+=		${LIBDM_OBJDIR}/libdevmapper.a

LDADD+=		-lprop

.PATH:	${LVM2TOOLS_DISTDIR}/tools

.include <bsd.prog.mk>
