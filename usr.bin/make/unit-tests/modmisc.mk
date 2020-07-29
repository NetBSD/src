# $Id: modmisc.mk,v 1.28 2020/07/29 18:48:47 rillig Exp $
#
# miscellaneous modifier tests

# do not put any dirs in this list which exist on some
# but not all target systems - an exists() check is below.
path=:/bin:/tmp::/:.:/no/such/dir:.
# strip cwd from path.
MOD_NODOT=S/:/ /g:N.:ts:
# and decorate, note that $'s need to be doubled. Also note that 
# the modifier_variable can be used with other modifiers.
MOD_NODOTX=S/:/ /g:N.:@d@'$$d'@
# another mod - pretend it is more interesting
MOD_HOMES=S,/home/,/homes/,
MOD_OPT=@d@$${exists($$d):?$$d:$${d:S,/usr,/opt,}}@
MOD_SEP=S,:, ,g

all:	modvar modvarloop modsysv mod-HTE emptyvar undefvar
all:	mod-subst
all:	mod-regex
all:	mod-loop-varname mod-loop-resolve mod-loop-varname-dollar
all:	mod-subst-dollar mod-loop-dollar
all:	mod-regex-limits
all:	mod-regex-errors
all:	mod-assign
all:	mod-assign-nested
all:	mod-tu-space
all:	mod-quote
all:	mod-break-many-words
all:	mod-remember
all:	mod-gmtime
all:	mod-localtime
all:	mod-hash
all:	mod-range

# See also sysv.mk.
modsysv:
	@echo "The answer is ${libfoo.a:L:libfoo.a=42}"

# Demonstrates modifiers that are given indirectly from a variable.
modvar:
	@echo "path='${path}'"
	@echo "path='${path:${MOD_NODOT}}'"
	@echo "path='${path:S,home,homes,:${MOD_NODOT}}'"
	@echo "path=${path:${MOD_NODOTX}:ts:}"
	@echo "path=${path:${MOD_HOMES}:${MOD_NODOTX}:ts:}"

.for d in ${path:${MOD_SEP}:N.} /usr/xbin
path_$d?= ${d:${MOD_OPT}:${MOD_HOMES}}/
paths+= ${d:${MOD_OPT}:${MOD_HOMES}}
.endfor

modvarloop:
	@echo "path_/usr/xbin=${path_/usr/xbin}"
	@echo "paths=${paths}"
	@echo "PATHS=${paths:tu}"

PATHNAMES=	a/b/c def a.b.c a.b/c a a.a .gitignore a a.a
mod-HTE:
	@echo "dirname of '"${PATHNAMES:Q}"' is '"${PATHNAMES:H:Q}"'"
	@echo "basename of '"${PATHNAMES:Q}"' is '"${PATHNAMES:T:Q}"'"
	@echo "suffix of '"${PATHNAMES:Q}"' is '"${PATHNAMES:E:Q}"'"
	@echo "root of '"${PATHNAMES:Q}"' is '"${PATHNAMES:R:Q}"'"

# When a modifier is applied to the "" variable, the result is discarded.
emptyvar:
	@echo S:${:S,^$,empty,}
	@echo C:${:C,^$,empty,}
	@echo @:${:@var@${var}@}

# The :U modifier turns even the "" variable into something that has a value.
# The resulting variable is empty, but is still considered to contain a
# single empty word. This word can be accessed by the :S and :C modifiers,
# but not by the :@ modifier since it explicitly skips empty words.
undefvar:
	@echo S:${:U:S,^$,empty,}
	@echo C:${:U:C,^$,empty,}
	@echo @:${:U:@var@empty@}

WORDS=		sequences of letters
.if ${WORDS:S,,,} != ${WORDS}
.warning The empty pattern matches something.
.endif
.if ${WORDS:S,e,*,1} != "s*quences of letters"
.warning The :S modifier flag '1' is not applied exactly once.
.endif
.if ${WORDS:S,e,*,} != "s*quences of l*tters"
.warning The :S modifier does not replace every first match per word.
.endif
.if ${WORDS:S,e,*,g} != "s*qu*nc*s of l*tt*rs"
.warning The :S modifier flag 'g' does not replace every occurrence.
.endif
.if ${WORDS:S,^sequ,occurr,} != "occurrences of letters"
.warning The :S modifier fails for a short match anchored at the start.
.endif
.if ${WORDS:S,^of,with,} != "sequences with letters"
.warning The :S modifier fails for an exact match anchored at the start.
.endif
.if ${WORDS:S,^office,does not match,} != ${WORDS}
.warning The :S modifier matches a too long pattern anchored at the start.
.endif
.if ${WORDS:S,f$,r,} != "sequences or letters"
.warning The :S modifier fails for a short match anchored at the end.
.endif
.if ${WORDS:S,s$,,} != "sequence of letter"
.warning The :S modifier fails to replace one occurrence per word.
.endif
.if ${WORDS:S,of$,,} != "sequences letters"
.warning The :S modifier fails for an exact match anchored at the end.
.endif
.if ${WORDS:S,eof$,,} != ${WORDS}
.warning The :S modifier matches a too long pattern anchored at the end.
.endif
.if ${WORDS:S,^of$,,} != "sequences letters"
.warning The :S modifier does not match a word anchored at both ends.
.endif
.if ${WORDS:S,^o$,,} != ${WORDS}
.warning The :S modifier matches a prefix anchored at both ends.
.endif
.if ${WORDS:S,^f$,,} != ${WORDS}
.warning The :S modifier matches a suffix anchored at both ends.
.endif
.if ${WORDS:S,^eof$,,} != ${WORDS}
.warning The :S modifier matches a too long prefix anchored at both ends.
.endif
.if ${WORDS:S,^office$,,} != ${WORDS}
.warning The :S modifier matches a too long suffix anchored at both ends.
.endif

mod-subst:
	@echo $@:
	@echo :${:Ua b b c:S,a b,,:Q}:
	@echo :${:Ua b b c:S,a b,,1:Q}:
	@echo :${:Ua b b c:S,a b,,W:Q}:
	@echo :${:Ua b b c:S,b,,g:Q}:
	@echo :${:U1 2 3 1 2 3:S,1 2,___,Wg:S,_,x,:Q}:
	@echo ${:U12345:S,,sep,g:Q}

mod-regex:
	@echo $@:
	@echo :${:Ua b b c:C,a b,,:Q}:
	@echo :${:Ua b b c:C,a b,,1:Q}:
	@echo :${:Ua b b c:C,a b,,W:Q}:
	@echo :${:Uword1 word2:C,****,____,g:C,word,____,:Q}:
	@echo :${:Ua b b c:C,b,,g:Q}:
	@echo :${:U1 2 3 1 2 3:C,1 2,___,Wg:C,_,x,:Q}:

# In the :@ modifier, the name of the loop variable can even be generated
# dynamically.  There's no practical use-case for this, and hopefully nobody
# will ever depend on this, but technically it's possible.
mod-loop-varname:
	@echo :${:Uone two three:@${:Ubar:S,b,v,}@+${var}+@:Q}:

# The :@ modifier resolves the variables a little more often than expected.
# In particular, it resolves _all_ variables from the context, and not only
# the loop variable (in this case v).
#
# The d means direct reference, the i means indirect reference.
RESOLVE=	${RES1} $${RES1}
RES1=		1d${RES2} 1i$${RES2}
RES2=		2d${RES3} 2i$${RES3}
RES3=		3

mod-loop-resolve:
	@echo $@:${RESOLVE:@v@w${v}w@:Q}:

# Until 2020-07-20, the variable name of the :@ modifier could end with one
# or two dollar signs, which were silently ignored.
# There's no point in allowing a dollar sign in that position.
mod-loop-varname-dollar:
	@echo $@:${1 2 3:L:@v$@($v)@:Q}.
	@echo $@:${1 2 3:L:@v$$@($v)@:Q}.
	@echo $@:${1 2 3:L:@v$$$@($v)@:Q}.

# No matter how many dollar characters there are, they all get merged
# into a single dollar by the :S modifier.
mod-subst-dollar:
	@echo $@:${:U1:S,^,$,:Q}:
	@echo $@:${:U2:S,^,$$,:Q}:
	@echo $@:${:U3:S,^,$$$,:Q}:
	@echo $@:${:U4:S,^,$$$$,:Q}:
	@echo $@:${:U5:S,^,$$$$$,:Q}:
	@echo $@:${:U6:S,^,$$$$$$,:Q}:
	@echo $@:${:U7:S,^,$$$$$$$,:Q}:
	@echo $@:${:U8:S,^,$$$$$$$$,:Q}:
# This generates no dollar at all:
	@echo $@:${:UU8:S,^,${:U$$$$$$$$},:Q}:
# Here is an alternative way to generate dollar characters.
# It's unexpectedly complicated though.
	@echo $@:${:U:range=5:ts\x24:C,[0-9],,g:Q}:

# Demonstrate that it is possible to generate dollar characters using the
# :@ modifier.
#
# These are edge cases that could have resulted in a parse error as well
# since the $@ at the end could have been interpreted as a variable, which
# would mean a missing closing @ delimiter.
mod-loop-dollar:
	@echo $@:${:U1:@word@${word}$@:Q}:
	@echo $@:${:U2:@word@$${word}$$@:Q}:
	@echo $@:${:U3:@word@$$${word}$$$@:Q}:
	@echo $@:${:U4:@word@$$$${word}$$$$@:Q}:
	@echo $@:${:U5:@word@$$$$${word}$$$$$@:Q}:
	@echo $@:${:U6:@word@$$$$$${word}$$$$$$@:Q}:

mod-regex-limits:
	@echo $@:00-ok:${:U1 23 456:C,..,\0\0,:Q}
	@echo $@:11-missing:${:U1 23 456:C,..,\1\1,:Q}
	@echo $@:11-ok:${:U1 23 456:C,(.).,\1\1,:Q}
	@echo $@:22-missing:${:U1 23 456:C,..,\2\2,:Q}
	@echo $@:22-missing:${:U1 23 456:C,(.).,\2\2,:Q}
	@echo $@:22-ok:${:U1 23 456:C,(.)(.),\2\2,:Q}
	# The :C modifier only handles single-digit capturing groups,
	# which is more than enough for daily use.
	@echo $@:capture:${:UabcdefghijABCDEFGHIJrest:C,(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.),\9\8\7\6\5\4\3\2\1\0\10\11\12,}

mod-regex-errors:
	@echo $@: ${UNDEF:Uvalue:C,[,,}

# Just a bit of basic code coverage for the obscure ::= assignment modifiers.
mod-assign:
	@echo $@: ${1 2 3:L:@i@${FIRST::?=$i}@} first=${FIRST}.
	@echo $@: ${1 2 3:L:@i@${LAST::=$i}@} last=${LAST}.
	@echo $@: ${1 2 3:L:@i@${APPENDED::+=$i}@} appended=${APPENDED}.
	@echo $@: ${echo.1 echo.2 echo.3:L:@i@${RAN::!=${i:C,.*,&; & 1>\&2,:S,., ,g}}@} ran:${RAN}.
	# The assignments happen in the global scope and thus are
	# preserved even after the shell command has been run.
	@echo $@: global: ${FIRST:Q}, ${LAST:Q}, ${APPENDED:Q}, ${RAN:Q}.

mod-assign-nested:
	@echo $@: ${1:?${THEN1::=then1${IT1::=t1}}:${ELSE1::=else1${IE1::=e1}}}${THEN1}${ELSE1}${IT1}${IE1}
	@echo $@: ${0:?${THEN2::=then2${IT2::=t2}}:${ELSE2::=else2${IE2::=e2}}}${THEN2}${ELSE2}${IT2}${IE2}
	@echo $@: ${SINK3:Q}
	@echo $@: ${SINK4:Q}
SINK3:=	${1:?${THEN3::=then3${IT3::=t3}}:${ELSE3::=else3${IE3::=e3}}}${THEN3}${ELSE3}${IT3}${IE3}
SINK4:=	${0:?${THEN4::=then4${IT4::=t4}}:${ELSE4::=else4${IE4::=e4}}}${THEN4}${ELSE4}${IT4}${IE4}

mod-tu-space:
	# The :tu and :tl modifiers operate on the variable value
	# as a single string, not as a list of words. Therefore,
	# the adjacent spaces are preserved.
	@echo $@: ${a   b:L:tu:Q}

mod-quote:
	@echo $@: new${.newline:Q}${.newline:Q}line

# Cover the bmake_realloc in brk_string.
mod-break-many-words:
	@echo $@: ${UNDEF:U:range=500:[#]}

# Demonstrate the :_ modifier.
# In the parameterized form, having the variable name on the right side
# of the = assignment operator is confusing. Luckily this modifier is
# only rarely needed.
mod-remember:
	@echo $@: ${1 2 3:L:_:@var@${_}@}
	@echo $@: ${1 2 3:L:@var@${var:_=SAVED:}@}, SAVED=${SAVED}

mod-gmtime:
	@echo $@:
	@echo ${%Y:L:gmtim=1593536400}		# modifier name too short
	@echo ${%Y:L:gmtime=1593536400}		# 2020-07-01T00:00:00Z
	@echo ${%Y:L:gmtimer=1593536400}	# modifier name too long

mod-localtime:
	@echo $@:
	@echo ${%Y:L:localtim=1593536400}	# modifier name too short
	@echo ${%Y:L:localtime=1593536400}	# 2020-07-01T00:00:00Z
	@echo ${%Y:L:localtimer=1593536400}	# modifier name too long

mod-hash:
	@echo $@:
	@echo ${12345:L:has}			# modifier name too short
	@echo ${12345:L:hash}			# ok
	@echo ${12345:L:hash=SHA-256}		# :hash does not accept '='
	@echo ${12345:L:hasX}			# misspelled
	@echo ${12345:L:hashed}			# modifier name too long

mod-range:
	@echo $@:
	@echo ${a b c:L:rang}			# modifier name too short
	@echo ${a b c:L:range}			# ok
	@echo ${a b c:L:rango}			# misspelled
	@echo ${a b c:L:ranger}			# modifier name too long
