#	$NetBSD: merge.awk,v 1.2 2001/03/31 11:37:48 aymeric Exp $
#
#	@(#)merge.awk	8.3 (Berkeley) 5/25/94
#
# merge index entries into one line per label
$1 == prev {
	printf ", %s", $2;
	next;
}
{
	if (NR != 1)
		printf "\n";
	printf "%s \t%s", $1, $2;
	prev = $1;
}
END {
	printf "\n"
}
