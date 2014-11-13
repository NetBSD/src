#	$NetBSD: Makefile.cats.inc,v 1.28 2014/11/13 15:03:06 uebayasi Exp $

MACHINE_ARCH?=	arm
CPPFLAGS+=	-D${MACHINE}

.if defined(ABLEELF) && (${ABLEELF} == "1")
# use a standard ELF format, adjusted to align areas
LINKFLAGS=	-T ${THISARM}/conf/ldscript.elf

.else
# Need to convert the kernel from ELF to a.out so that the firmware
# can load it.

CFLAGS+=	-fno-unwind-tables

LINKFLAGS=	-T ldscript

SYSTEM_LD_HEAD_EXTRA+=; \
	( cat ${ARM}/conf/kern.ldscript.head ; \
	  OBJDUMP=${OBJDUMP} ${HOST_SH} $S/conf/mkldscript.sh \
		${SYSTEM_OBJ} ; \
	  cat ${ARM}/conf/kern.ldscript.tail ) > ldscript
SYSTEM_DEP+=	${ARM}/conf/kern.ldscript.head \
		$S/conf/mkldscript.sh \
		${ARM}/conf/kern.ldscript.tail

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout"; \
	${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout
.endif
