#	$NetBSD: Makefile.shark.inc,v 1.27.30.1 2023/09/11 13:35:37 martin Exp $

CPPFLAGS+=		-D${MACHINE}
CFLAGS+=		-fno-unwind-tables
GENASSYM_EXTRAS+=	${THISARM}/shark/genassym.cf

# Need to convert the kernel from ELF to a.out so that OpenFirmware
# can load it.

KERNLDSCRIPT=	ldscript
EXTRA_CLEAN+=	${KERNLDSCRIPT}

LOADADDRESS=	0xF0000020

ldscript: \
    ${ARM}/conf/kern.ldscript.head \
    ${ARM}/conf/kern.ldscript.tail \
    $S/conf/mkldscript.sh \
    ${SYSTEM_OBJ}
	( cat ${ARM}/conf/kern.ldscript.head ; \
	  OBJDUMP=${OBJDUMP} ${HOST_SH} $S/conf/mkldscript.sh \
		${SYSTEM_OBJ} ; \
	  cat ${ARM}/conf/kern.ldscript.tail ) > ldscript

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${ARM_ELF2AOUT} $@ $@.aout"; \
	${ARM_ELF2AOUT} $@ $@.aout

KERNIMAGES=	netbsd netbsd.aout
