#	$NetBSD: Makefile,v 1.2 2001/10/04 17:55:16 tv Exp $

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
		PATH="${TOOLDIR}/bin:$$PATH" DESTDIR= \
		${MAKE} all LEX=true BISON=true

texinfo.install:
.for d in makeinfo util
	cd texinfo/${d} && \
		PATH="${TOOLDIR}/bin:$$PATH" DESTDIR= \
		${MAKE} install LEX=true BISON=true
.endfor

texinfo.clean:
	rm -rf texinfo*

.include <bsd.hostprog.mk>
