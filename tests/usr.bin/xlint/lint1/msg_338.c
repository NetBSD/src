/*	$NetBSD: msg_338.c,v 1.5 2021/08/22 22:15:07 rillig Exp $	*/
# 3 "msg_338.c"

// Test for message: option '%c' should be handled in the switch [338]

int getopt(int, char *const *, const char *);
extern char *optarg;

int
main(int argc, char **argv)
{
	int o;

	while ((o = getopt(argc, argv, "a:bc:d")) != -1) { /* expect: 338 *//* expect: 338 */
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
		case 'e':	/* expect: option 'e' should be listed */
			break;
		case 'f':	/* expect: option 'f' should be listed */
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

	/* A while loop that is not related to getopt is simply skipped. */
	while (o != 0) {
		switch (o) {
		case '?':
			o = ':';
		}
	}

	return 0;
}

void usage(void);

/*
 * Before ckgetopt.c 1.11 from 2021-08-23, lint wrongly warned about a
 * missing '?' in the switch statement, even though it was there.
 *
 * Seen in usr.bin/ftp/main.c 1.127 from 2020-07-18.
 */
int
question_option(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "?x")) != -1) {
		switch (c) {
		case 'x':
			break;
		case '?':
			usage();
			return 0;
		default:
			usage();
			return 1;
		}
	}
	return 0;
}
