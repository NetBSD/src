USE_FORT ?= no
MKLINT = no

.include "../../lvm2tools.mk"

PROG=           lvm

BINDIR=         /sbin

SRCS= dumpconfig.c formats.c lvchange.c lvconvert.c lvcreate.c lvdisplay.c  \
      lvextend.c lvmchange.c lvmcmdline.c lvmdiskscan.c lvreduce.c          \
      lvremove.c lvrename.c lvresize.c lvscan.c polldaemon.c pvchange.c     \
      pvck.c pvcreate.c pvdisplay.c pvmove.c pvremove.c pvresize.c pvscan.c \
      reporter.c segtypes.c toollib.c vgcfgbackup.c vgcfgrestore.c	    \
      vgchange.c vgck.c vgconvert.c vgcreate.c vgdisplay.c vgexport.c       \
      vgextend.c vgimport.c vgmerge.c vgmknodes.c vgreduce.c vgremove.c     \
      vgrename.c vgscan.c vgsplit.c lvm.c

CPPFLAGS+=      -I${LVM2TOOLS_DISTDIR}/lib -I${LVM2TOOLS_DISTDIR}/include\
                -I.  -I${LIBDM_INCLUDE}

CFLAGS+= -I${LVM2TOOLS_DISTDIR}/lib -I${LVM2TOOLS_DISTDIR}/include\
         -I. -I${LIBDM_INCLUDE}

CFLAGS+= -DLVM_SHARED_PATH=\"$(BINDIR)/lvm\"

LDADD+= -L${LVM2TOOLSOBJDIR.liblvm} -llvm
DPADD+= ${LVM2TOOLSOBJDIR.liblvm}

LDADD+= -L${LIBDM_OBJDIR.libdevmapper} -ldevmapper

LDADD+= -lprop

.PATH:  ${LVM2TOOLS_DISTDIR}/tools
.PATH:  ${LVM2TOOLS_DISTDIR}/man

#MAN= dmsetup.8

#MLINKS= dmsetup.8 dmsetup.1

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
