#	$NetBSD: Makefile.mipsco.inc,v 1.1.2.2 2000/12/08 09:28:23 bouyer Exp $

# Used if DEBUG != ""
DEBUG_SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb

# XXX
POST_STRIP_SYSTEM_LD_TAIL=; \
		echo objcopy --output-target=ecoff-bigmips $@ $@.ecoff; \
		objcopy --output-target=ecoff-bigmips $@ $@.ecoff
