#	$NetBSD: Makefile,v 1.3 2019/09/23 13:42:33 christos Exp $

TARGET_PLATFORM = soft68k   # Set the default platform to "soft"

.if (${TARGET_PLATFORM} == "soft68k")
CDBASE = soft68kcd          # CD base name for Soft platform
.elif (${TARGET_PLATFORM} == "luna68k")
CDBASE = luna68kcd       # CD base name for Luna platform
.endif

CDRELEASE = true         # include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG = true

.include "${.CURDIR}/../../../common/Makefile.bootcd"
