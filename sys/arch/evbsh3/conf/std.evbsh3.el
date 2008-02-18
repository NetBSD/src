# $NetBSD: std.evbsh3.el,v 1.6.44.1 2008/02/18 21:04:29 mjf Exp $
#
# standard, required NetBSD/evbsh3 'options'

machine evbsh3 sh3
include		"conf/std"	# MI standard options
include		"arch/sh3/conf/std.sh3el"	# arch standard options

makeoptions	DEFTEXTADDR="0x8c010000"
