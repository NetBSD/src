/*	$NetBSD: xpm2bootimg.c,v 1.1.4.2 2002/02/28 04:12:38 nathanw Exp $	*/

/*
 *	convert XPM format image to boot title format
 *
 *	written by Yasha (ITOH Yasufumi), public domain
 */

#include <stdio.h>
#include <string.h>

static int opt_ascii;

#define IMGWIDTH	56
#define IMGHEIGHT	52

/* if you change this, you must also make changes to the extraction code */
#define VALBIT	2
#define LENBIT	3
#define LENMAX	(1<<LENBIT)

#if VALBIT + LENBIT > 8
 #error too long encoding --- not portable between architectures in this code
#endif

/* this program may run on cross host, and should be portable */
#ifdef __STDC__
# define PROTO(x)	x
#else
# define PROTO(x)	()
#endif

static void putbyte PROTO((int c));
static void initdot PROTO((void));
static void putrun PROTO((int val, int len));
static void adddot PROTO((int val));
static void flushdot PROTO((void));

static unsigned rgb16b PROTO((int rgb));
static char *destring PROTO((char *str));
static char *getline PROTO((void));
static void error PROTO((char *msg));
int main PROTO((int argc, char *argv[]));

static int outbuf;
static int bufbits;
static int curval;
static int curlen;

static int obytes;

static void
putbyte(c)
	int c;
{
	static unsigned char wbuf;

	if (c == -1) {
		if (obytes % 16 && opt_ascii)
			printf("\n");
		if (obytes & 1) {
			if (opt_ascii)
				printf("\t.byte\t0x%02x\n", wbuf);
			else
				putchar(wbuf);
		}
		if (opt_ascii)
			printf("| compressed image %d bytes\n", obytes);
		return;
	}
	if (obytes % 16 == 0 && opt_ascii)
		printf("\t.word\t");

	obytes++;
	if (obytes & 1)
		wbuf = c;
	else {
		if (opt_ascii) {
			if ((obytes >> 1) % 8 != 1)
				printf(",");
			printf("0x%04x", (wbuf << 8) | c);
		} else
			printf("%c%c", wbuf, c);
	}

	if (obytes % 16 == 0 && opt_ascii)
		printf("\n");
}

static void
initdot()
{

	outbuf = bufbits = curval = curlen = obytes = 0;
}

static int put;

static void
putrun(val, len)
	int val, len;
{

/*	fprintf(stderr, "val %d, len %d\n", val, len);*/
	outbuf <<= VALBIT;
	outbuf |= val;
	outbuf <<= LENBIT;
	outbuf |= len - 1;
	bufbits += VALBIT + LENBIT;

	if (bufbits >= 8) {
		putbyte((unsigned char) (outbuf >> (bufbits - 8)));
		bufbits -= 8;
		put = 1;
	}
}

static void
adddot(val)
	int val;
{

	if (curval != val) {
		if (curlen)
			putrun(curval, curlen);
		curlen = 0;
		curval = val;
	}
	curlen++;
	if (curlen == LENMAX) {
		putrun(val, LENMAX);
		curlen = 0;
	}
}

static void
flushdot()
{

	if (curlen) {
		putrun(curval, curlen);
		curlen = 0;
	}

	if (bufbits) {
		/* make sure data drain */
		put = 0;
		while (put == 0)
			putrun(curval, LENMAX);
	}
	putbyte(-1);
}

/*
 * convert r8g8b8 to g5r5b5i1
 */
static unsigned
rgb16b(rgb)
	int rgb;
{
	unsigned r = rgb >> 16, g = (rgb >> 8) & 0xff, b = rgb & 0xff;
	unsigned rgb16;

	rgb16 = (g << 8 & 0xf800) | (r << 3 & 0x7c0) | (b >> 2 & 0x3e);

	/*
	 *  v v v v v i i i
	 * valid bits  used for I bit
	 */
	if ((r & 7) + (g & 7) + (b & 7) >= 11)
		rgb16 |= 1;

	return rgb16;
}

static char *
destring(str)
	char *str;	/* must be writable */
{
	size_t len;
	char *p;

	if (*str != '"' || (len = strlen(str)) < 2)
		return NULL;
	p = str + len - 1;
	if (*p == ',') {
		if (len < 3)
			return NULL;
		p--;
	}

	if (*p != '"')
		return NULL;

	*p = '\0';
	return str + 1;
}

static char *filename;
static FILE *infp;
static unsigned lineno;

static char *
getline()
{
	static char buf[256];
	char *p;

	if (!fgets(buf, sizeof buf, infp)) {
		if (ferror(infp)) {
			perror(filename);
			exit(1);
		} else
			return NULL;	/* end of input */
	}
	lineno++;
	if (!(p = strchr(buf, '\n'))) {
		fprintf(stderr, "%s:%d: too long line\n", filename, lineno);
		exit(1);
	}
	*p = '\0';

	return buf;
}

static void
error(msg)
	char *msg;
{
	if (!msg)
		msg = "format error";

	fprintf(stderr, "%s:%d: %s\n", filename, lineno, msg);
	exit(1);
}

static struct color {
	int		ch;
	enum col {
		COL_BLACK, COL_1, COL_2, COL_WHITE
	} val;
} coltbl[32];

unsigned col1, col2;

enum col bitmap[IMGHEIGHT][IMGWIDTH];

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *p;
	unsigned u, colors, xcol, x;
	char buf[256];
	int in_oc;
	char *progname = argv[0];

	/*
	 * parse arg
	 */
	if (argc > 1 && !strcmp(argv[1], "-s")) {
		/*
		 * -s option: output assembler source
		 * (output binary otherwise)
		 */
		opt_ascii = 1;
		argc--;
		argv++;
	}
	if (argc == 1) {
		infp = stdin;
		filename = "stdin";
	} else if (argc == 2) {
		if ((infp = fopen(argv[1], "r")) == NULL) {
			perror(argv[1]);
			return 1;
		}
		filename = argv[1];
	} else {
		fprintf(stderr, "usage: %s [file.xpm]\n", progname);
		return 1;
	}

	/*
	 * check XPM magic
	 */
	if (!(p = getline()))
		error("short file");

	if (strcmp(p, "/* XPM */"))
		error((char *) NULL);

	while ((p = getline()) && !(p = destring(p)))
		;
	if (!p)
		error((char *) NULL);

	/*
	 * the first string must be
	 *	"56 52 5 1 XPMEXT",
	 */
	{
		unsigned w, h, cpp;

		if (sscanf(p, "%u %u %u %u %s",
				&w, &h, &colors, &cpp, buf) != 5)
			error("must be \"56 52 * 1 XPMEXT\"");

		if (w != IMGWIDTH)
			error("image width must be 56");
		if (h != IMGHEIGHT)
			error("image height must be 52");
		if (cpp != 1)
			error("chars-per-pixel must be 1");
		if (strcmp(buf, "XPMEXT"))
			error("XPMEXT is required");
	}
	if (colors > sizeof coltbl / sizeof coltbl[0])
		error("too many colors");

	/*
	 * read colors
	 * ". c #ffffff",
	 */
	xcol = 0;
	for (u = 0; u < colors; u++) {
		while ((p = getline()) && !(p = destring(p)))
			;
		if (!p)
			error((char *) NULL);
		if (sscanf(p, "%c %c %s", buf, buf+1, buf+2) != 3)
			error((char *) NULL);

		coltbl[u].ch = buf[0];
		if (buf[2] == '#') {
			int v;
			if (sscanf(buf+3, "%x", &v) != 1)
				error((char *) NULL);
			if (v == 0)
				coltbl[u].val = COL_BLACK;
			else if (v == 0xffffff)
				coltbl[u].val = COL_WHITE;
			else if (xcol == 0) {
				coltbl[u].val = COL_1;
				col1 = rgb16b(v);
				xcol++;
			} else if (xcol == 1) {
				coltbl[u].val = COL_2;
				col2 = rgb16b(v);
				xcol++;
			} else
				error("too many colors");
		} else if (!strcmp(buf+2, "None")) {
			/*
			 * transparent color is treated as black
			 */
			coltbl[u].val = COL_BLACK;
		} else
			error("unknown color (symbolic name is not supported)");
	}

	/*
	 * read bitmaps
	 */
	for (u = 0; u < IMGHEIGHT; u++) {

		while ((p = getline()) && !(p = destring(p)))
			;
		if (!p)
			error((char *) NULL);

		if (strlen(p) != IMGWIDTH)
			error((char *) NULL);

		for (x = 0; x < IMGWIDTH; x++, p++) {
			unsigned i;

			for (i = 0; i < colors; i++)
				if (coltbl[i].ch == *p)
					goto found_ch;
			error("unknown character");

		found_ch:
			bitmap[u][x] = coltbl[i].val;
		}
	}

	/*
	 * read XPMEXTs and output copyright string
	 */
	in_oc = 0;
	while ((p = getline()) && *p == '\"') {
		if (!(p = destring(p)))
			error((char *) NULL);
		if (!strcmp(p, "XPMEXT copyright"))
			in_oc = 1;
		else if (!strncmp(p, "XPMENDEXT", 3))
			break;
		else if (!strncmp(p, "XPM", 3))
			in_oc = 0;
		else {
			if (in_oc) {
				if (opt_ascii)
					printf("\t.ascii\t\"\\n%s\"\n", p);
				else
					printf("\n%s", p);
			}
		}
	}

	/* terminate string */
	if (opt_ascii)
		printf("\t.byte\t0\n");
	else
		putchar('\0');

	/* output color palette */
	if (opt_ascii)
		printf("\t.word\t0x%x,0x%x\n", col1, col2);
	else
		printf("%c%c%c%c", col1 >> 8, col1, col2 >> 8, col2);

	/*
	 * scan bitmap and output
	 */
	initdot();

	for (u = 0; u < IMGHEIGHT; u++)
		for (x = 0; x < IMGWIDTH; x++)
			adddot(bitmap[u][x]);

	flushdot();

	if (opt_ascii)
		printf("\t.even\n");

	return ferror(stdout);
}
