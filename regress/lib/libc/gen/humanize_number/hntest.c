/*	$NetBSD: hntest.c,v 1.1 2004/07/14 22:47:31 enami Exp $	*/

#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

const struct hnopts {
	size_t ho_len;
	int64_t ho_num;
	const char *ho_suffix;
	int ho_scale;
	int ho_flags;
	int ho_retval;			/* expected return value */
	const char *ho_retstr;		/* expected string in buffer */
} hnopts[] = {
	/*
	 * Rev. 1.6 produces "10.0".
	 */
	{ 5, 10737418236ULL * 1024, "",
	  HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL, 3, "10T" },

	{ 5, 10450000, "",
	  HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL, 3, "10M" },
	{ 5, 10500000, "",		/* just for reference */
	  HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL, 3, "10M" },

	/*
	 * Trailing space.  Rev. 1.7 produces "1 ".
	 */
	{ 5, 1, "", 0, HN_NOSPACE, 1, "1" },

	/*
	 * Space and HN_B.  Rev. 1.7 produces "1B".
	 */
	{ 5, 1, "", HN_AUTOSCALE, HN_B, 3, "1 B" },
	{ 5, 1000, "",			/* just for reference */
	  HN_AUTOSCALE, HN_B, 3, "1 K" },

	/*
	 * Truncated output.  Rev. 1.7 produces "1.0 K".
	 */
	{ 6, 1000, "A", HN_AUTOSCALE, HN_DECIMAL, -1, "" },
};

int
main(int argc, char *argv[])
{
	const struct hnopts *ho;
	char *buf = NULL;
	size_t buflen = 0;
	int i, rv, error = 0;

	for (i = 0; i < sizeof(hnopts) / sizeof(hnopts[0]); i++) {
		ho = &hnopts[i];
		if (buflen < ho->ho_len) {
			buflen = ho->ho_len;
			buf = realloc(buf, buflen);
			if (buf == NULL)
				err(1, "realloc(..., %d)", buflen);
		}

		rv = humanize_number(buf, ho->ho_len, ho->ho_num,
		    ho->ho_suffix, ho->ho_scale, ho->ho_flags);

		if (rv == ho->ho_retval &&
		    (rv == -1 || strcmp(buf, ho->ho_retstr) == 0))
			continue;

		fprintf(stderr,
		    "humanize_number(\"%s\", %d, %" PRId64
		    ", \"%s\", 0x%x, 0x%x) = %d, but got %d/[%s]\n",
		    ho->ho_retstr, ho->ho_len, ho->ho_num,
		    ho->ho_suffix, ho->ho_scale, ho->ho_flags,
		    ho->ho_retval, rv, rv == -1 ? "" : buf);
		error = 1;
	}
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}
