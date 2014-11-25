#	$NetBSD: Makefile.cats.inc,v 1.31 2014/11/25 12:02:20 skrll Exp $

MACHINE_ARCH?=	arm
CPPFLAGS+=	-D${MACHINE}

.if defined(ABLEELF) && (${ABLEELF} == "1")
# use a standard ELF format, adjusted to align areas
LINKSCRIPT=	-T ${THISARM}/conf/ldscript.elf

.else
# Need to convert the kernel from ELF to a.out so that the firmware
# can load it.

CFLAGS+=	-fno-unwind-tables

LINKSCRIPT=	-T ldscript
LOADADDRESS=	0xF0000020

SYSTEM_LD_HEAD_EXTRA+=; \
	( cat ${ARM}/conf/kern.ldscript.head ; \
	  OBJDUMP=${OBJDUMP} ${HOST_SH} $S/conf/mkldscript.sh \
		${SYSTEM_OBJ} ; \
	  cat ${ARM}/conf/kern.ldscript.tail ) > ldscript
KERNLDSCRIPT=	${ARM}/conf/kern.ldscript.head \
		$S/conf/mkldscript.sh \
		${ARM}/conf/kern.ldscript.tail

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout"; \
	${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout
.endif
