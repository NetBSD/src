#	$NetBSD: Makefile,v 1.1 2003/03/24 15:04:06 atatat Exp $

NOSHARE=	# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/libsm

LIB=		sm

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		assert.c cf.c clock.c clrerr.c config.c debug.c \
		errstring.c exc.c fclose.c feof.c ferror.c fflush.c \
		fget.c findfp.c flags.c fopen.c fpos.c fprintf.c \
		fpurge.c fput.c fread.c fscanf.c fseek.c fvwrite.c \
		fwalk.c fwrite.c get.c heap.c ldap.c makebuf.c match.c \
		mbdb.c mpeix.c niprop.c path.c put.c refill.c rewind.c \
		rpool.c setvbuf.c shm.c signal.c smstdio.c snprintf.c \
		sscanf.c stdio.c strcasecmp.c strdup.c strerror.c \
		strexit.c string.c stringf.c strio.c strl.c strrevcmp.c \
		strto.c syslogio.c test.c ungetc.c vasprintf.c vfprintf.c \
		vfscanf.c vprintf.c vsnprintf.c vsprintf.c vsscanf.c \
		wbuf.c wsetup.c xtrap.c

libinstall::

.include <bsd.lib.mk>
