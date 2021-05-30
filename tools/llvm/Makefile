#	$NetBSD: Makefile,v 1.24 2021/05/30 01:57:00 joerg Exp $

.include <bsd.hostinit.mk>

#
# Do *not* set HOSTPROG or HOSTLIB at this point otherwise
# it will create a loop trying to extract the object directory.
#
.include "Makefile.inc"
.include "${LLVM_TOPLEVEL}/lib/Makefile.inc"

realdepends realall: config/config.status need-dl need-terminfo support-modules

dependall: clean-modulecache

clean-modulecache:
	rm -rf ${LLVM_MODULE_CACHE}

config/config.status: ${LLVM_SRCDIR}/configure
	mkdir -p config
	printf '#!/bin/sh\necho 2.7.3' > config/python
	chmod 755 config/python
	cd config && ${HOST_SH} ${CONFIG_DIR}/configure ${LLVM_CONFIGURE_ARGS} \
	    --enable-optimized CC=${HOST_CC:Q} CXX=${HOST_CXX:Q} \
	    --with-python=${.OBJDIR}/config/python && \
	cp ${LLVM_TOPLEVEL}/include/module.modulemap include/llvm/module.modulemap

support-modules: module-test.cpp Makefile
	if ${HOST_CXX} -stdlib=libc++ -c -fmodules -fcxx-modules -fmodules-cache-path=./module.cache \
	   ${.CURDIR}/module-test.cpp  3> /dev/null 2>&1; then \
		echo HOST_SUPPORTS_MODULES=yes > ${.TARGET}; \
	else \
		echo HOST_SUPPORTS_MODULES=no > ${.TARGET}; \
	fi

need-dl: Makefile
	printf '#include <dlfcn.h>\nint main(void){void *p; return dladdr(p, p);}' > need-dl.c
	if ${HOST_CC} -o need-dl.out -D_GNU_SOURCE need-dl.c > /dev/null 2>&1; then \
		echo > ${.TARGET}; \
	elif ${HOST_CC} -o need-dl.out -D_GNU_SOURCE need-dl.c -ldl > /dev/null 2>&1; then \
		echo -ldl > ${.TARGET}; \
	else \
		echo > ${.TARGET}; \
	fi

need-terminfo: Makefile
	printf 'int setupterm(char *, int, int *);\nint main(void){return setupterm("", 0, 0);}' > need-terminfo.c
	for lib in tinfo terminfo ncurses curses; do \
		if ${HOST_CC} -o need-terminfo.out need-terminfo.c -l$$lib > /dev/null 2>&1; then \
			echo -l$$lib > ${.TARGET}; \
			break; \
		fi; \
	done

CLEANFILES+=	need-dl need-dl.c need-dl.out need-terminfo need-terminfo.c need-terminfo.out

cleandir: cleandir-llvm

cleandir-llvm: .PHONY
	test ! -d config || rm -r config
	test ! -d module-cache || rm -r module-cache
	rm -f need-dl need-dl.tmp module-cache module-test.o

.include <bsd.hostprog.mk>
