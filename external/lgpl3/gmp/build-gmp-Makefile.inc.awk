#! /usr/bin/awk -f

/^config.status: linking/ {
	# $3 = src
	# $5 = dst

	sub(/mpn\//, "", $5)
	if (match($3, /\.c$/)) {
		c_list[$5] = $3
	} else if (match($3, /\.asm$/)) {
		asm_list[$5] = $3
	}
}

END {
	printf("SRCS+= \\\n");
	# XXX check that the basenames are the same?
	# XXX yeah - logops_n.c and popham.c may need this
	for (c in c_list) {
		printf("\t%s \\\n", c)
	}
	printf("\nASM_SRCS_LIST= \\\n");
	for (asm in asm_list) {
		printf("\t%s\t\t%s \\\n", asm, asm_list[asm])
	}
}
