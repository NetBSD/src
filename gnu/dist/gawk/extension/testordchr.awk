BEGIN {
	extension("./ordchr.so", "dlload")

	print "ord(\"a\") is", ord("a")
	print "chr(65) is", chr(65)
}
