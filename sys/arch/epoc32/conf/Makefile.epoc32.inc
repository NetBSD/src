#	$NetBSD: Makefile.epoc32.inc,v 1.6 2018/08/12 09:27:02 mrg Exp $

MACHINE_ARCH?=		arm
CPPFLAGS+=		-D${MACHINE}

SYSTEM_FIRST_OBJ=	epoc32_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/epoc32/epoc32_start.S

KERNEL_BASE_VIRT=	$(LOADADDRESS)

KERNLDSCRIPT=		ldscript

EXTRA_CLEAN+=		netbsd.map assym.d ${KERNLDSCRIPT} tmp

# generate ldscript from common template
${KERNLDSCRIPT}: \
    ${THISARM}/conf/ldscript.epoc32 \
    ${THISARM}/conf/Makefile.epoc32.inc \
    Makefile
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.epoc32 > tmp && mv tmp $@
