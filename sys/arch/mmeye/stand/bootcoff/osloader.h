#ifndef	_OSLOADER_H_
#define	_OSLOADER_H_

#include <sys/types.h>

/*
 * COFF file header
 */

struct coff_filehdr {
    u_short	f_magic;	/* magic number */
    u_short	f_nscns;	/* # of sections */
    long	f_timdat;	/* timestamp */
    long	f_symptr;	/* file offset of symbol table */
    long	f_nsyms;	/* # of symbol table entries */
    u_short	f_opthdr;	/* size of optional header */
    u_short	f_flags;	/* flags */
};

/* f_magic flags */
#define COFF_MAGIC_SH3	0x500

/* f_flags */
#define COFF_F_RELFLG	0x1
#define COFF_F_EXEC	0x2
#define COFF_F_LNNO	0x4
#define COFF_F_LSYMS	0x8
#define COFF_F_SWABD	0x40
#define COFF_F_AR16WR	0x80
#define COFF_F_AR32WR	0x100

/*
 * COFF system header
 */

struct coff_aouthdr {
    short	a_magic;
    short	a_vstamp;
    long	a_tsize;
    long	a_dsize;
    long	a_bsize;
    long	a_entry;
    long	a_tstart;
    long	a_dstart;
};

/* magic */
#define COFF_OMAGIC	0444	/* text not write-protected; data seg
				   is contiguous with text */
#define COFF_NMAGIC	0410	/* text is write-protected; data starts
				   at next seg following text */
#define COFF_ZMAGIC	0000	/* text and data segs are aligned for
				   direct paging */
#define COFF_SMAGIC	0443	/* shared lib */

/*
 * COFF section header
 */

struct coff_scnhdr {
    char	s_name[8];
    long	s_paddr;
    long	s_vaddr;
    long	s_size;
    long	s_scnptr;
    long	s_relptr;
    long	s_lnnoptr;
    u_short	s_nreloc;
    u_short	s_nlnno;
    long	s_flags;
};

/* s_flags */
#define COFF_STYP_REG		0x00
#define COFF_STYP_DSECT		0x01
#define COFF_STYP_NOLOAD	0x02
#define COFF_STYP_GROUP		0x04
#define COFF_STYP_PAD		0x08
#define COFF_STYP_COPY		0x10
#define COFF_STYP_TEXT		0x20
#define COFF_STYP_DATA		0x40
#define COFF_STYP_BSS		0x80
#define COFF_STYP_INFO		0x200
#define COFF_STYP_OVER		0x400
#define COFF_STYP_SHLIB		0x800

/*
 * COFF shared library header
 */

struct coff_slhdr {
	long	entry_len;	/* in words */
	long	path_index;	/* in words */
	char	sl_name[1];
};

#define COFF_ROUND(val, by)     (((val) + by - 1) & ~(by - 1))

#define COFF_ALIGN(a) ((a) & ~(COFF_LDPGSZ - 1))

#define COFF_HDR_SIZE \
	(sizeof(struct coff_filehdr) + sizeof(struct coff_aouthdr))

#define COFF_BLOCK_ALIGN(ap, value) \
        (ap->a_magic == COFF_ZMAGIC ? COFF_ROUND(value, COFF_LDPGSZ) : \
         value)

#define COFF_TXTOFF(fp, ap) \
        (ap->a_magic == COFF_ZMAGIC ? 0 : \
         COFF_ROUND(COFF_HDR_SIZE + fp->f_nscns * \
		    sizeof(struct coff_scnhdr), COFF_SEGMENT_ALIGNMENT(ap)))

#define COFF_DATOFF(fp, ap) \
        (COFF_BLOCK_ALIGN(ap, COFF_TXTOFF(fp, ap) + ap->a_tsize))

#define COFF_SEGMENT_ALIGN(ap, value) \
        (COFF_ROUND(value, (ap->a_magic == COFF_ZMAGIC ? COFF_LDPGSZ : \
         COFF_SEGMENT_ALIGNMENT(ap))))

#define COFF_LDPGSZ 4096

#define COFF_SEGMENT_ALIGNMENT(ap) 4

#define COFF_BADMAG(ex) (ex->f_magic != COFF_MAGIC_SH3)

#define IBCS2_HIGH_SYSCALL(n)		(((n) & 0x7f) == 0x28)
#define IBCS2_CVT_HIGH_SYSCALL(n)	(((n) >> 8) + 128)

#ifdef DEBUG_COFF
#define DPRINTF(a)      printf a;
#else
#define DPRINTF(a)
#endif

#endif /* !_OSLOADER_H_ */
