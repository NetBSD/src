/*	$NetBSD: llparse.h,v 1.5 2001/06/18 09:57:27 jdolecek Exp $	*/

	/************************************************************
		attributes stack garbage
	************************************************************/

#define LLMAXATTR	512
#define LLMAXDESC	256
#define	LLATTR		/* build an attribute stack */

	/*
	**	attribute stack
	**
	**	AttrStack =	stack of record
	**				values : array of values;
	**				ptr	: index;
	**	end;
	**
	*/

	typedef union llattrib LLattrib;

	extern LLattrib	llattributes[LLMAXATTR];
	extern int	llattrtop;

	extern struct	llattr {
		LLattrib	*llabase; /* ptr into the attr stack (llattributes) */
		int		llaindex;/* # attrs on the stack so far for this prod */
		int		llacnt;/* total # ever to go on for this prod */

		int		lloldtop;/* when popping this prod, restore stack to here ;
						 one attr will remain on the stack (for the lhs) */
	}	llattrdesc[LLMAXDESC];

	extern int	lldescindex;

	/************************************************************
		attributes stack garbage
	************************************************************/

	extern	struct	lltoken {
		short		llterm;		/* token number */
		short		llstate;	/* inserted deleted normal */
		LLattrib	llattrib; 
	} 	lltoken;
	typedef	struct lltoken	LLtoken;

/************************************************************
	constants used in llparse.c
************************************************************/

#define STACKSIZE	500
#define MAXCORR		16

#define	NORMAL		0
#define	DELETE		1
#define	INSERT		2

/************************************************************
	datatypes used to communicate with the parser
************************************************************/

struct	llinsert {
	short	llinscost;
	short	llinslength;
	short	llinsert[MAXCORR];
};
typedef	struct llinsert	LLinsert;

extern	short	llparsestack[];
extern	short	llstackptr;
extern	short	llinfinite;

/************************************************************
	variables used to pass information
	specific to each grammer
************************************************************/

extern	short	llnterms;
extern	short	llnsyms;
extern	short	llnprods;

extern	char	*llefile;

extern	struct	llparsetable {
	short	llterm;
	short	llprod;
}	llparsetable[];

extern	short	llparseindex[];

extern	short	llepsilon[];

extern	short	llproductions[];

extern	struct	llprodindex {
	short	llprodstart;
	short	llprodlength;
	short	llprodtlen;
}	llprodindex[];

extern	struct	llcosts {
	short	llinsert;
	short	lldelete;
}	llcosts[];

extern	struct	llstable {
	short	llsstart;
	short	llslength;
}	llstable[];

extern	short	llsspace[];

extern	struct	lletable {
	short	llecost;
	short	llelength;
	short	llestart;
}	lletable[];

extern	long	lleindex[];

extern	short	llespace[];

extern	char	*llstrings[];

/************************************************************
	routines defined in llparse.c
************************************************************/

extern int llparse();
extern void llcopye();
extern void llcopys();
extern void llcorrector();
extern int llepsilonok();
extern void llexpand();
extern short llfindaction();
extern void llgetprefix();
extern void llgettoken();
extern void llinsert();
extern void llinsertsym();
extern void llinserttokens();
extern void llparsererror();
extern void llpushprod();
extern void llreadetab();
extern void llaction();
extern void llpushattr();
extern void llsetattr();
extern void llfinprod();

/************************************************************
	routines defined in llscan.c
************************************************************/
extern void llscan();
extern void dump_buffer();
extern void llaccept();
