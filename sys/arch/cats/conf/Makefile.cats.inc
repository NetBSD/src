#	$NetBSD: Makefile.cats.inc,v 1.36 2018/12/31 13:30:17 christos Exp $

MACHINE_ARCH?=	arm
CPPFLAGS+=	-D${MACHINE}

.if defined(ABLEELF) && (${ABLEELF} == "1")
# use a standard ELF format, adjusted to align areas
KERNLDSCRIPT=	${THISARM}/conf/ldscript.elf

.else
# Need to convert the kernel from ELF to a.out so that the firmware
# can load it.

CFLAGS+=	-fno-unwind-tables

LOADADDRESS=	0xF0000020

KERNLDSCRIPT=	ldscript
EXTRA_CLEAN+=	${KERNLDSCRIPT}

${KERNLDSCRIPT}: \
    ${ARM}/conf/kern.ldscript.head \
    ${ARM}/conf/kern.ldscript.tail \
    $S/conf/mkldscript.sh \
    ${SYSTEM_OBJ}
	( cat ${ARM}/conf/kern.ldscript.head ; \
	  OBJDUMP=${OBJDUMP} ${HOST_SH} $S/conf/mkldscript.sh \
		${SYSTEM_OBJ} ; \
	  cat ${ARM}/conf/kern.ldscript.tail ) > $@
.if ${HAVE_BINUTILS} == 227
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout"; \
	${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout
.else
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${ARM_ELF2AOUT} $@ $@.aout"; \
	${ARM_ELF2AOUT} $@ $@.aout
.endif
.endif

