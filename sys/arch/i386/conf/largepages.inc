#	$NetBSD: largepages.inc,v 1.1.2.2 2000/11/20 20:09:18 bouyer Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions	KERN_LDSCRIPT="kern.ldscript.4MB"
options 	LARGEPAGES		# Use large pages for kernel image
