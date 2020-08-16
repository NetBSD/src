# $NetBSD: varmod-subst-regex.mk,v 1.2 2020/08/16 12:30:45 rillig Exp $
#
# Tests for the :C,from,to, variable modifier.

all: mod-regex
all: mod-regex-limits
all: mod-regex-errors

mod-regex:
	@echo $@:
	@echo :${:Ua b b c:C,a b,,:Q}:
	@echo :${:Ua b b c:C,a b,,1:Q}:
	@echo :${:Ua b b c:C,a b,,W:Q}:
	@echo :${:Uword1 word2:C,****,____,g:C,word,____,:Q}:
	@echo :${:Ua b b c:C,b,,g:Q}:
	@echo :${:U1 2 3 1 2 3:C,1 2,___,Wg:C,_,x,:Q}:

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
