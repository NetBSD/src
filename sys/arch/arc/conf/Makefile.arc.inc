#	$NetBSD: Makefile.arc.inc,v 1.3 2001/05/21 03:34:48 soda Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2
ELF2ECOFF?=	${OBJCOPY} --output-target=ecoff-littlemips

# Used if DEBUG != ""
DEBUG_SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb

# XXX
POST_STRIP_SYSTEM_LD_TAIL=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; \
		${ELF2ECOFF} $@ $@.ecoff
