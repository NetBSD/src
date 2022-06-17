/*	$NetBSD: msg_338.c,v 1.8 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_338.c"

// Test for message: option '%c' should be handled in the switch [338]

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

/*
 * If the first character of the options string is ':', getopt does not print
 * its own error messages. Getopt returns ':' if an option is missing its
 * argument; that is handled by the 'default:' already.
 */
int
suppress_errors(int argc, char **argv)
{
	int c;

	/* expect+1: warning: option 'o' should be handled in the switch [338] */
	while ((c = getopt(argc, argv, ":b:o")) != -1) {
		switch (c) {
		case 'b':
			return 'b';
		default:
			usage();
		}
	}
	return 0;
}

/*
 * If the first character of the options string is ':', getopt returns ':'
 * if an option is missing its argument. This condition can be handled
 * separately from '?', which getopt returns for unknown options.
 */
int
missing_argument(int argc, char **argv)
{
	int c;

	/* expect+1: warning: option 'o' should be handled in the switch [338] */
	while ((c = getopt(argc, argv, ":b:o")) != -1) {
		switch (c) {
		case 'b':
			return 'b';
		case ':':
			return 'm';
		default:
			usage();
		}
	}
	return 0;
}

/*
 * Getopt only returns ':' if ':' is the first character in the options
 * string. Everywhere else, a ':' marks the preceding option as having a
 * required argument. In theory, if the options string contained "a::x",
 * that could be interpreted as '-a argument', followed by '-:' and '-x',
 * but nobody does that.
 */
int
unreachable_colon(int argc, char **argv)
{
	int c;

	/* expect+1: warning: option 'b' should be handled in the switch [338] */
	while ((c = getopt(argc, argv, "b:")) != -1) {
		switch (c) {
		/* expect+1: warning: option ':' should be listed in the options string [339] */
		case ':':
			return 'm';
		default:
			usage();
		}
	}
	return 0;
}
