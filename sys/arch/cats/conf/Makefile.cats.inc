#	$NetBSD: Makefile.cats.inc,v 1.20.10.2 2009/09/16 13:37:37 yamt Exp $

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
	 "${OBJCOPY} -O a.out-arm-netbsd \
		-R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
		-R .debug_abbrev -R .debug_info -R .debug_line -R .debug_str \
		-R .debug_frame -R .debug_loc -R .debug_pubnames \
		-R .debug_aranges -R .ARM.attributes \
		$@ $@.aout"; \
	${OBJCOPY} -O a.out-arm-netbsd \
		-R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
		-R .debug_abbrev -R .debug_info -R .debug_line -R .debug_str \
		-R .debug_frame -R .debug_loc -R .debug_pubnames \
		-R .debug_aranges -R .ARM.attributes \
		$@ $@.aout
.endif
.endif
