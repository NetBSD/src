BEGIN {
	# 1. Should print aCa
	IGNORECASE = 1
	FS = "[c]"
	IGNORECASE = 0
	$0 = "aCa"
	print $1

	# 2. Should print a
	IGNORECASE = 1
	FS = "[c]"
	$0 = "aCa"
	print $1

	# 3. Should print a
	IGNORECASE = 1
	FS = "C"
	IGNORECASE = 0
	$0 = "aCa"
	print $1

	# 4. Should print aCa
	IGNORECASE = 1
	FS = "c"
	$0 = "aCa"
	print $1

	# 5. Should print aCa
	FS = "xy"
	IGNORECASE = 0
	FS = "c"
	IGNORECASE = 1
	$0 = "aCa"
	print $1

	# 6. Should print aCa
	FS = "xy"
	IGNORECASE = 0
	FS = "c"
	IGNORECASE = 1
	split("aCa",a)
	print a[1]
}
