#	$NetBSD: Makefile.cats.inc,v 1.1.2.8 2003/01/03 16:41:14 thorpej Exp $

CPPFLAGS+=	-D${MACHINE}

.if (${OBJECT_FMT} == "ELF")
# Need to convert the kernel from ELF to a.out so that the firmware
# can load it.

LINKFLAGS=	-T ${THISARM}/conf/kern.ldscript

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo \
	 "${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs $@ $@.aout"; \
	${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs $@ $@.aout
.endif
