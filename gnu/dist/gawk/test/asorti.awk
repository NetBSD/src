function init(a)
{
	delete a

	a["aardvark"] = 1
	a["animal"] = 2
	a["zebra"] = 3
	a["zoo"] = 4
	a["Iguana"] = 5
	a["Alligator"] = 6
	a["Nouns"] = 7
	a["people"] = 8
}

BEGIN {

	for (IGNORECASE = 0; IGNORECASE < 2; IGNORECASE++) {
		init(a)

		n = asorti(a)

		for (i = 1; i <= n; i++)
			printf("a[%d] = \"%s\"\n", i, a[i])

		print "============"
	}
}
