# $NetBSD: varmod-localtime.mk,v 1.4 2020/10/29 19:01:10 rillig Exp $
#
# Tests for the :localtime variable modifier, which returns the given time,
# formatted as a local timestamp.

# Test for the default time format, %c.  Since the time always varies, it's
# only possible to check for the general format here.  The names of the
# month and weekday are always in English, independent from the locale.
# Example: Thu Oct 29 18:56:41 2020
.if ${:U:localtime:tW:M??? ??? ?? ??\:??\:?? ????} == ""
.  error
.endif

all:
	@echo ${%Y:L:localtim=1593536400}	# modifier name too short
	@echo ${%Y:L:localtime=1593536400}	# 2020-07-01T00:00:00Z
	@echo ${%Y:L:localtimer=1593536400}	# modifier name too long
