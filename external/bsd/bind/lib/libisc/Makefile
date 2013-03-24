#	$NetBSD: Makefile,v 1.3 2013/03/24 18:55:55 christos Exp $

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
		fsaccess.c interfaceiter.c keyboard.c net.c os.c resource.c \
		socket.c stdio.c stdtime.c strerror.c syslog.c time.c \
		backtrace-emptytbl.c
NLS_SRCS=	msgcat.c
PTHREAD_SRCS=	condition.c mutex.c thread.c
API_SRCS=	app_api.c mem_api.c socket_api.c task_api.c timer_api.c 

SRCS=		assertions.c base32.c base64.c bitstring.c buffer.c \
		bufferlist.c commandline.c error.c event.c \
		hash.c heap.c hex.c hmacmd5.c hmacsha.c httpd.c inet_aton.c \
		iterated_hash.c lex.c lfsr.c lib.c log.c md5.c mem.c \
		mutexblock.c netaddr.c netscope.c ondestroy.c parseint.c \
		portset.c quota.c radix.c random.c ratelimiter.c refcount.c \
		region.c result.c rwlock.c serial.c sha1.c sha2.c sockaddr.c \
		stats.c string.c strtoul.c symtab.c task.c taskpool.c timer.c \
		version.c ${UNIX_SRCS} ${NLS_SRCS} ${PTHREAD_SRCS} ${API_SRCS}


.if (${USE_INET6} == "no")
CPPFLAGS.net.c=	-Wno-error
SRCS+=		ipv6.c
.endif

.include <bsd.lib.mk>
