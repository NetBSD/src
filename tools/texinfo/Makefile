#	$NetBSD: Makefile,v 1.4 2001/10/08 22:43:56 tv Exp $

.include <bsd.own.mk>	# for TOOLDIR

DIST=		${.CURDIR}/../../gnu/dist/texinfo
TIMESTAMP=	${TOOLDIR}/bin/makeinfo

# Dependencies that trigger a rebuild.
${TIMESTAMP}: \
	${.CURDIR}/Makefile \
	${DIST}/ChangeLog

realall: texinfo.all
realinstall: texinfo.install
clean: texinfo.clean

texinfo.configure:
	rm -rf texinfo
	mkdir texinfo
	cd texinfo && \
		CC="${HOST_CC}" \
		CFLAGS="${HOST_CFLAGS}" \
		CPPFLAGS="${HOST_CPPFLAGS}" \
		LDFLAGS="${HOST_LDFLAGS}" \
		PATH="${TOOLDIR}/bin:$$PATH" \
		${DIST}/configure \
		--disable-shared --with-included-gettext \
		--prefix=${TOOLDIR}
	@touch $@

texinfo.all: texinfo.configure
	cd texinfo && \
		PATH="${TOOLDIR}/bin:$$PATH" \
		${MAKE} -j1 all LEX=true BISON=true DESTDIR=

texinfo.install:
.for d in makeinfo util
	cd texinfo/${d} && \
		PATH="${TOOLDIR}/bin:$$PATH" \
		${MAKE} -j1 install LEX=true BISON=true DESTDIR=
.endfor

texinfo.clean:
	rm -rf texinfo*

.include <bsd.hostprog.mk>
