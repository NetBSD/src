#	$NetBSD: Makefile.arc.inc,v 1.4 2001/11/14 23:02:02 thorpej Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# Used if DEBUG != ""
DEBUG_SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb

# XXX
POST_STRIP_SYSTEM_LD_TAIL=; \
		echo ${OBJCOPY} -O ecoff-littlemips $@ $@.ecoff; \
		${OBJCOPY} -O ecoff-littlemips $@ $@.ecoff
