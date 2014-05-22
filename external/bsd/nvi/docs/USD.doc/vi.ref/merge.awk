#	$NetBSD: merge.awk,v 1.1.4.2 2014/05/22 15:50:36 yamt Exp $
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
