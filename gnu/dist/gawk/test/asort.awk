function init(a)
{
	a[1] = "aardvark"
	a[2] = "animal"
	a[3] = "zebra"
	a[4] = "zoo"
	a[5] = "Iguana"
	a[6] = "Alligator"
	a[7] = "Nouns"
	a[8] = "people"
}

BEGIN {

	for (IGNORECASE = 0; IGNORECASE < 2; IGNORECASE++) {
		init(a)

		n = asort(a)

		for (i = 1; i <= n; i++)
			printf("a[%d] = \"%s\"\n", i, a[i])

		print "============"
	}
}
