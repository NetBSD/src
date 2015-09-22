#	$NetBSD: largepages.inc,v 1.3.74.1 2015/09/22 12:05:44 skrll Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERNLDSCRIPT="kern.ldscript.4MB"
