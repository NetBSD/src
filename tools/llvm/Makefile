#	$NetBSD: Makefile,v 1.7 2011/07/05 17:30:35 joerg Exp $

.include <bsd.init.mk>

#
# Do *not* set HOSTPROG or HOSTLIB at this point otherwise
# it will create a loop trying to extract the object directory.
#
.include "Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"

realdepends realall: config/config.status need-dl \
	config/include/clang/Basic/Version.inc

config/include/clang/Basic/Version.inc: \
	${LLVM_TOPLEVEL}/config/clang/Basic/Version.inc
	mkdir -p ${.TARGET:H}
	cat < ${.ALLSRC} > ${.TARGET}

config/config.status: ${LLVM_SRCDIR}/configure
	mkdir -p config
	cd config && ${LLVM_SRCDIR}/configure --prefix=/usr \
	    --enable-targets=x86,powerpc,sparc,arm --host=${MACHINE_GNU_PLATFORM} \
	    --with-c-include-dirs=/usr/include/clang-3.0:/usr/include \
	    --enable-optimized --disable--assertions

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
	rm -r config
	rm -f need-dl need-dl.tmp

.include <bsd.hostprog.mk>
