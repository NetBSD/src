{ word[$0]++ }
END {
	for (i in word)
		print i
}
