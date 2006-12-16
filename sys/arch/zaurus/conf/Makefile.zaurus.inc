#	$NetBSD: Makefile.zaurus.inc,v 1.1 2006/12/16 05:08:47 ober Exp $

MACHINE_ARCH=		arm
CPPFLAGS+=		-D${MACHINE}

SYSTEM_FIRST_OBJ=	zaurus_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/zaurus/zaurus_start.S

KERNEL_BASE_PHYS=	0xa0200000
KERNEL_BASE_VIRT=	0xc0200000

LINKFLAGS=		-T ldscript

netbsd: ldscript             # XXX
EXTRA_CLEAN+= ldscript tmp

# generate ldscript from common template 
ldscript: ${THISARM}/conf/ldscript.zaurus ${THISARM}/conf/Makefile.zaurus.inc
	echo ${KERNELS}
	sed -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.zaurus > tmp && mv tmp $@
