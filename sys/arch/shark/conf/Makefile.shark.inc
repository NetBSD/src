#	$NetBSD: Makefile.shark.inc,v 1.3 2003/01/03 02:34:49 thorpej Exp $

CPPFLAGS+=		-D${MACHINE}
GENASSYM_EXTRAS+=	${THISARM}/shark/genassym.cf

.if (${OBJECT_FMT} == "ELF")
# Need to convert the kernel from ELF to a.out so that OpenFirmware
# can load it.

LINKFLAGS=	-T ${THISARM}/conf/kern.ldscript

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo \
	 "${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs $@ $@.aout"; \
	${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs $@ $@.aout
.endif
