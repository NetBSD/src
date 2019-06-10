# $NetBSD: Makefile,v 1.1.46.1 2019/06/10 22:05:05 christos Exp $

SUBDIR+=	lib .WAIT bin

run-configure:
	[ ! -d tmp ] || rm -r tmp
	mkdir -p tmp
	cd tmp && CONFIG_SITE= ${XZSRCDIR:Q}/configure --enable-external-sha256 \
	    CC=${CC:Q} CFLAGS=${CFLAGS:N-Werror:Q} \
	    CPPFLAGS=${CPPFLAGS:N-std=c++11:Q} LDFLAGS=${LDFLAGS:Q} LD=${LD:Q} \
	    --target=${MACHINE_GNU_PLATFORM} --host=${MACHINE_GNU_PLATFORM}
	mv tmp/config.h ${XZINCDIR:Q}
	printf '#include <sys/endian.h>\n#if BYTE_ORDER == BIG_ENDIAN\n#  define WORDS_BIGENDIAN 1\n#endif\n' >> ${XZINCDIR:Q}/config.h

.include "Makefile.inc"

.include <bsd.subdir.mk>
