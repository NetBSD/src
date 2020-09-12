# $NetBSD: varname-dot-parsefile.mk,v 1.3 2020/09/12 11:55:28 rillig Exp $
#
# Tests for the special .PARSEFILE variable, which contains the basename part
# of the file that is currently parsed.

.if ${.PARSEFILE} != "varname-dot-parsefile.mk"
.  error
.endif

# During parsing, it is possible to undefine .PARSEFILE.
# Not that anyone would ever want to do this, but there's code in parse.c,
# function PrintLocation, that explicitly handles this situation.
.if !defined(.PARSEFILE)
.  error
.endif
.undef .PARSEFILE
.if defined(.PARSEFILE)
.  error
.endif

# The variable .PARSEFILE is indirectly used by the .info directive,
# via PrintLocation.
.info At this point, .PARSEFILE is undefined.

# There is absolutely no point in faking the location of the file that is
# being parsed.  Technically, it's possible though.
.PARSEFILE = fake-parsefile
.info The location can be faked.

# After including another file, .PARSEFILE is reset.
.include "/dev/null"
.info The location is no longer fake.

all:
	@echo At run time, .PARSEFILE is ${.PARSEFILE:Uundefined}.
