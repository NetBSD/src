#	$NetBSD: lkmtramp.awk,v 1.1 2008/03/01 12:40:08 skrll Exp $
#
BEGIN {
	print "#include <machine/asm.h>"
}

$2 == "R_PARISC_PCREL17F" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next;
	print "LEAF_ENTRY(__wrap_"$3")"

	print "\tldil\tL%__real_"$3",%r1"
	print "\tldo\tR%__real_"$3"(%r1),%r1"
	print "\tbv,n\t%r0(%r1)"
	print "\tnop"

	print "EXIT(__wrap_"$3")"
	x[$3]=".";
}
