#	$NetBSD: libuuid_ul-rename.mk,v 1.1 2019/03/07 03:53:51 mrg Exp $

#
# functions exported by fontconfig's private libuuid copy are
# renamed to have a "ul_" prefix (from util-linux).  listed here
# so that both the libuuid_ul build and fontconfig can find them.
#

RENAME_FUNCS= \
	uuid_compare uuid_copy \
	uuid_generate uuid_generate_random \
	uuid_parse uuid_unparse \
	uuid_pack uuid_unpack
.for _f in ${RENAME_FUNCS}
CPPFLAGS+= -D${_f}=ul_${_f}
.endfor
