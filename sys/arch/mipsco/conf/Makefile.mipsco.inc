#	$NetBSD: Makefile.mipsco.inc,v 1.1 2000/12/03 04:51:26 matt Exp $

# Used if DEBUG != ""
DEBUG_SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb

# XXX
POST_STRIP_SYSTEM_LD_TAIL=; \
		echo objcopy --output-target=ecoff-bigmips $@ $@.ecoff; \
		objcopy --output-target=ecoff-bigmips $@ $@.ecoff
