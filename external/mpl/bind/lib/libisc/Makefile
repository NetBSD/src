#	$NetBSD: Makefile,v 1.6 2019/02/24 20:01:33 christos Exp $

LIB=isc
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isc
.include "${DIST}/api"

.if ${NAMED_USE_PTHREADS} == "yes"
.PATH.c:	${DIST}/unix ${DIST}/nls ${DIST}/pthreads ${DIST}
.else
.PATH.c:	${DIST}/unix ${DIST}/nls ${DIST}/nothreads ${DIST}
.endif

UNIX_SRCS=	app.c backtrace.c dir.c entropy.c errno2result.c file.c \
		fsaccess.c interfaceiter.c net.c os.c resource.c \
		socket.c stdio.c stdtime.c syslog.c time.c \
		backtrace-emptytbl.c meminfo.c
PTHREAD_SRCS=	condition.c mutex.c thread.c

CRYPTO_SRCS=	aes.c

SRCS=		assertions.c base32.c base64.c bind9.c buffer.c \
		bufferlist.c commandline.c crc64.c counter.c error.c event.c \
		hash.c heap.c hex.c hmac.c httpd.c ht.c \
		iterated_hash.c lex.c lfsr.c lib.c log.c md.c mem.c \
		mutexblock.c netaddr.c netscope.c nonce.c parseint.c \
		pool.c portset.c quota.c radix.c random.c ratelimiter.c \
		regex.c \
		region.c result.c rwlock.c serial.c \
		sockaddr.c stats.c string.c symtab.c task.c \
		taskpool.c timer.c tm.c version.c \
		${UNIX_SRCS} ${PTHREAD_SRCS} ${CRYPTO_SRCS}



.if (${USE_INET6} == "no")
CPPFLAGS.net.c=	-Wno-error
SRCS+=		ipv6.c
.endif

# On certain flavors of arm we don't have native atomics only ones
# libcall ones which are inefficient. But this should not be an
# error anyway.
COPTS.mem.c+= ${${ACTIVE_CC} == "clang":? -Wno-error-atomic-alignment :}



LDADD+=-lz
DPADD+=${LIBZ}

.include <bsd.lib.mk>
