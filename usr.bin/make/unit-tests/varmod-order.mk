# $NetBSD: varmod-order.mk,v 1.3 2020/08/16 20:13:10 rillig Exp $
#
# Tests for the :O variable modifier, which returns the words, sorted in
# ascending order.

NUMBERS=	one two three four five six seven eight nine ten

.if ${NUMBERS:O} != "eight five four nine one seven six ten three two"
.error ${NUMBERS:O}
.endif

all:
	@:;
