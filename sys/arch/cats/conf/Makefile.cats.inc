#	$NetBSD: Makefile.cats.inc,v 1.17.26.1 2007/03/12 05:47:32 rmind Exp $

MACHINE_ARCH=	arm
CPPFLAGS+=	-D${MACHINE}

.if (${OBJECT_FMT} == "ELF")

.if defined(ABLEELF) && (${ABLEELF} == "1")
# use a standard ELF format, adjusted to align areas
LINKFLAGS=	-T ${THISARM}/conf/ldscript.elf

.else
# Need to convert the kernel from ELF to a.out so that the firmware
# can load it.

LINKFLAGS=	-T ldscript

SYSTEM_LD_HEAD_EXTRA+=; \
	( cat ${ARM}/conf/kern.ldscript.head ; \
	  OBJDUMP=${OBJDUMP} ${HOST_SH} $S/conf/mkldscript.sh \
		${SYSTEM_OBJ} ; \
	  cat ${ARM}/conf/kern.ldscript.tail ) > ldscript

SYSTEM_LD_TAIL_EXTRA+=; \
	echo \
	 "${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs -R .comment $@ $@.aout"; \
	${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs -R .comment $@ $@.aout
.endif
.endif
