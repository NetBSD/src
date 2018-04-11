#	$NetBSD: Makefile,v 1.19.6.1 2018/04/11 02:58:47 msaitoh Exp $

LIBISPRIVATE=yes

LIB=ntp

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/libntp 

.PATH.c:	${DIST}

SRCS= \
a_md5encrypt.c \
adjtime.c \
atoint.c \
atolfp.c \
atouint.c \
audio.c \
authkeys.c \
authreadkeys.c \
authusekey.c \
bsd_strerror.c \
buftvtots.c \
caljulian.c \
caltontp.c \
calyearstart.c \
clocktime.c \
clocktypes.c \
decodenetnum.c \
dofptoa.c \
dolfptoa.c \
emalloc.c \
findconfig.c \
getopt.c \
hextoint.c \
hextolfp.c \
humandate.c \
icom.c \
iosignal.c \
is_ip_address.c \
lib_strbuf.c \
libssl_compat.c \
machines.c \
mktime.c \
modetoa.c \
mstolfp.c \
msyslog.c \
netof.c \
ntp_calendar.c \
ntp_crypto_rnd.c \
ntp_intres.c \
ntp_libopts.c \
ntp_lineedit.c \
ntp_random.c \
ntp_rfc2553.c \
ntp_worker.c \
numtoa.c \
numtohost.c \
octtoint.c \
prettydate.c \
recvbuff.c \
refnumtoa.c \
snprintf.c \
socket.c \
socktoa.c \
socktohost.c \
ssl_init.c \
statestr.c \
strdup.c \
syssignal.c \
systime.c \
timetoa.c \
uglydate.c \
vint64ops.c \
work_fork.c \
work_thread.c \
ymd2yd.c

CPPFLAGS+= -I${IDIST}/sntp/libopts

COPTS.msyslog.c+=	-Wno-error=format-nonliteral

# For MKREPRO, avoid using __DATE__ and __TIME__.
# Instead, use the date and time from ${IMPORTDATE_FILE}.
#
# The file should contain one line, like this:
#	Fri Dec 27 19:28:17 EST 2013 (import)
#
.if ${MKREPRO:Uno} == "yes"
IMPORTDATE_FILE := ${.PARSEDIR}/../../importdate
MKREPRO_DATE != ${TOOL_AWK} '{printf "%3s %2d %4d", $$2, $$3, $$6}' \
	<${IMPORTDATE_FILE} # "Mmm DD YYYY"
MKREPRO_TIME != ${TOOL_AWK} '{print $$4}' \
	<${IMPORTDATE_FILE} # "HH:MM:SS"
CPPFLAGS.ntp_calendar.c += -DMKREPRO_DATE=\"${MKREPRO_DATE:Q}\"
CPPFLAGS.ntp_calendar.c += -DMKREPRO_TIME=\"${MKREPRO_TIME:Q}\"
.endif

.include <bsd.lib.mk>
