#	$NetBSD: Makefile.cats.inc,v 1.14 2003/09/21 15:21:03 matt Exp $

CPPFLAGS+=	-D${MACHINE}

.if (${OBJECT_FMT} == "ELF")
# Need to convert the kernel from ELF to a.out so that the firmware
# can load it.

LINKFLAGS=	-T ${THISARM}/conf/kern.ldscript

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo \
	 "${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs -R .comment $@ $@.aout"; \
	${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs -R .comment $@ $@.aout
.endif
