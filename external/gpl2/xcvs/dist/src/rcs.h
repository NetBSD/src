/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * RCS source control definitions needed by rcs.c and friends
 */

/* Strings which indicate a conflict if they occur at the start of a line.  */
#define	RCS_MERGE_PAT_1 "<<<<<<< "
#define	RCS_MERGE_PAT_2 "=======\n"
#define	RCS_MERGE_PAT_3 ">>>>>>> "

#define	RCSEXT		",v"
#define RCSPAT		"*,v"
#define	RCSHEAD		"head"
#define	RCSBRANCH	"branch"
#define	RCSSYMBOLS	"symbols"
#define	RCSDATE		"date"
#define	RCSDESC		"desc"
#define RCSEXPAND	"expand"

/* Used by the version of death support which resulted from old
   versions of CVS (e.g. 1.5 if you define DEATH_SUPPORT and not
   DEATH_STATE).  Only a hacked up RCS (used by those old versions of
   CVS) will put this into RCS files.  Considered obsolete.  */
#define RCSDEAD		"dead"

#define	DATEFORM	"%02d.%02d.%02d.%02d.%02d.%02d"
#define	SDATEFORM	"%d.%d.%d.%d.%d.%d"

/*
 * Opaque structure definitions used by RCS specific lookup routines
 */
#define VALID	0x1			/* flags field contains valid data */
#define	INATTIC	0x2			/* RCS file is located in the Attic */
#define PARTIAL 0x4			/* RCS file not completly parsed */

/* All the "char *" fields in RCSNode, Deltatext, and RCSVers are
   '\0'-terminated (except "text" in Deltatext).  This means that we
   can't deal with fields containing '\0', which is a limitation that
   RCS does not have.  Would be nice to fix this some day.  */

struct rcsnode
{
    /* Reference count for this structure.  Used to deal with the
       fact that there might be a pointer from the Vers_TS or might
       not.  Callers who increment this field are responsible for
       calling freercsnode when they are done with their reference.  */
    int refcount;

    /* Flags (INATTIC, PARTIAL, &c), see above.  */
    int flags;

    /* File name of the RCS file.  This is not necessarily the name
       as specified by the user, but it is a name which can be passed to
       system calls and a name which is OK to print in error messages
       (the various names might differ in case).  */
    char *path;

    /* Use when printing paths.  */
    char *print_path;

    /* Value for head keyword from RCS header, or NULL if empty.  HEAD may only
     * be empty in a valid RCS file when the file has no revisions, a state
     * that should not be able to occur with CVS.
     */
    char *head;

    /* Value for branch keyword from RCS header, or NULL if omitted.  */
    char *branch;

    /* Raw data on symbolic revisions.  The first time that RCS_symbols is
       called, we parse these into ->symbols, and free ->symbols_data.  */
    char *symbols_data;

    /* Value for expand keyword from RCS header, or NULL if omitted.  */
    char *expand;

    /* List of nodes, the key of which is the symbolic name and the data
       of which is the numeric revision that it corresponds to (malloc'd).  */
    List *symbols;

    /* List of nodes (type RCSVERS), the key of which the numeric revision
       number, and the data of which is an RCSVers * for the revision.  */
    List *versions;

    /* Value for access keyword from RCS header, or NULL if empty.
       FIXME: RCS_delaccess would also seem to use "" for empty.  We
       should pick one or the other.  */
    char *access;

    /* Raw data on locked revisions.  The first time that RCS_getlocks is
       called, we parse these into ->locks, and free ->locks_data.  */
    char *locks_data;

    /* List of nodes, the key of which is the numeric revision and the
       data of which is the user that it corresponds to (malloc'd).  */
    List *locks;

    /* Set for the strict keyword from the RCS header.  */
    int strict_locks;

    /* Value for the comment keyword from RCS header (comment leader), or
       NULL if omitted.  */
    char *comment;

    /* Value for the desc field in the RCS file, or NULL if empty.  */
    char *desc;

    /* File offset of the first deltatext node, so we can seek there.  */
    off_t delta_pos;

    /* Newphrases from the RCS header.  List of nodes, the key of which
       is the "id" which introduces the newphrase, and the value of which
       is the value from the newphrase.  */
    List *other;
};

typedef struct rcsnode RCSNode;

struct deltatext {
    char *version;

    /* Log message, or NULL if we do not intend to change the log message
       (that is, RCS_copydeltas should just use the log message from the
       file).  */
    char *log;

    /* Change text, or NULL if we do not intend to change the change text
       (that is, RCS_copydeltas should just use the change text from the
       file).  Note that it is perfectly valid to have log be NULL and
       text non-NULL, or vice-versa.  */
    char *text;
    size_t len;

    /* Newphrase fields from deltatext nodes.  FIXME: duplicates the
       other field in the rcsversnode, I think.  */
    List *other;
};
typedef struct deltatext Deltatext;

struct rcsversnode
{
    /* Duplicate of the key by which this structure is indexed.  */
    char *version;

    char *date;
    char *author;
    char *state;
    char *next;
    int dead;
    int outdated;
    Deltatext *text;
    List *branches;
    /* Newphrase fields from deltatext nodes.  Also contains ";add" and
       ";delete" magic fields (see rcs.c, log.c).  I think this is
       only used by log.c (where it looks up "log").  Duplicates the
       other field in struct deltatext, I think.  */
    List *other;
    /* Newphrase fields from delta nodes.  */
    List *other_delta;
#ifdef PRESERVE_PERMISSIONS_SUPPORT
    /* Hard link information for each revision. */
    List *hardlinks;
#endif
};
typedef struct rcsversnode RCSVers;

/*
 * CVS reserves all even-numbered branches for its own use.  "magic" branches
 * (see rcs.c) are contained as virtual revision numbers (within symbolic
 * tags only) off the RCS_MAGIC_BRANCH, which is 0.  CVS also reserves the
 * ".1" branch for vendor revisions.  So, if you do your own branching, you
 * should limit your use to odd branch numbers starting at 3.
 */
#define	RCS_MAGIC_BRANCH	0

/* The type of a function passed to RCS_checkout.  */
typedef void (*RCSCHECKOUTPROC) (void *, const char *, size_t);

struct rcsbuffer;

/* What RCS_deltas is supposed to do.  */
enum rcs_delta_op {RCS_ANNOTATE, RCS_FETCH};

/*
 * exported interfaces
 */
RCSNode *RCS_parse (const char *file, const char *repos);
RCSNode *RCS_parsercsfile (const char *rcsfile);
void RCS_fully_parse (RCSNode *);
void RCS_reparsercsfile (RCSNode *, FILE **, struct rcsbuffer *);
extern int RCS_setattic (RCSNode *, int);

char *RCS_check_kflag (const char *arg);
char *RCS_getdate (RCSNode * rcs, const char *date, int force_tag_match);
char *RCS_gettag (RCSNode * rcs, const char *symtag, int force_tag_match,
		  int *simple_tag);
int RCS_exist_rev (RCSNode *rcs, char *rev);
int RCS_exist_tag (RCSNode *rcs, char *tag);
char *RCS_tag2rev (RCSNode *rcs, char *tag);
char *RCS_getversion (RCSNode *rcs, const char *tag, const char *date,
		      int force_tag_match, int *simple_tag);
char *RCS_magicrev (RCSNode *rcs, char *rev);
int RCS_isbranch (RCSNode *rcs, const char *rev);
int RCS_nodeisbranch (RCSNode *rcs, const char *tag);
char *RCS_whatbranch (RCSNode *rcs, const char *tag);
char *RCS_head (RCSNode * rcs);
int RCS_datecmp (const char *date1, const char *date2);
time_t RCS_getrevtime (RCSNode * rcs, const char *rev, char *date, int fudge);
List *RCS_symbols (RCSNode *rcs);
void RCS_check_tag (const char *tag);
int RCS_valid_rev (const char *rev);
List *RCS_getlocks (RCSNode *rcs);
void freercsnode (RCSNode ** rnodep);
char *RCS_getbranch (RCSNode *rcs, const char *tag, int force_tag_match);
char *RCS_branch_head (RCSNode *rcs, char *rev);

int RCS_isdead (RCSNode *, const char *);
char *RCS_getexpand (RCSNode *);
void RCS_setexpand (RCSNode *, const char *);
int RCS_checkout (RCSNode *, const char *, const char *, const char *,
                  const char *, const char *, RCSCHECKOUTPROC, void *);
int RCS_checkin (RCSNode *rcs, const char *update_dir, const char *workfile,
		 const char *message, const char *rev, time_t citime,
		 int flags);
int RCS_cmp_file (RCSNode *, const char *, char **, const char *, const char *,
		  const char * );
int RCS_settag (RCSNode *, const char *, const char *);
int RCS_deltag (RCSNode *, const char *);
int RCS_setbranch (RCSNode *, const char *);
int RCS_lock (RCSNode *, const char *, int);
int RCS_unlock (RCSNode *, char *, int);
int RCS_delete_revs (RCSNode *, char *, char *, int);
void RCS_addaccess (RCSNode *, char *);
void RCS_delaccess (RCSNode *, char *);
char *RCS_getaccess (RCSNode *);
void RCS_rewrite (RCSNode *, Deltatext *, char *);
void RCS_abandon (RCSNode *);
int rcs_change_text (const char *, char *, size_t, const char *,
		     size_t, char **, size_t *);
void RCS_deltas (RCSNode *, FILE *, struct rcsbuffer *, const char *,
		 enum rcs_delta_op, char **, size_t *,
		 char **, size_t *);
void RCS_setincexc (void **, const char *arg);
void RCS_setlocalid (const char *, unsigned int, void **, const char *arg);
char *make_file_label (const char *, const char *, RCSNode *);

extern bool preserve_perms;

/* From import.c.  */
extern int add_rcs_file (const char *, const char *, const char *,
                         const char *, const char *, const char *,
                         const char *, int, char **, const char *, size_t,
                         FILE *, bool);
void free_keywords (void *keywords);
