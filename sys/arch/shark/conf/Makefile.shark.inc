#	$NetBSD: Makefile.shark.inc,v 1.1 2002/02/10 01:57:02 thorpej Exp $

GENASSYM_EXTRAS+=	${THISARM}/shark/genassym.cf

.if (${OBJECT_FMT} == "ELF")
# Need to convert the kernel from ELF to a.out so that OpenFirmware
# can load it.

# ZMAGIC a.out includes the a.out header in front of the text
# segment, so if we link the kernel at the normal load address,
# it will be wrong once the a.out header is stuck in the file.
# XXX HOWEVER, by linking it +32 bytes to compensate, the resulting
# XXX ELF kernel cannot be booted itself.
LOADADDRESS=0xF0000020

SYSTEM_LD_TAIL_EXTRA+=; \
	${DBSYM} $@ || true; \
	echo OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
	    ${ARM}/conf/elf2aout.sh $@ $@.aout; \
	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
	    ${ARM}/conf/elf2aout.sh $@ $@.aout
.endif
