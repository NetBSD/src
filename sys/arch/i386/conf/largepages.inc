#	$NetBSD: largepages.inc,v 1.1.44.2 2008/01/21 09:36:55 yamt Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERN_LDSCRIPT="kern.ldscript.4MB"
