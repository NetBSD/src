# $Id: modmisc.mk,v 1.42 2020/08/16 12:30:45 rillig Exp $
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
all:	mod-assign
all:	mod-assign-nested
all:	mod-tu-space
all:	mod-quote
all:	mod-break-many-words
all:	mod-remember
all:	mod-gmtime
all:	mod-gmtime-indirect
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
	@echo ${%Y:L:gm=gm:M*}

mod-gmtime-indirect:
	@echo $@:
	# It's not possible to pass the seconds via a variable expression.
	# Parsing of the :gmtime modifier stops at the '$' and returns to
	# ApplyModifiers.  There, a colon would be skipped but not a dollar.
	# Parsing continues by looking at the next modifier.  Now the ${:U}
	# is expanded and interpreted as a variable modifier, which results
	# in the error message "Unknown modifier '1'".
	@echo ${%Y:L:gmtime=${:U1593536400}}

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

# To apply a modifier indirectly via another variable, the whole
# modifier must be put into a single variable.
.if ${value:L:${:US}${:U,value,replacement,}} != "S,value,replacement,}"
.warning unexpected
.endif

# Adding another level of indirection (the 2 nested :U expressions) helps.
.if ${value:L:${:U${:US}${:U,value,replacement,}}} != "replacement"
.warning unexpected
.endif

# Multiple indirect modifiers can be applied one after another as long as
# they are separated with colons.
.if ${value:L:${:US,a,A,}:${:US,e,E,}} != "vAluE"
.warning unexpected
.endif

# An indirect variable that evaluates to the empty string is allowed though.
# This makes it possible to define conditional modifiers, like this:
#
# M.little-endian=	S,1234,4321,
# M.big-endian=		# none
.if ${value:L:${:Dempty}S,a,A,} != "vAlue"
.warning unexpected
.endif

# begin mod-shell

.if ${:!echo hello | tr 'l' 'l'!} != "hello"
.warning unexpected
.endif

# The output is truncated at the first null byte.
# Cmd_Exec returns only a string pointer without length information.
.if ${:!echo hello | tr 'l' '\0'!} != "he"
.warning unexpected
.endif

.if ${:!echo!} != ""
.warning A newline at the end of the output must be stripped.
.endif

.if ${:!echo;echo!} != " "
.warning Only a single newline at the end of the output is stripped.
.endif

.if ${:!echo;echo;echo;echo!} != "   "
.warning Other newlines in the output are converted to spaces.
.endif

# end mod-shell
