#	$NetBSD: Makefile.shark.inc,v 1.8.18.2 2008/01/20 16:04:18 chris Exp $

MACHINE_ARCH=		arm
CPPFLAGS+=		-D${MACHINE}
GENASSYM_EXTRAS+=	${THISARM}/shark/genassym.cf

.if (${OBJECT_FMT} == "ELF")
# Need to convert the kernel from ELF to a.out so that OpenFirmware
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
		$@ $@.aout"; \
	${OBJCOPY} -O a.out-arm-netbsd \
		-R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
		-R .debug_abbrev -R .debug_info -R .debug_line -R .debug_str \
		$@ $@.aout

KERNIMAGES=	netbsd netbsd.aout

.endif
