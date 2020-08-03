#	$NetBSD: Makefile,v 1.14 2020/08/03 17:23:44 christos Exp $

LIB=isc
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

LIBUVDIR=${NETBSDSRCDIR}/external/mit/libuv
LIBUVOBJDIR!=   cd ${LIBUVDIR}/lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${LIBUVDIR}/dist/include

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isc
.include "${DIST}/api"

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
		hash.c heap.c hex.c hmac.c hp.c httpd.c ht.c \
		iterated_hash.c lex.c lfsr.c lib.c log.c md.c mem.c \
		mutexblock.c netaddr.c netscope.c nonce.c parseint.c \
		pool.c portset.c queue.c quota.c radix.c random.c \
		ratelimiter.c regex.c \
		region.c result.c rwlock.c safe.c serial.c siphash.c \
		sockaddr.c stats.c string.c symtab.c task.c \
		taskpool.c timer.c tm.c utf8.c version.c \
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

.include <bsd.lib.mk>
