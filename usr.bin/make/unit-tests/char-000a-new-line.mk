# $NetBSD: char-000a-new-line.mk,v 1.1 2026/08/23 08:23:56 rillig Exp $
#
# Tests for the character U+000A "LINE FEED" or "NEW LINE".

# begin word splitting
#
# When splitting a string into words, the newline character has different
# functions depending on where exactly it occurs:
#	* At the beginning of the string, or after a space, it separates words.
#	* Inside "double" or 'single' quotes, it represents itself.
#	* After an unquoted word character, it stops splitting.

# expect+3: used
# expect+2: +
.for var in used +${.newline} ignored
.  info ${var}
.endfor

.export used +${.newline} ignored
.unexport used +${.newline} ignored
.undef used +${.newline} ignored

# Same for the MAKEFLAGS environment variable.
.MAKEFLAGS: used=yes-from-cmdline +${.newline} ignored=yes-from-cmdline
.if ${used} != "yes-from-cmdline" || ${ignored:Uundefined} != "undefined"
.  error
.endif

# expect+1: used +
.info ${:U used +${.newline} ignored :%=%}
# expect+1: used +
.info ${:U used +${.newline} ignored :@v@$v@}
# expect+1: used +
.info ${:U used +${.newline} ignored :C,x,x,}
# expect+1: used +
.info ${:U a.used a.+${.newline} a.ignored :E}
# expect+1: used +
.info ${:U used/x +/${.newline} ignored/x :H}
# expect+1: used +
.info ${:U used +${.newline} ignored :M*}
# expect+1: used +
.info ${:U used +${.newline} ignored :N}
# expect+1: + used
.info ${:U used +${.newline} ignored :O}
# expect+1: used +
.info ${:U used +${.newline} ignored :R}
# expect+1: used +
.info ${:U used +${.newline} ignored :S,x,x,}
# expect+1: used +
.info ${:U used +${.newline} ignored :T}
# expect+1: 2
.info ${:U used +${.newline} ignored :[#]}
# expect+1: used +
.info ${:U used +${.newline} ignored :[1..-1]}
# expect+1: 1234567 1234567
.info ${:U used +${.newline} ignored :mtime=1234567}
# expect+1: 1 2
.info ${:U used +${.newline} ignored :range}
# expect+1: used +
.info ${:U used +${.newline} ignored :tA}
# expect+1: used +
.info ${:U used +${.newline} ignored :ts }
# expect+1: used +
.info ${:U used +${.newline} ignored :u}

# end word splitting
