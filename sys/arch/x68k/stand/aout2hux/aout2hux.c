/*
 *	aout2hux - convert a.out/ELF executable to Human68k .x format
 *
 *	Read two a.out/ELF format executables with different load addresses
 *	and generate Human68k .x format executable.
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 * usage:
 *	aout2hux [ -o output.x ] a.out1 loadaddr1 a.out2 loadaddr2
 *
 *	The input files must be static OMAGIC/NMAGIC m68k a.out executables
 *	or m68k ELF executables.
 *	Two executables must have different loading addresses.
 *	Each of the load address must be a hexadecimal number.
 *	Load address shall be multiple of 4 for as / ld of NetBSD/m68k.
 *
 * example:
 *	% cc -N -static -Wl,-Ttext,0        -o aout1 *.o
 *	% cc -N -static -Wl,-Ttext,10203040 -o aout2 *.o
 *	% aout2hux -o foo.x aout1 0 aout2 10203040
 *
 *	$NetBSD: aout2hux.c,v 1.3.14.1 1999/12/27 18:34:17 wrstuden Exp $
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

/* fseek() offset type */
typedef long	foff_t;

#ifndef DEFAULT_OUTPUT_FILE
# define DEFAULT_OUTPUT_FILE	"out.x"
#endif

#ifdef DEBUG
# define DPRINTF(x)	printf x
#else
# define DPRINTF(x)
#endif

struct exec_info {
	foff_t		text_off;	/* file offset of text section */
	foff_t		data_off;	/* file offset of data section */
	u_int32_t	text_size;	/* size of text section */
	u_int32_t	text_pad;	/* pad between text and data */
	u_int32_t	data_size;	/* size of data section */
	u_int32_t	bss_size;	/* size of bss */
	u_int32_t	entry_addr;	/* entry point address */
};

unsigned get_uint16 PROTO((be_uint16_t *be));
u_int32_t get_uint32 PROTO((be_uint32_t *be));
void put_uint16 PROTO((be_uint16_t *be, unsigned v));
void put_uint32 PROTO((be_uint32_t *be, u_int32_t v));
void *do_realloc PROTO((void *p, size_t s));

static int open_aout __P((const char *fn, struct aout_m68k *hdr,
		struct exec_info *inf));
static int open_elf PROTO((const char *fn, FILE *fp, struct elf_m68k_hdr *hdr,
		struct exec_info *inf));
FILE *open_exec PROTO((const char *fn, struct exec_info *inf));
int check_2_exec_inf PROTO((struct exec_info *inf1, struct exec_info *inf2));
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
 * check a.out header
 */
static int
open_aout(fn, hdr, inf)
	const char *fn;
	struct aout_m68k *hdr;
	struct exec_info *inf;
{
	int i;

	DPRINTF(("%s: is an a.out\n", fn));

	if ((i = AOUT_GET_MID(hdr)) != AOUT_MID_M68K && i != AOUT_MID_M68K4K) {
		fprintf(stderr, "%s: wrong architecture (mid %d)\n", fn, i);
		return 1;
	}

	/* if unsolved relocations exist, not an executable but an object */
	if (hdr->a_trsize.hostval || hdr->a_drsize.hostval) {
		fprintf(stderr, "%s: not an executable (object file?)\n", fn);
		return 1;
	}

	if (AOUT_GET_FLAGS(hdr) & (AOUT_FLAG_PIC | AOUT_FLAG_DYNAMIC)) {
		fprintf(stderr, "%s: PIC and DYNAMIC are not supported\n", fn);
		return 1;
	}

	inf->text_size = get_uint32(&hdr->a_text);
	inf->data_size = get_uint32(&hdr->a_data);
	inf->bss_size = get_uint32(&hdr->a_bss);
	inf->entry_addr = get_uint32(&hdr->a_entry);
	inf->text_off = sizeof(struct aout_m68k);
	inf->data_off = sizeof(struct aout_m68k) + inf->text_size;
	inf->text_pad = -inf->text_size & (AOUT_PAGESIZE(hdr) - 1);

	return 0;
}

/*
 * digest ELF structure
 */
static int
open_elf(fn, fp, hdr, inf)
	const char *fn;
	FILE *fp;
	struct elf_m68k_hdr *hdr;
	struct exec_info *inf;
{
	int i;
	size_t nphdr;
	struct elf_m68k_phdr phdr[2];

	DPRINTF(("%s: is an ELF\n", fn));

	if (hdr->e_ident[EI_VERSION] != EV_CURRENT ||
	    get_uint32(&hdr->e_version) != EV_CURRENT) {
		fprintf(stderr, "%s: unknown ELF version\n", fn);
		return 1;
	}

	if (get_uint16(&hdr->e_type) != ET_EXEC) {
		fprintf(stderr, "%s: not an executable\n", fn);
		return 1;
	}

	if ((i = get_uint16(&hdr->e_machine)) != EM_68K) {
		fprintf(stderr, "%s: wrong architecture (%d)\n", fn, i);
		return 1;
	}

	if ((i = get_uint16(&hdr->e_shentsize)) != SIZE_ELF68K_SHDR) {
		fprintf(stderr, "%s: size shdr %d should be %d\n", fn, i,
			SIZE_ELF68K_SHDR);
		return 1;
	}

	if ((i = get_uint16(&hdr->e_phentsize)) != SIZE_ELF68K_PHDR) {
		fprintf(stderr, "%s: size phdr %d should be %d\n", fn, i,
			SIZE_ELF68K_PHDR);
		return 1;
	}

	if ((nphdr = get_uint16(&hdr->e_phnum)) != 1 && nphdr != 2) {
		fprintf(stderr,
			"%s: has %d loadable segments (should be 1 or 2)\n",
			fn, nphdr);
		return 1;
	}

	/* Read ELF program header table. */
	if (fseek(fp, (foff_t) get_uint32(&hdr->e_phoff), SEEK_SET)) {
		perror(fn);
		return 1;
	}
	if (fread(phdr, sizeof phdr[0], nphdr, fp) != nphdr) {
		fprintf(stderr, "%s: can't read ELF program header\n", fn);
		return 1;
	}

	/* Just error checking. */
	for (i = 0; i < (int) nphdr; i++) {
		if (get_uint32(&phdr[i].p_type) != PT_LOAD) {
			fprintf(stderr,
				"%s: program header #%d is not loadable\n",
				fn, i);
			return 1;
		}
	}

	if (nphdr == 1 && (get_uint32(&phdr[0].p_flags) & PF_W)) {
		/*
		 * Only one writable section --- probably "ld -N" executable.
		 * Find out the start of data segment.
		 */
		struct elf_m68k_shdr shdr;
		int nshdr;

		nshdr = get_uint16(&hdr->e_shnum);

		/* section #0 always exists and reserved --- skip */
		if (nshdr > 1 &&
		    fseek(fp,
			  (foff_t) (get_uint32(&hdr->e_shoff) + sizeof shdr),
			  SEEK_SET)) {
			perror(fn);
			return 1;
		}
		for (i = 1; i < nshdr; i++) {
			if (fread(&shdr, sizeof shdr, 1, fp) != 1) {
				fprintf(stderr,
					"%s: can't read ELF section header\n",
					fn);
				return 1;
			}

			DPRINTF(("%s: section header #%d: flags 0x%x\n",
				fn, i, get_uint32(&shdr.sh_flags)));

			if (ELF68K_ISDATASEG(&shdr)) {
				/*
				 * data section is found.
				 */
				DPRINTF(("%s: one section, data found\n", fn));
				inf->text_off = get_uint32(&phdr[0].p_offset);
				inf->text_size = get_uint32(&shdr.sh_offset) -
						 inf->text_off;
				inf->text_pad = 0;
				inf->data_off = inf->text_off + inf->text_size;
				inf->data_size = get_uint32(&phdr[0].p_filesz) -
						 inf->text_size;
				inf->bss_size = get_uint32(&phdr[0].p_memsz) -
						get_uint32(&phdr[0].p_filesz);
				inf->entry_addr = get_uint32(&hdr->e_entry);
				goto data_found;
			}
		}
		/*
		 * No data section found --- probably text + bss.
		 */
		DPRINTF(("%s: one section, no data section\n", fn));
		inf->text_size = get_uint32(&phdr[0].p_filesz);
		inf->data_size = 0;
		inf->bss_size = get_uint32(&phdr[0].p_memsz) - inf->text_size;
		inf->entry_addr = get_uint32(&hdr->e_entry);
		inf->text_off = get_uint32(&phdr[0].p_offset);
		inf->data_off = 0;
		inf->text_pad = 0;
data_found:;
	} else if (nphdr == 1) {
		/*
		 * Only one non-writable section --- pure text program?
		 */
		DPRINTF(("%s: one RO section\n", fn));
		inf->text_size = get_uint32(&phdr[0].p_filesz);
		inf->data_size = 0;
		inf->bss_size = 0;
		inf->entry_addr = get_uint32(&hdr->e_entry);
		inf->text_off = get_uint32(&phdr[0].p_offset);
		inf->data_off = 0;
		inf->text_pad = get_uint32(&phdr[0].p_memsz) - inf->text_size;
	} else {
		/*
		 * two sections
		 * text + data assumed.
		 */
		int t = 0, d = 1, tmp;	/* first guess */
#define SWAP_T_D	tmp = t, t = d, d = tmp

		DPRINTF(("%s: two sections\n", fn));

		/* Find out text and data. */
		if (get_uint32(&phdr[t].p_vaddr) > get_uint32(&phdr[d].p_vaddr))
			SWAP_T_D;

		if ((get_uint32(&phdr[t].p_flags) & PF_X) == 0 &&
		    get_uint32(&phdr[d].p_flags) & PF_X)
			SWAP_T_D;

		if ((get_uint32(&phdr[d].p_flags) & PF_W) == 0 &&
		    get_uint32(&phdr[t].p_flags) & PF_W)
			SWAP_T_D;
#undef SWAP_T_D

		/* Are the text/data sections correctly detected? */
		if (get_uint32(&phdr[t].p_vaddr) >
		    get_uint32(&phdr[d].p_vaddr)) {
			fprintf(stderr, "%s: program sections not in order\n",
				fn);
			return 1;
		}

		if ((get_uint32(&phdr[t].p_flags) & PF_X) == 0)
			fprintf(stderr, "%s: warning: text is not executable\n",
				fn);

		if ((get_uint32(&phdr[d].p_flags) & PF_W) == 0)
			fprintf(stderr, "%s: warning: data is not writable\n",
				fn);

		inf->text_size = get_uint32(&phdr[t].p_filesz);
		inf->data_size = get_uint32(&phdr[d].p_filesz);
		inf->bss_size = get_uint32(&phdr[d].p_memsz) - inf->data_size;
		inf->entry_addr = get_uint32(&hdr->e_entry);
		inf->text_off = get_uint32(&phdr[t].p_offset);
		inf->data_off = get_uint32(&phdr[d].p_offset);
		inf->text_pad = get_uint32(&phdr[d].p_vaddr) -
			(get_uint32(&phdr[t].p_vaddr) + inf->text_size);
	}

	return 0;
}

/*
 * open an executable
 */
FILE *
open_exec(fn, inf)
	const char *fn;
	struct exec_info *inf;
{
	FILE *fp;
	int i;
	union {
		struct aout_m68k	u_aout;
		struct elf_m68k_hdr	u_elf;
	} buf;
#define hdra	(&buf.u_aout)
#define hdre	(&buf.u_elf)

	if (!(fp = fopen(fn, "r"))) {
		perror(fn);
		return (FILE *) NULL;
	}

	/*
	 * Check for a.out.
	 */

	if (fread(hdra, sizeof(struct aout_m68k), 1, fp) != 1) {
		fprintf(stderr, "%s: can't read a.out header\n", fn);
		goto out;
	}

	if ((i = AOUT_GET_MAGIC(hdra)) != AOUT_OMAGIC && i != AOUT_NMAGIC)
		goto notaout;

	if (open_aout(fn, hdra, inf))
		goto out;

	/* OK! */
	return fp;

notaout:
	/*
	 * Check for ELF.
	 */

	if (hdre->e_ident[EI_MAG0] != ELFMAG0 ||
	    hdre->e_ident[EI_MAG1] != ELFMAG1 ||
	    hdre->e_ident[EI_MAG2] != ELFMAG2 ||
	    hdre->e_ident[EI_MAG3] != ELFMAG3 ||
	    hdre->e_ident[EI_CLASS] != ELFCLASS32 ||
	    hdre->e_ident[EI_DATA] != ELFDATA2MSB) {
		fprintf(stderr,
		    "%s: not an OMAGIC or NMAGIC a.out, or a 32bit BE ELF\n",
		    fn);
		goto out;
	}

	/* ELF header is longer than a.out header.  Read the rest. */
	if (fread(hdra + 1,
		  sizeof(struct elf_m68k_hdr) - sizeof(struct aout_m68k),
		  1, fp) != 1) {
		fprintf(stderr, "%s: can't read ELF header\n", fn);
		goto out;
	}

	if (open_elf(fn, fp, hdre, inf))
		goto out;

	/* OK! */
	return fp;

out:	fclose(fp);
	return (FILE *) NULL;
#undef hdra
#undef hdre
}

/*
 * compare two executables and check if they are compatible
 */
int
check_2_exec_inf(inf1, inf2)
	struct exec_info *inf1, *inf2;
{

	if (inf1->text_size != inf2->text_size ||
	    inf1->text_pad != inf2->text_pad ||
	    inf1->data_size != inf2->data_size ||
	    inf1->bss_size != inf2->bss_size)
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
 * read input executables and output .x body
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
	struct exec_info inf1, inf2;
	FILE *fpx = NULL;
	struct huxhdr xhdr;
	u_int32_t textsize, datasize, paddingsize, execoff;

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
	 * open input executables and check them
	 */
	if (!(fpa1 = open_exec(fn1, &inf1)) || !(fpa2 = open_exec(fn2, &inf2)))
		goto out;

	/*
	 * check for consistency
	 */
	if (check_2_exec_inf(&inf1, &inf2)) {
		fprintf(stderr, "files %s and %s are incompatible\n",
				fn1, fn2);
		goto out;
	}
	/* check entry address */
	if (inf1.entry_addr - loadadr1 != inf2.entry_addr - loadadr2) {
		fprintf(stderr, "address of %s or %s may be incorrect\n",
				fn1, fn2);
		goto out;
	}

	/*
	 * get information of the executables
	 */
	textsize = inf1.text_size;
	paddingsize = inf1.text_pad;
	datasize = inf1.data_size;
	execoff = inf1.entry_addr - loadadr1;

	DPRINTF(("text: %u, data: %u, pad: %u, bss: %u, exec: %u\n", 
		textsize, datasize, paddingsize, inf1.bss_size, execoff));

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
	put_uint32(&xhdr.x_data, inf1.data_size);
	put_uint32(&xhdr.x_bss, inf1.bss_size);

	/*
	 * create output file
	 */
	if (!(fpx = fopen(fnx, "w")) ||
	    fseek(fpx, (foff_t) sizeof xhdr, SEEK_SET)) { /* skip header */
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
	if (fseek(fpa1, inf1.text_off, SEEK_SET)) {
		perror(fn1);
		goto out;
	}
	if (fseek(fpa2, inf2.text_off, SEEK_SET)) {
		perror(fn2);
		goto out;
	}
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
	if (fseek(fpa1, inf1.data_off, SEEK_SET)) {
		perror(fn1);
		goto out;
	}
	if (fseek(fpa2, inf2.data_off, SEEK_SET)) {
		perror(fn2);
		goto out;
	}
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
	if (fseek(fpx, (foff_t) 0, SEEK_SET) ||
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
	    SIZE_ELF68K_HDR != 52 || SIZE_ELF68K_SHDR != 40 ||
	    SIZE_ELF68K_PHDR != 32 ||
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
		if (val >= 0x10000000)
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
The input files must be static OMAGIC/NMAGIC m68k a.out executables\n\
or m68k ELF executables.\n\
Two executables must have different loading addresses.\n\
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
