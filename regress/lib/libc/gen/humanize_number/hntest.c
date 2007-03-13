/*	$NetBSD: hntest.c,v 1.5 2007/03/13 02:56:18 enami Exp $	*/

#include <err.h>
#include <inttypes.h>
#include <stdarg.h>
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

	{ 5, 1, "", 0, 0, 2, "1 " }, /* just for reference */
	{ 5, 1, "", 0, HN_B, 3, "1 B" }, /* and more ... */
	{ 5, 1, "", 0, HN_DECIMAL, 2, "1 " },
	{ 5, 1, "", 0, HN_NOSPACE | HN_B, 2, "1B" },
	{ 5, 1, "", 0, HN_B | HN_DECIMAL, 3, "1 B" },
	{ 5, 1, "", 0, HN_NOSPACE | HN_B | HN_DECIMAL, 2, "1B" },

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

	/*
	 * Failure case reported by Greg Troxel <gdt@NetBSD.org>.
	 * Rev. 1.11 incorrectly returns 5 with filling the buffer
	 * with "1000".
	 */
	{ 5, 1048258238, "",
	  HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL, 4, "1.0G" },
	/* Similar case it prints 1000 where it shouldn't */
	{ 5, 1023488, "",
	  HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL, 4, "1.0M" },
	{ 5, 1023999, "",
	  HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL, 4, "1.0M" },
};

struct hnflags {
	int hf_flags;
	const char *hf_name;
};

const struct hnflags scale_flags[] = {
	{ HN_GETSCALE, "HN_GETSCALE" },
	{ HN_AUTOSCALE, "HN_AUTOSCALE" },
};
const struct hnflags normal_flags[] = {
	{ HN_DECIMAL, "HN_DECIMAL" },
	{ HN_NOSPACE, "HN_NOSPACE" },
	{ HN_B, "HN_B" },
	{ HN_DIVISOR_1000, "HN_DIVISOR_1000" },
};

const char *
	formatflags(char *, size_t, const struct hnflags *, size_t, int);
void	newline(void);
void	w_printf(const char *, ...);
int	main(int, char *[]);

const char *
formatflags(char *buf, size_t buflen, const struct hnflags *hfs,
    size_t hfslen, int flags)
{
	const struct hnflags *hf;
	char *p = buf;
	size_t len = buflen;
	int i, found, n;

	if (flags == 0) {
		snprintf(buf, buflen, "0");
		return (buf);
	}
	for (i = found = 0; i < hfslen && flags & ~found; i++) {
		hf = &hfs[i];
		if (flags & hf->hf_flags) {
			found |= hf->hf_flags;
			n = snprintf(p, len, "|%s", hf->hf_name);
			if (n >= len) {
				p = buf;
				len = buflen;
				/* Print `flags' as number */
				goto bad;
			}
			p += n;
			len -= n;
		}
	}
	flags &= ~found;
	if (flags)
bad:
		snprintf(p, len, "|0x%x", flags);
	return (*buf == '|' ? buf + 1 : buf);
}

static int col, bol = 1;
void
newline(void)
{

	fprintf(stderr, "\n");
	col = 0;
	bol = 1;
}

void
w_printf(const char *fmt, ...)
{
	char buf[80];
	va_list ap;
	int n;

	va_start(ap, fmt);
	if (col >= 0) {
		n = vsnprintf(buf, sizeof(buf), fmt, ap);
		if (n >= sizeof(buf)) {
			col = -1;
			goto overflow;
		} else if (n == 0)
			goto out;

		if (!bol) {
			if (col + n > 75)
				fprintf(stderr, "\n    "), col = 4;
			else
				fprintf(stderr, " "), col++;
		}
		fprintf(stderr, "%s", buf);
		col += n;
		bol = 0;
	} else {
overflow:
		vfprintf(stderr, fmt, ap);
	}
out:
	va_end(ap);
}

int
main(int argc, char *argv[])
{
	char fbuf[128];
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

		w_printf("humanize_number(\"%s\", %d, %" PRId64 ",",
		    ho->ho_retstr, ho->ho_len, ho->ho_num);
		w_printf("\"%s\",", ho->ho_suffix);
		w_printf("%s,", formatflags(fbuf, sizeof(fbuf), scale_flags,
		    sizeof(scale_flags) / sizeof(scale_flags[0]),
		    ho->ho_scale));
		w_printf("%s)", formatflags(fbuf, sizeof(fbuf), normal_flags,
		    sizeof(normal_flags) / sizeof(normal_flags[0]),
		    ho->ho_flags));
		w_printf("= %d,", ho->ho_retval);
		w_printf("but got");
		w_printf("%d/[%s]", rv, rv == -1 ? "" : buf);
		newline();
		error = 1;
	}
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}
