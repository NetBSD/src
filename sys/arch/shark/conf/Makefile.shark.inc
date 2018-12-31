#	$NetBSD: Makefile.shark.inc,v 1.27 2018/12/31 13:37:07 christos Exp $

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

.if ${HAVE_BINUTILS} == 227
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout"; \
	${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout
.else
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${ARM_ELF2AOUT} $@ $@.aout"; \
	${ARM_ELF2AOUT} $@ $@.aout
.endif

KERNIMAGES=	netbsd netbsd.aout
