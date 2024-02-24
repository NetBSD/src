#	$NetBSD: Makefile,v 1.1.2.2 2024/02/24 13:07:34 martin Exp $

LIBISPRIVATE=yes
LIB=isc

.include <bsd.own.mk>

LIBUVDIR=${NETBSDSRCDIR}/external/mit/libuv
LIBUVOBJDIR!=   cd ${LIBUVDIR}/lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${LIBUVDIR}/dist/include

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isc
CPPFLAGS+=-I${IDIST}/lib/isc

DIST_ISC= ${DIST} ${DIST}/netmgr

.if ${NAMED_USE_PTHREADS} == "yes"
.PATH.c:	${DIST}/unix ${DIST}/nls ${DIST}/pthreads ${DIST_ISC}
.else
.PATH.c:	${DIST}/unix ${DIST}/nls ${DIST}/nothreads ${DIST_ISC}
.endif

UNIX_SRCS=	backtrace.c dir.c entropy.c errno.c errno2result.c \
		file.c fsaccess.c interfaceiter.c net.c os.c resource.c \
		socket.c stdio.c stdtime.c syslog.c time.c \
		backtrace-emptytbl.c meminfo.c

PTHREAD_SRCS=	condition.c mutex.c thread.c

NETMGR_SRCS=    tcp.c udp.c netmgr.c tcpdns.c uv-compat.c uverr2result.c 

CRYPTO_SRCS=	aes.c

SRCS=		app.c assertions.c astack.c base32.c base64.c bind9.c buffer.c \
		bufferlist.c commandline.c crc64.c counter.c error.c event.c \
		hash.c heap.c hex.c hmac.c httpd.c ht.c managers.c \
		iterated_hash.c lex.c lfsr.c lib.c log.c md.c mem.c \
		mutexblock.c netaddr.c netscope.c nonce.c parseint.c \
		pool.c portset.c quota.c radix.c random.c \
		ratelimiter.c regex.c trampoline.c \
		region.c result.c rwlock.c safe.c serial.c siphash.c \
		sockaddr.c stats.c string.c symtab.c task.c \
		taskpool.c timer.c tls.c tm.c utf8.c version.c \
		${UNIX_SRCS} ${PTHREAD_SRCS} ${CRYPTO_SRCS} ${NETMGR_SRCS}


.if (${USE_INET6} == "no")
CPPFLAGS.net.c=	-Wno-error
SRCS+=		ipv6.c
.endif

LDFLAGS+=-Wl,--version-script=${.CURDIR}/isc.map
LDADD+=-Wl,-Bstatic

.if ${MKPICLIB} != "no"
LDADD+=-L${LIBUVOBJDIR} -luv_pic
DPADD+=${LIBUVOBJDIR}/libuv_pic.a
.else
LDADD+=-L${LIBUVOBJDIR} -luv
DPADD+=${LIBUVOBJDIR}/libuv.a
.endif

LDADD+=-Wl,-Bdynamic

LDADD+=-lz
DPADD+=${LIBZ}

COPTS.iterated_hash.c+= -Wno-error=deprecated-declarations
COPTS.hmac.c+= -Wno-error=deprecated-declarations
COPTS.md.c+= -Wno-error=deprecated-declarations

.include <bsd.lib.mk>
