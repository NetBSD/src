#	$NetBSD: Makefile.arc.inc,v 1.2 2001/03/28 17:04:44 tsutsui Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# Used if DEBUG != ""
DEBUG_SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb

# XXX
POST_STRIP_SYSTEM_LD_TAIL=; \
		echo ${OBJCOPY} --output-target=ecoff-littlemips $@ $@.ecoff; \
		${OBJCOPY} --output-target=ecoff-littlemips $@ $@.ecoff
