#	$NetBSD: largepages.inc,v 1.4 2015/08/21 02:02:00 uebayasi Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERNLDSCRIPT="kern.ldscript.4MB"
