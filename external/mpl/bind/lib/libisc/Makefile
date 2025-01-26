#	$NetBSD: Makefile,v 1.24 2025/01/26 16:25:53 christos Exp $

LIB=isc
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

LIBUVDIR=${NETBSDSRCDIR}/external/mit/libuv
LIBUVOBJDIR!=   cd ${LIBUVDIR}/lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${LIBUVDIR}/dist/include

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isc
CPPFLAGS+=-I${IDIST}/lib/isc
CPPFLAGS+=-I${BIND_SRCDIR}/include/isc

DIST_ISC= ${DIST} ${DIST}/netmgr

.PATH.c:	${DIST}/unix ${DIST}/nls ${DIST}/pthreads ${DIST_ISC}

UNIX_SRCS=	backtrace.c dir.c entropy.c errno.c errno2result.c \
		file.c interfaceiter.c net.c os.c \
		stdio.c stdtime.c syslog.c time.c \
		meminfo.c

PTHREAD_SRCS=	condition.c mutex.c thread.c

NETMGR_SRCS=    netmgr.c proxystream.c proxyudp.c socket.c streamdns.c \
		tcp.c netmgr-timer.c tlsstream.c udp.c

CRYPTO_SRCS=

SRCS=		ascii.c assertions.c async.c base32.c base64.c \
		commandline.c crc64.c counter.c error.c fips.c \
		getaddresses.c hash.c hashmap.c heap.c helper.c hex.c histo.c \
		hmac.c httpd.c ht.c managers.c iterated_hash.c job.c \
		lex.c lib.c log.c loop.c md.c mem.c \
		mutexblock.c netaddr.c netscope.c nonce.c parseint.c \
		picohttpparser.c portset.c proxy2.c quota.c radix.c random.c \
		ratelimiter.c regex.c \
		region.c result.c rwlock.c safe.c serial.c signal.c \
		sockaddr.c stats.c string.c symtab.c tid.c \
		timer.c tls.c tm.c url.c utf8.c uv.c work.c xml.c \
		${UNIX_SRCS} ${PTHREAD_SRCS} ${CRYPTO_SRCS} ${NETMGR_SRCS}


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

CLEANTFILES+=netmgr-timer.c

CPPFLAGS.netmgr-timer.c+=-I${DIST}/netmgr
netmgr-timer.c: ${DIST}/netmgr/timer.c
	${MKTARGET_CREATE}
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.lib.mk>
