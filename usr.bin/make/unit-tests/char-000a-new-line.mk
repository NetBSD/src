# $NetBSD: char-000a-new-line.mk,v 1.2 2026/08/23 08:48:56 rillig Exp $
#
# Tests for the character U+000A "LINE FEED" or "NEW LINE".

# begin word splitting
#
# When splitting a string into words, the newline character separates words,
# unless it occurs in "double" or 'single' quotes.

# expect+4: used
# expect+3: +
# expect+2: word
.for var in used +${.newline} word
.  info ${var}
.endfor

.export used +${.newline} word
.unexport used +${.newline} word
.undef used +${.newline} word

# Same for the MAKEFLAGS environment variable.
.MAKEFLAGS: used=yes-from-cmdline +${.newline} word=yes-from-cmdline
.if ${used} != "yes-from-cmdline" || ${word:Uundefined} != "yes-from-cmdline"
.  error
.endif

# expect+1: used + word
.info ${:U used +${.newline} word :%=%}
# expect+1: used + word
.info ${:U used +${.newline} word :@v@$v@}
# expect+1: used + word
.info ${:U used +${.newline} word :C,x,x,}
# expect+1: used + word
.info ${:U a.used a.+${.newline} a.word :E}
# expect+1: used + word
.info ${:U used/x +/${.newline} word/x :H}
# expect+1: used + word
.info ${:U used +${.newline} word :M*}
# expect+1: used + word
.info ${:U used +${.newline} word :N}
# expect+1: + used word
.info ${:U used +${.newline} word :O}
# expect+1: used + word
.info ${:U used +${.newline} word :R}
# expect+1: used + word
.info ${:U used +${.newline} word :S,x,x,}
# expect+1: used + word
.info ${:U used +${.newline} word :T}
# expect+1: 3
.info ${:U used +${.newline} word :[#]}
# expect+1: used + word
.info ${:U used +${.newline} word :[1..-1]}
# expect+1: 1234567 1234567 1234567
.info ${:U used +${.newline} word :mtime=1234567}
# expect+1: 1 2 3
.info ${:U used +${.newline} word :range}
# expect+1: used + word
.info ${:U used +${.newline} word :tA}
# expect+1: used + word
.info ${:U used +${.newline} word :ts }
# expect+1: used + word
.info ${:U used +${.newline} word :u}

# end word splitting
