$NetBSD: mean.awk,v 1.1.1.3 1997/10/03 22:25:25 christos Exp $
/^ *[0-9]/	{
	# print out the average time to each hop along a route.
	tottime = 0; n = 0;
	for (f = 5; f <= NF; ++f) {
		if ($f == "ms") {
			tottime += $(f - 1)
			++n
		}
	}
	if (n > 0)
		print $1, tottime/n, median
}
