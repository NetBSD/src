BEGIN {
	data = "foooobazbarrrrr"
	match(data, /(fo+).+(bar*)/, arr)
	for (i = 0; i in arr; i++) {
		printf("arr[%d] = \"%s\"\n", i, arr[i])
		printf("arr[%d, \"start\"] = %s, arr[%d, \"length\"] = %s\n",
			i, arr[i, "start"], i, arr[i, "length"])
	}
}
