

enum { NM_NEW, NM_NWA, NM_DEL, NM_DLA, NM_NORMAL, };

enum { LINK_DEF, LINK_C }; /* linkage definitions */
extern int elnk;
#define	SCLINK	00020	/* for symtab */

extern int cppdebug;

/* spole is symbol at base, nscur is where we are in the stack. */
extern struct symtab *spole, *nscur;

/* insert a symbol into this something */
#define	INSSYM(sp) (sp->snext = nscur->sup, nscur->sup = sp, sp->sdown = nscur)
#define	POPSYM()   (nscur = nscur->sdown)

/* C++-specific node types */
#define		CONST_CAST	(MAXOP+35)
#define		DYN_CAST	(MAXOP+36)
#define		REINT_CAST	(MAXOP+37)
#define		STATIC_CAST	(MAXOP+38)
#define		NEWKW		(MAXOP+39)
#define		DELETE		(MAXOP+40)
#define		NMLIST		(MAXOP+41)

/* C++-specific symtab types */
#define	CLNAME	(MAXSTCL+1)	/* symtab entry is class */
#define	NSPACE	(MAXSTCL+2)	/* symtab entry is namespace */
 
char *decoratename(struct symtab *sp, int type);
NODE *cxx_new(NODE *p);
NODE *cxx_delete(NODE *p, int del);
void dclns(NODE *attr, char *n);
struct symtab *cxxlookup(NODE *p, int declare);
void cxxsetname(struct symtab *sp);
void cxxmember(struct symtab *sp);
struct symtab *cxxstrvar(struct symtab *so);
struct symtab *cxxdclstr(char *n);
struct symtab *cxxftnfind(NODE *p, int flags);
struct symtab *cxxdeclvar(NODE *p);
void symtree(void);
NODE *cxxrstruct(int soru, NODE *attr, NODE *t, char *tag);
NODE *cxxmatchftn(NODE *, NODE *);
NODE *cxxaddhidden(NODE *, NODE *);
NODE *cxxstructref(NODE *p, int f, char *name);
