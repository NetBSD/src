#! /bin/awk -f

BEGIN {
	extension("./arrayparm.so", "dlload")

	mkarray(newvar, "hi", "hello")

	for (i in newvar)
		printf ("newvar[\"%s\"] = \"%s\"\n", i, newvar[i])
}
