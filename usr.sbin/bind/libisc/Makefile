#	$NetBSD: Makefile,v 1.1.1.1 2004/05/18 00:06:58 christos Exp $

LIB=isc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isc
.include "${DIST}/api"

.if ${NAMED_USE_PTHREADS} == "yes"
.PATH.c:	${DIST}/unix ${DIST}/nls ${DIST}/pthreads ${DIST}
.else
.PATH.c:	${DIST}/unix ${DIST}/nls ${DIST}/nothreads ${DIST}
.endif

UNIX_SRCS=	app.c dir.c entropy.c errno2result.c file.c fsaccess.c \
		interfaceiter.c keyboard.c net.c os.c resource.c socket.c \
		stdio.c stdtime.c strerror.c syslog.c time.c
NLS_SRCS=	msgcat.c
PTHREAD_SRCS=	condition.c mutex.c thread.c

SRCS=		assertions.c base64.c bitstring.c buffer.c \
		bufferlist.c commandline.c error.c event.c \
		hash.c heap.c hex.c hmacmd5.c lex.c lfsr.c \
		lib.c log.c md5.c mem.c mutexblock.c netaddr.c \
		netscope.c ondestroy.c parseint.c quota.c random.c \
		ratelimiter.c region.c result.c rwlock.c \
		serial.c sha1.c sockaddr.c string.c strtoul.c symtab.c \
		task.c taskpool.c timer.c version.c ${UNIX_SRCS} \
		${NLS_SRCS} ${PTHREAD_SRCS}

.include <bsd.lib.mk>
