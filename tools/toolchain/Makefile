#	$NetBSD: Makefile,v 1.5 2001/10/08 22:19:55 tv Exp $

.include <bsd.own.mk>	# for TOOLDIR

DIST=		${.CURDIR}/../../gnu/dist/toolchain
TIMESTAMP=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc

# Dependencies that trigger a rebuild.
${TIMESTAMP}: \
	${.CURDIR}/Makefile \
	${DIST}/ChangeLog \
	${DIST}/bfd/ChangeLog \
	${DIST}/binutils/ChangeLog \
	${DIST}/gas/ChangeLog \
	${DIST}/gcc/ChangeLog \
	${DIST}/gcc/cp/ChangeLog \
	${DIST}/gcc/f/ChangeLog \
	${DIST}/ld/ChangeLog \
	${DIST}/opcodes/ChangeLog

realall: toolchain.all
realinstall: toolchain.install
clean: toolchain.clean

.for f in aclocal autoconf autoheader automake
toolchain.configure: ${TOOLDIR}/bin/${f}
${TOOLDIR}/bin/${f}: stub.sh
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${BINMODE} $> $@
.endfor

toolchain.configure:
	rm -rf toolchain
	mkdir toolchain
	cd toolchain && \
		CC="${HOST_CC}" \
		CFLAGS="${HOST_CFLAGS}" \
		CPPFLAGS="${HOST_CPPFLAGS}" \
		LDFLAGS="${HOST_LDFLAGS}" \
		PATH="${TOOLDIR}/bin:$$PATH" \
		LANGUAGES="c c++ objc" \
		${DIST}/configure \
		--target=${MACHINE_GNU_PLATFORM} \
		--disable-shared --disable-nls \
		--prefix=${TOOLDIR}
	cd toolchain/gcc && \
		mv -f Makefile Makefile.orig && \
		egrep -v '^(LIBGCC|LIBGCC1_TEST|LIBGCC2|INSTALL_LIBGCC) =' \
			Makefile.orig >Makefile
	@touch $@

toolchain.all: toolchain.configure
	cd toolchain && \
		MACHINE= PATH="${TOOLDIR}/bin:$$PATH" \
		${MAKE} all-binutils all-gas all-ld all-gcc \
		LEX=true BISON=true DESTDIR=

toolchain.install:
	cd toolchain && \
		PATH="${TOOLDIR}/bin:$$PATH" \
		${MAKE} install-binutils install-gas install-ld install-gcc \
		LEX=true BISON=true DESTDIR=
	mv ${TOOLDIR}/bin/cpp ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-cpp

toolchain.clean:
	rm -rf toolchain*

.include <bsd.hostprog.mk>
