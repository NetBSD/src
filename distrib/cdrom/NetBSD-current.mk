# $NetBSD: NetBSD-current.mk,v 1.8 2010/10/05 14:16:20 tsutsui Exp $
#
# Configuration file for NetBSD-current.

# sysinst expects the architectures at top level
RELEASE_SUBDIR=		# empty

# BOOTFILE.alpha is absolute
BOOTFILE.alpha=		${EXTFILEDIR}/alpha.bootxx
EXTFILES.alpha=		alpha.bootxx:alpha/binary/sets/base.tgz,./usr/mdec/bootxx_cd9660
INTFILES.alpha=		netbsd.alpha:alpha/installation/instkernel/netbsd.gz,link \
			boot:alpha/binary/sets/base.tgz,./usr/mdec/boot

# cats needs an a.out kernel to boot from
INTFILES.cats=		netbsd.cats:cats/binary/kernel/netbsd-INSTALL.aout.gz

# BOOTFILE.i386 is relative to CD staging root
BOOTFILE.i386=		boot.i386
INTFILES.i386=		boot.i386:i386/binary/sets/base.tgz,./usr/mdec/bootxx_cd9660 \
			boot:i386/binary/sets/base.tgz,./usr/mdec/boot \
			netbsd:i386/binary/kernel/netbsd-INSTALL.gz,link

# BOOTFILE.amd64 is relative to CD staging root
BOOTFILE.amd64=		boot.amd64
INTFILES.amd64=		boot.amd64:amd64/binary/sets/base.tgz,./usr/mdec/bootxx_cd9660 \
			boot:amd64/binary/sets/base.tgz,./usr/mdec/boot \
			netbsd:amd64/binary/kernel/netbsd-INSTALL.gz,link

# macppc has external bootblock generation tool
EXTFILES.macppc=	macppc.bootxx:macppc/binary/sets/base.tgz,./usr/mdec/bootxx \
			macppc.ofwboot:macppc/binary/sets/base.tgz,./usr/mdec/ofwboot
INTFILES.macppc=	ofwboot.xcf:macppc/installation/ofwboot.xcf,link \
			ofwboot:macppc/binary/sets/base.tgz,./usr/mdec/ofwboot \
			netbsd.macppc:macppc/binary/kernel/netbsd-INSTALL.gz,link

# BOOTFILE.pmax is absolute
BOOTFILE.pmax=		${EXTFILEDIR}/pmax.bootxx
EXTFILES.pmax=		pmax.bootxx:pmax/binary/sets/base.tgz,./usr/mdec/bootxx_cd9660
INTFILES.pmax=		netbsd.pmax:pmax/binary/kernel/netbsd-INSTALL.gz,link \
			boot.pmax:pmax/binary/sets/base.tgz,./usr/mdec/boot.pmax

# sgimips needs the compressed kernels with short names in /, the various
# boot files that go into the volume header and the bootblock.h file
# to grep for the volume header size
EXTFILES.sgimips=	sgimips.bootblock.h:sgimips/binary/sets/comp.tgz,./usr/include/sys/bootblock.h
INTFILES.sgimips=	aoutboot:sgimips/binary/sets/base.tgz,./usr/mdec/aoutboot \
			ip2xboot:sgimips/binary/sets/base.tgz,./usr/mdec/ip2xboot \
			ip3xboot:sgimips/binary/sets/base.tgz,./usr/mdec/ip3xboot \
			ip2x:sgimips/binary/kernel/netbsd-INSTALL32_IP2x.gz,link \
			ip3x:sgimips/binary/kernel/netbsd-INSTALL32_IP3x.gz,link


# BOOTFILE.sparc is absolute
BOOTFILE.sparc=		${EXTFILEDIR}/sparc-boot.fs
EXTFILES.sparc=		sparc-boot.fs:sparc/installation/bootfs/boot.fs.gz
INTFILES.sparc=		installation/bootfs/instfs.tgz:sparc/installation/bootfs/instfs.tgz,link
INTDIRS.sparc=		installation/bootfs
MKISOFS_ARGS.sparc=	-hide-hfs ./installation -hide-joliet ./installation

# BOOTFILE.sparc64 is absolute
BOOTFILE.sparc64=	${EXTFILEDIR}/sparc64-boot.fs
EXTFILES.sparc64=	sparc64-boot.fs:sparc64/installation/misc/boot.fs.gz

# BOOTFILE.vax is absolute
BOOTFILE.vax=		${EXTFILEDIR}/vax.xxboot
EXTFILES.vax=		vax.xxboot:vax/binary/sets/base.tgz,./usr/mdec/xxboot
INTFILES.vax=		netbsd.vax:vax/installation/netboot/install.ram.gz,link \
			boot.vax:vax/binary/sets/base.tgz,./usr/mdec/boot
