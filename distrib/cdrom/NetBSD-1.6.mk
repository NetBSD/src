# $NetBSD: NetBSD-1.6.mk,v 1.1 2002/06/24 14:49:15 mrg Exp $
#
# Configuration file for the NetBSD 1.6 binary release.

# sysinst expects the architectures at top level
RELEASE_SUBDIR=		# empty

# BOOTFILE.alpha is absolute
BOOTFILE.alpha=		${EXTFILEDIR}/alpha.bootxx
EXTFILES.alpha=		alpha.bootxx:alpha/binary/sets/base.tgz,./usr/mdec/bootxx_cd9660
INTFILES.alpha=		netbsd.alpha:alpha/installation/instkernel/netbsd.gz,link \
			boot:alpha/binary/sets/base.tgz,./usr/mdec/boot

# BOOTFILE.i386 is relative to CD staging root
BOOTFILE.i386=		boot.i386
INTFILES.i386=		boot.i386:i386/installation/floppy/boot-big.fs,link

# macppc has external bootblock generation tool
EXTFILES.macppc=	macppc.ofwboot:macppc/binary/sets/base.tgz,./usr/mdec/ofwboot
INTFILES.macppc=	ofwboot.xcf:macppc/installation/ofwboot.xcf,link \
			netbsd.macppc:macppc/installation/netbsd.ram.gz,link

# BOOTFILE.pmax is absolute
BOOTFILE.pmax=		${EXTFILEDIR}/pmax.bootxx
EXTFILES.pmax=		pmax.bootxx:pmax/binary/sets/base.tgz,./usr/mdec/bootxx_cd9660
INTFILES.pmax=		netbsd.pmax:pmax/binary/kernel/install.gz,link \
			boot.pmax:pmax/binary/sets/base.tgz,./usr/mdec/boot.pmax

# BOOTFILE.sparc is absolute
BOOTFILE.sparc=		${EXTFILEDIR}/sparc-boot.fs
EXTFILES.sparc=		sparc-boot.fs:sparc/installation/bootfs/boot.fs.gz
INTFILES.sparc=		installation/bootfs/instfs.tgz:sparc/installation/bootfs/instfs.tgz,link
INTDIRS.sparc=		installation/bootfs
MKISOFS_ARGS.sparc=	-hide-hfs ./installation -hide-joliet ./installation

# BOOTFILE.vax is absolute
BOOTFILE.vax=		${EXTFILEDIR}/vax.xxboot
EXTFILES.vax=		vax.xxboot:vax/binary/sets/base.tgz,./usr/mdec/xxboot
INTFILES.vax=		netbsd.vax:vax/installation/netboot/install.ram.gz,link \
			boot.vax:vax/binary/sets/base.tgz,./usr/mdec/boot
VAX_IBOOT?=		${INSTALLBOOT} -m vax -o sunsum -o append
