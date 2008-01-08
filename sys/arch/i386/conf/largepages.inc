#	$NetBSD: largepages.inc,v 1.2.8.1 2008/01/08 22:09:57 bouyer Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERN_LDSCRIPT="kern.ldscript.4MB"
