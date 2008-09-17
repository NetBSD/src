#	$NetBSD: merge.awk,v 1.1.2.2 2008/09/17 05:42:10 wrstuden Exp $
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
