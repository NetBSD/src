# std.evbsh3.el,v 1.6 2006/03/17 16:06:51 uebayasi Exp
#
# standard, required NetBSD/evbsh3 'options'

machine evbsh3 sh3
include		"conf/std"	# MI standard options
include		"arch/sh3/conf/std.sh3el"	# arch standard options

makeoptions	DEFTEXTADDR="0x8c010000"
