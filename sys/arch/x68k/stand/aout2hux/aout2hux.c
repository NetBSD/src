/*
 *	aout2hux - convert a.out executable to Human68k .x format
 *
 *	Read two a.out format executables with different load addresses
 *	and generate Human68k .x format executable.
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 * usage:
 *	aout2hux [ -o output.x ] a.out1 loadaddr1 a.out2 loadaddr2
 *
 *	The input a.out files must be static OMAGIC/NMAGIC m68k executables.
 *	Two a.out files must have different loading addresses.
 *	Each of the load address must be a hexadecimal number.
 *	Load address shall be multiple of 4 for as / ld of NetBSD/m68k.
 *
 * example:
 *	% cc -N -static -Wl,-T,0        -o aout1 *.o
 *	% cc -N -static -Wl,-T,10203040 -o aout2 *.o
 *	% aout2hux -o foo.x aout1 0 aout2 10203040
 *
 *	$NetBSD: aout2hux.c,v 1.2 1999/02/02 10:00:18 itohy Exp $
 */

#include <sys/types.h>
#ifndef NO_UNISTD
# include <unistd.h>
#endif
#ifndef NO_STDLIB
# include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>

#include "type_local.h"
#include "aout68k.h"
#include "hux.h"

#ifndef DEFAULT_OUTPUT_FILE
# define DEFAULT_OUTPUT_FILE	"out.x"
#endif

#ifdef DEBUG
# define DPRINTF(x)	printf x
#else
# define DPRINTF(x)
#endif

unsigned get_uint16 PROTO((be_uint16_t *be));
u_int32_t get_uint32 PROTO((be_uint32_t *be));
void put_uint16 PROTO((be_uint16_t *be, unsigned v));
void put_uint32 PROTO((be_uint32_t *be, u_int32_t v));
void *do_realloc PROTO((void *p, size_t s));

FILE *open_aout PROTO((const char *fn, struct aout_m68k *hdr));
int check_2_aout_hdrs PROTO((struct aout_m68k *h1, struct aout_m68k *h2));
int aout2hux PROTO((const char *fn1, const char *fn2,
		u_int32_t loadadr1, u_int32_t loadadr2, const char *fnx));
int gethex PROTO((u_int32_t *pval, const char *str));
void usage PROTO((const char *name));
int main PROTO((int argc, char *argv[]));

#if !defined(bzero) && defined(__SVR4)
# define bzero(d, n)	memset((d), 0, (n))
#endif

/*
 * read/write big-endian integer
 */

unsigned
get_uint16(be)
	be_uint16_t *be;
{

	return be->val[0] << 8 | be->val[1];
}

u_int32_t
get_uint32(be)
	be_uint32_t *be;
{

	return be->val[0]<<24 | be->val[1]<<16 | be->val[2]<<8 | be->val[3];
}

void
put_uint16(be, v)
	be_uint16_t *be;
	unsigned v;
{

	be->val[0] = (u_int8_t) (v >> 8);
	be->val[1] = (u_int8_t) v;
}

void
put_uint32(be, v)
	be_uint32_t *be;
	u_int32_t v;
{

	be->val[0] = (u_int8_t) (v >> 24);
	be->val[1] = (u_int8_t) (v >> 16);
	be->val[2] = (u_int8_t) (v >> 8);
	be->val[3] = (u_int8_t) v;
}

void *
do_realloc(p, s)
	void *p;
	size_t s;
{

	p = p ? realloc(p, s) : malloc(s);	/* for portability */

	if (!p) {
		fprintf(stderr, "malloc failed\n");
		exit(1);
	}

	return p;
}

/*
 * open an a.out and check the header
 */
FILE *
open_aout(fn, hdr)
	const char *fn;
	struct aout_m68k *hdr;
{
	FILE *fp;
	int i;

	if (!(fp = fopen(fn, "r"))) {
		perror(fn);
		return (FILE *) NULL;
	}
	if (fread(hdr, sizeof(struct aout_m68k), 1, fp) != 1) {
		fprintf(stderr, "%s: can't read a.out header\n", fn);
		goto out;
	}

	if ((i = AOUT_GET_MAGIC(hdr)) != AOUT_OMAGIC && i != AOUT_NMAGIC) {
		fprintf(stderr, "%s: not an OMAGIC or NMAGIC a.out\n", fn);
		goto out;
	}

	if ((i = AOUT_GET_MID(hdr)) != AOUT_MID_M68K && i != AOUT_MID_M68K4K) {
		fprintf(stderr, "%s: wrong architecture (mid %d)\n", fn, i);
		goto out;
	}

	/* if unsolved relocations exist, not an executable but an object */
	if (hdr->a_trsize.hostval || hdr->a_drsize.hostval) {
		fprintf(stderr, "%s: not an executable (object file?)\n", fn);
		goto out;
	}

	if (AOUT_GET_FLAGS(hdr) & (AOUT_FLAG_PIC | AOUT_FLAG_DYNAMIC)) {
		fprintf(stderr, "%s: PIC and DYNAMIC are not supported\n", fn);
		goto out;
	}

	/* OK! */
	return fp;

out:	fclose(fp);
	return (FILE *) NULL;
}

/*
 * look at two a.out headers and check if they are compatible
 */
int
check_2_aout_hdrs(h1, h2)
	struct aout_m68k *h1, *h2;
{

	if (/* compare magic */
	    h1->a_magic.hostval != h2->a_magic.hostval ||
	    /* compare section size */
	    h1->a_text.hostval != h2->a_text.hostval ||
	    h1->a_data.hostval != h2->a_data.hostval ||
	    h1->a_bss.hostval != h2->a_bss.hostval)
		return -1;

	return 0;
}

/* allocation unit (in bytes) of relocation table */
#define RELTBL_CHUNK	8192

/*
 * add an entry to the relocation table
 */
#define ADD_RELTBL(adr)	\
	if (relsize + sizeof(struct relinf_l) > relallocsize)		    \
		reltbl = do_realloc(reltbl, relallocsize += RELTBL_CHUNK);  \
	if ((adr) < reladdr + HUX_MINLREL) {				    \
		struct relinf_s *r = (struct relinf_s *)(reltbl + relsize); \
		put_uint16(&r->locoff_s, (unsigned)((adr) - reladdr));	    \
		relsize += sizeof(struct relinf_s);			    \
		DPRINTF(("short"));					    \
	} else {							    \
		struct relinf_l *r = (struct relinf_l *)(reltbl + relsize); \
		put_uint16(&r->lrelmag, HUXLRELMAGIC);			    \
		put_uint32((be_uint32_t *)r->locoff_l, (adr) - reladdr);    \
		relsize += sizeof(struct relinf_l);			    \
		DPRINTF(("long "));					    \
	}								    \
	DPRINTF((" reloc 0x%06x", (adr)));				    \
	reladdr = (adr);

#define ERR1	{ if (ferror(fpa1)) perror(fn1);			\
		  else fprintf(stderr, "%s: unexpected EOF\n", fn1);	\
		  goto out; }
#define ERR2	{ if (ferror(fpa2)) perror(fn2);			\
		  else fprintf(stderr, "%s: unexpected EOF\n", fn2);	\
		  goto out; }
#define ERRC	{ fprintf(stderr, "files %s and %s are inconsistent\n",	\
				  fn1, fn2);				\
		  goto out; }

/*
 * read a.out files and output .x body
 * and create relocation table
 */
#define CREATE_RELOCATION(segsize)	\
	while (segsize > 0 || nbuf) {					\
		if (nbuf == 0) {					\
			if (fread(&b1.half[0], SIZE_16, 1, fpa1) != 1)	\
				ERR1					\
			if (fread(&b2.half[0], SIZE_16, 1, fpa2) != 1)	\
				ERR2					\
			nbuf = 1;					\
			segsize -= SIZE_16;				\
		} else if (nbuf == 1) {					\
			if (segsize == 0) {				\
				if (b1.half[0].hostval != b2.half[0].hostval) \
					ERRC				\
				fwrite(&b1.half[0], SIZE_16, 1, fpx);	\
				nbuf = 0;				\
				addr += SIZE_16;			\
			} else {					\
				if (fread(&b1.half[1], SIZE_16, 1, fpa1) != 1)\
					ERR1				\
				if (fread(&b2.half[1], SIZE_16, 1, fpa2) != 1)\
					ERR2				\
				nbuf = 2;				\
				segsize -= SIZE_16;			\
			}						\
		} else /* if (nbuf == 2) */ {				\
			if (b1.hostval != b2.hostval &&			\
			    get_uint32(&b1) - loadadr1			\
					== get_uint32(&b2) - loadadr2) {\
				/* do relocation */			\
				ADD_RELTBL(addr)			\
									\
				put_uint32(&b1, get_uint32(&b1) - loadadr1);  \
				DPRINTF((" v 0x%08x\t", get_uint32(&b1)));    \
				fwrite(&b1, SIZE_32, 1, fpx);		\
				nbuf = 0;				\
				addr += SIZE_32;			\
			} else if (b1.half[0].hostval == b2.half[0].hostval) {\
				fwrite(&b1.half[0], SIZE_16, 1, fpx);	\
				addr += SIZE_16;			\
				b1.half[0] = b1.half[1];		\
				b2.half[0] = b2.half[1];		\
				nbuf = 1;				\
			} else						\
				ERRC					\
		}							\
	}

int
aout2hux(fn1, fn2, loadadr1, loadadr2, fnx)
	const char *fn1, *fn2, *fnx;
	u_int32_t loadadr1, loadadr2;
{
	int status = 1;			/* the default is "failed" */
	FILE *fpa1 = NULL, *fpa2 = NULL;
	struct aout_m68k hdr1, hdr2;
	FILE *fpx = NULL;
	struct huxhdr xhdr;
	u_int32_t textsize, datasize, pagesize, paddingsize, execoff;

	/* for relocation */
	be_uint32_t b1, b2;
	int nbuf;
	u_int32_t addr;

	/* for relocation table */
	size_t relsize, relallocsize;
	u_int32_t reladdr;
	char *reltbl = NULL;


	/*
	 * check load addresses
	 */
	if (loadadr1 == loadadr2) {
		fprintf(stderr, "two load addresses must be different\n");
		return 1;
	}

	/*
	 * open a.out files and check the headers
	 */
	if (!(fpa1 = open_aout(fn1, &hdr1)) || !(fpa2 = open_aout(fn2, &hdr2)))
		goto out;

	/*
	 * check for consistency
	 */
	if (check_2_aout_hdrs(&hdr1, &hdr2)) {
		fprintf(stderr, "files %s and %s are incompatible\n",
				fn1, fn2);
		goto out;
	}
	/* check entry address */
	if (get_uint32(&hdr1.a_entry) - loadadr1 !=
					get_uint32(&hdr2.a_entry) - loadadr2) {
		fprintf(stderr, "address of %s or %s may be incorrect\n",
				fn1, fn2);
		goto out;
	}

	/*
	 * get information from a.out header
	 */
	textsize = get_uint32(&hdr1.a_text);
	pagesize = AOUT_PAGESIZE(&hdr1);
	paddingsize = ((textsize + pagesize - 1) & ~(pagesize - 1)) - textsize;
	datasize = get_uint32(&hdr1.a_data);
	execoff = get_uint32(&hdr1.a_entry) - loadadr1;

	DPRINTF(("text: %u, data: %u, page: %u, pad: %u, bss: %u, exec: %u\n", 
		textsize, datasize, pagesize, paddingsize,
		get_uint32(&hdr1.a_bss), execoff));

	if (textsize & 1) {
		fprintf(stderr, "text size is not even\n");
		goto out;
	}
	if (datasize & 1) {
		fprintf(stderr, "data size is not even\n");
		goto out;
	}
	if (execoff >= textsize &&
	    (execoff < textsize + paddingsize ||
	     execoff >= textsize + paddingsize + datasize)) {
		fprintf(stderr, "exec addr is not in text or data segment\n");
		goto out;
	}

	/*
	 * prepare for .x header
	 */
	bzero((void *) &xhdr, sizeof xhdr);
	put_uint16(&xhdr.x_magic, HUXMAGIC);
	put_uint32(&xhdr.x_entry, execoff);
	put_uint32(&xhdr.x_text, textsize + paddingsize);
	xhdr.x_data.hostval = hdr1.a_data.hostval;
	xhdr.x_bss.hostval = hdr1.a_bss.hostval;

	/*
	 * create output file
	 */
	if (!(fpx = fopen(fnx, "w")) ||
	    fseek(fpx, (off_t) sizeof xhdr, SEEK_SET)) { /* skip header */
		perror(fnx);
		goto out;
	}

	addr = 0;
	nbuf = 0;

	relsize = relallocsize = 0;
	reladdr = 0;

	/*
	 * text segment
	 */
	CREATE_RELOCATION(textsize)

	/*
	 * page boundary
	 */
	addr += paddingsize;
	while (paddingsize--)
		putc('\0', fpx);

	/*
	 * data segment
	 */
	CREATE_RELOCATION(datasize)

	/*
	 * error check of the above
	 */
	if (ferror(fpx)) {
		fprintf(stderr, "%s: write failure\n", fnx);
		goto out;
	}

	/*
	 * write relocation table
	 */
	if (relsize > 0) {
		DPRINTF(("\n"));
		if (fwrite(reltbl, 1, relsize, fpx) != relsize) {
			perror(fnx);
			goto out;
		}
	}

	/*
	 * write .x header at the top of the output file
	 */
	put_uint32(&xhdr.x_rsize, relsize);
	if (fseek(fpx, (off_t) 0, SEEK_SET) ||
	    fwrite(&xhdr, sizeof xhdr, 1, fpx) != 1) {
		perror(fnx);
		goto out;
	}

	status = 0;	/* all OK */

out:	/*
	 * cleanup
	 */
	if (fpa1)
		fclose(fpa1);
	if (fpa2)
		fclose(fpa2);
	if (fpx) {
		if (fclose(fpx) && status == 0) {
			/* Alas, final flush failed! */
			perror(fnx);
			status = 1;
		}
		if (status)
			remove(fnx);
	}
	if (reltbl)
		free(reltbl);

	return status;
}

#ifndef NO_BIST
void bist PROTO((void));

/*
 * built-in self test
 */
void
bist()
{
	be_uint16_t be16;
	be_uint32_t be32;
	be_uint32_t be32x2[2];

	be16.val[0] = 0x12; be16.val[1] = 0x34;
	be32.val[0] = 0xfe; be32.val[1] = 0xdc;
	be32.val[2] = 0xba; be32.val[3] = 0x98;

	put_uint16(&be32x2[0].half[1], 0x4567);
	put_uint32(&be32x2[1], 0xa9876543);

	if (sizeof(u_int8_t) != 1 || sizeof(u_int16_t) != 2 ||
	    sizeof(u_int32_t) != 4 ||
	    SIZE_16 != 2 || SIZE_32 != 4 || sizeof be32x2 != 8 ||
	    sizeof(struct relinf_s) != 2 || sizeof(struct relinf_l) != 6 ||
	    get_uint16(&be16) != 0x1234 || get_uint32(&be32) != 0xfedcba98 ||
	    get_uint16(&be32x2[0].half[1]) != 0x4567 ||
	    get_uint32(&be32x2[1]) != 0xa9876543) {
		fprintf(stderr, "BIST failed\n");
		exit(1);
	}
}
#endif

int
gethex(pval, str)
	u_int32_t *pval;
	const char *str;
{
	const unsigned char *p = (const unsigned char *) str;
	u_int32_t val;
	int over;

	/* skip leading "0x" if exists */
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
		p += 2;

	if (!*p)
		goto bad;

	for (val = 0, over = 0; *p; p++) {
		int digit;

		switch (*p) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			digit = *p - '0';
			break;
		case 'a': case 'A':	digit = 10; break;
		case 'b': case 'B':	digit = 11; break;
		case 'c': case 'C':	digit = 12; break;
		case 'd': case 'D':	digit = 13; break;
		case 'e': case 'E':	digit = 14; break;
		case 'f': case 'F':	digit = 15; break;
		default:
			goto bad;
		}
		if (val >= 0x10000000 && !over)
			over = 1;
		val = (val << 4) | digit;
	}

	if (over)
		fprintf(stderr, "warning: %s: constant overflow\n", str);

	*pval = val;

	DPRINTF(("gethex: %s -> 0x%x\n", str, val));

	return 0;

bad:
	fprintf(stderr, "%s: not a hexadecimal number\n", str);
	return 1;
}

void
usage(name)
	const char *name;
{

	fprintf(stderr, "\
usage: %s [ -o output.x ] a.out1 loadaddr1 a.out2 loadaddr2\n\n\
The input a.out files must be static OMAGIC/NMAGIC m68k executable.\n\
Two a.out files must have different loading addresses.\n\
Each of the load address must be a hexadecimal number.\n\
The default output filename is \"%s\".\n" ,name, DEFAULT_OUTPUT_FILE);

	exit(1);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	const char *outfile = DEFAULT_OUTPUT_FILE;
	u_int32_t adr1, adr2;

#ifndef NO_BIST
	bist();
#endif

	if (argc > 2 && argv[1][0] == '-' && argv[1][1] == 'o' && !argv[1][2]) {
		outfile = argv[2];
		argv += 2;
		argc -= 2;
	}

	if (argc != 5)
		usage(argv[0]);

	if (gethex(&adr1, argv[2]) || gethex(&adr2, argv[4]))
		usage(argv[0]);

	return aout2hux(argv[1], argv[3], adr1, adr2, outfile);
}
