#	$NetBSD: largepages.inc,v 1.3.54.1 2017/12/03 11:36:17 jdolecek Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERNLDSCRIPT="kern.ldscript.4MB"
