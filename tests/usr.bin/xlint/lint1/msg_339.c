/*	$NetBSD: msg_339.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_339.c"

// Test for message: option '%c' should be listed in the options string [339]

int getopt(int, char *const *, const char *);
extern char *optarg;

int
main(int argc, char **argv)
{
	int o;

	/* expect+2: warning: option 'c' should be handled in the switch [338] */
	/* expect+1: warning: option 'd' should be handled in the switch [338] */
	while ((o = getopt(argc, argv, "a:bc:d")) != -1) {
		switch (o) {
		case 'a':
			break;
		case 'b':
			/*
			 * The following while loop must not finish the check
			 * for the getopt options.
			 */
			while (optarg[0] != '\0')
				optarg++;
			break;
		case 'e':
			/* expect-1: warning: option 'e' should be listed in the options string [339] */
			break;
		case 'f':
			/* expect-1: warning: option 'f' should be listed in the options string [339] */
			/*
			 * The case labels in nested switch statements are
			 * ignored by the check for getopt options.
			 */
			switch (optarg[0]) {
			case 'X':
				break;
			}
			break;
		case '?':
		default:
			break;
		}
	}

	return 0;
}
