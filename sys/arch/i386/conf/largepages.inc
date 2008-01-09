#	$NetBSD: largepages.inc,v 1.1.106.2 2008/01/09 01:46:32 matt Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERN_LDSCRIPT="kern.ldscript.4MB"
