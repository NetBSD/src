/* $NetBSD: dzkbdvar.h,v 1.1.2.3 2001/03/12 13:30:02 bouyer Exp $ */

struct dzkm_attach_args {
	int	daa_line;	/* Line to search */
	int	daa_flags;	/* Console etc...*/
#define	DZKBD_CONSOLE	1
};



/* These functions must be present for the keyboard/mouse to work */
int dzgetc(struct dz_linestate *);
void dzputc(struct dz_linestate *, int);
void dzsetlpr(struct dz_linestate *, int);

/* Exported functions */
int dzkbd_cnattach(struct dz_linestate *);
