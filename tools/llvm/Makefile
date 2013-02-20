#	$NetBSD: Makefile,v 1.16 2013/02/20 14:13:31 joerg Exp $

.include <bsd.init.mk>

#
# Do *not* set HOSTPROG or HOSTLIB at this point otherwise
# it will create a loop trying to extract the object directory.
#
.include "Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"

realdepends realall: config/config.status need-dl

config/config.status: ${LLVM_SRCDIR}/configure
	mkdir -p config
	printf '#!/bin/sh\necho 2.7.3' > config/python
	chmod 755 config/python
	cd config && ${LLVM_SRCDIR}/configure ${LLVM_CONFIGURE_ARGS} \
	    --enable-optimized CC=${HOST_CC:Q} \
	    --with-python=${.OBJDIR}/config/python
# --disable-assertions

need-dl:
	printf '#include <dlfcn.h>\nint main(void){void *p; return dladdr(p, p);}' > need-dl.c
	if ${HOST_CC} -o need-dl.out -D_GNU_SOURCE need-dl.c > /dev/null 2>&1; then \
		echo > ${.TARGET}; \
	elif ${HOST_CC} -o need-dl.out -D_GNU_SOURCE need-dl.c -ldl > /dev/null 2>&1; then \
		echo -ldl > ${.TARGET}; \
	else \
		echo > ${.TARGET}; \
	fi

CLEANFILES+=	need-dl need-dl.c need-dl.out

cleandir: cleandir-llvm

cleandir-llvm:
	test ! -d config || rm -r config
	rm -f need-dl need-dl.tmp

.include <bsd.hostprog.mk>
