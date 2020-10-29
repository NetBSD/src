# $NetBSD: varmod-gmtime.mk,v 1.3 2020/10/29 18:59:43 rillig Exp $
#
# Tests for the :gmtime variable modifier, which formats a timestamp
# using strftime(3).

all:	mod-gmtime
all:	mod-gmtime-indirect

# Test for the default time format, %c.  Since the time always varies, it's
# only possible to check for the general format here.  The names of the
# month and weekday are always in English, independent from the locale.
# Example: Thu Oct 29 18:56:41 2020
.if ${:U:gmtime:tW:M??? ??? ?? ??\:??\:?? ????} == ""
.  error
.endif

mod-gmtime:
	@echo $@:
	@echo ${%Y:L:gmtim=1593536400}		# modifier name too short
	@echo ${%Y:L:gmtime=1593536400}		# 2020-07-01T00:00:00Z
	@echo ${%Y:L:gmtimer=1593536400}	# modifier name too long
	@echo ${%Y:L:gm=gm:M*}

mod-gmtime-indirect:
	@echo $@:

	# As of 2020-08-16, it is not possible to pass the seconds via a
	# variable expression.  This is because parsing of the :gmtime
	# modifier stops at the '$' and returns to ApplyModifiers.
	#
	# There, a colon would be skipped but not a dollar.
	# Parsing therefore continues at the '$' of the ${:U159...}, looking
	# for an ordinary variable modifier.
	#
	# At this point, the ${:U} is expanded and interpreted as a variable
	# modifier, which results in the error message "Unknown modifier '1'".
	#
	# If ApplyModifier_Gmtime were to pass its argument through
	# ParseModifierPart, this would work.
	@echo ${%Y:L:gmtime=${:U1593536400}}

all:
	@:;
