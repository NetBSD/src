#	$NetBSD: Makefile.arc.inc,v 1.1.2.2 2000/12/08 09:26:21 bouyer Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# Used if DEBUG != ""
DEBUG_SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb

# XXX
POST_STRIP_SYSTEM_LD_TAIL=; \
		echo objcopy --output-target=ecoff-littlemips $@ $@.ecoff; \
		objcopy --output-target=ecoff-littlemips $@ $@.ecoff
