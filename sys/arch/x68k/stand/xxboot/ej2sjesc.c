/*
 *	Convert an EUC-Japanese code C source file to Shift-JIS
 *	and to \xxx escape sequences (X680x0 IOCS uses Shift-JIS code)
 *
 *	This is compiled and executed on the host, and should be portable
 *
 *	Written by Yasha (ITOH Yasufumi)
 *	This code is in the public domain
 *
 *	$NetBSD: ej2sjesc.c,v 1.1 1998/09/01 20:02:34 itohy Exp $
 */

#include <stdio.h>

#ifndef __P
# ifdef __STDC__
#  define __P(x)	x
# else
#  define __P(x)	()
# endif
#endif

void euc_to_sjis __P((int *ph, int *pl));
int xfclose __P((FILE *fp));
int main __P((int argc, char *argv[]));

#define ISEUC1(c)	((unsigned char)(c) & 0x80)
#define ISEUC2(c)	((unsigned char)(c) & 0x80)

#define EUC2JIS(c)	((unsigned char)(c) & 0x7f)

void
euc_to_sjis(ph, pl)
	int *ph, *pl;
{
	int hi = EUC2JIS(*ph);
	int lo = EUC2JIS(*pl);

	lo += (hi % 2) ? 0x1f : 0x7d;
	if (lo >= 0x7f)
		lo++;
	hi = (hi - 0x21)/2 + 0x81;
	if (hi > 0x9f)
		hi += 0x40;
	*ph = hi;
	*pl = lo;
}

/*
 * I hear that closing stdin/stdout/stderr crashes exit(3) on some old BSDs
 * (?? is it true?)
 */
int
xfclose(fp)
	FILE *fp;
{

	if (fp == stdin)
		return 0;		/* do nothing */

	if (fp == stdout || fp == stderr)
		return fflush(fp);	/* complete write but not close */

	return fclose(fp);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *in, *out;
	int c;
	int error = 0;

	if (argc != 3) {
		fprintf(stderr, "usage: %s infile outfile\n", argv[0]);
		return 1;
	}

	if (!strcmp(argv[1], "-"))
		in = stdin;
	else if (!(in = fopen(argv[1], "r"))) {
		perror(argv[1]);
		return 1;
	}

	if (!strcmp(argv[2], "-"))
		out = stdout;
	else if (!(out = fopen(argv[2], "w"))) {
		perror(argv[2]);
		xfclose(in);
		return 1;
	}

	/* notify the original filname to the C compiler */
	fprintf(out, "#line 1 \"%s\"\n", argv[1]);

	while ((c = getc(in)) != EOF) {
		if (ISEUC1(c)) {
			int hi = c, lo = getc(in);

			if (!ISEUC2(lo)) {
				fprintf(stderr, "%s: not in EUC code\n",
						argv[1]);
				error = 1;
				break;
			}
			euc_to_sjis(&hi, &lo);
#if 0	/* for debug */
			putc(hi, out);
			putc(lo, out);
#else
			fprintf(out, "\\%03o\\%03o", hi, lo);
#endif
		} else
			putc(c, out);
	}
	if (ferror(in) || xfclose(in)) {
		perror(argv[1]);
		error = 1;
	}
	if (ferror(out) || xfclose(out)) {
		perror(argv[2]);
		error = 1;
	}

	if (error)
		remove(argv[2]);

	return error;
}
