#	$NetBSD: Makefile.mipsco.inc,v 1.1.2.3 2001/02/11 19:11:07 bouyer Exp $

# Used if DEBUG != ""
DEBUG_SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb

# build ECOFF image that can be booted directly from firmware
LD_ECOFF=	${LD} -N -Ttext ${TEXTADDR} -e start ${GP} \
		-T ${THISMIPS}/conf/kern.ldscript -x -oformat ecoff-bigmips

SYSTEM_LD_TAIL=	@echo ${LD_ECOFF} -o $@.ecoff '$${SYSTEM_OBJ}' vers.o ; \
		${LD_ECOFF} -o $@.ecoff ${SYSTEM_OBJ} vers.o swapnetbsd.o

SYSTEM_LD_TAIL+=; \
		${SIZE} $@ $@.ecoff; chmod 755 $@ $@.ecoff

