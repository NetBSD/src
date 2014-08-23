/*	$NetBSD: suff.c,v 1.71 2014/08/23 15:05:40 christos Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef MAKE_NATIVE
static char rcsid[] = "$NetBSD: suff.c,v 1.71 2014/08/23 15:05:40 christos Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)suff.c	8.4 (Berkeley) 3/21/94";
#else
__RCSID("$NetBSD: suff.c,v 1.71 2014/08/23 15:05:40 christos Exp $");
#endif
#endif /* not lint */
#endif

/*-
 * suff.c --
 *	Functions to maintain suffix lists and find implicit dependents
 *	using suffix transformation rules
 *
 * Interface:
 *	Suff_Init 	    	Initialize all things to do with suffixes.
 *
 *	Suff_End 	    	Clean up the module.  Must be called after
 *				Targ_End() so suffix references in nodes
 *				have been dealt with.
 *
 *	Suff_UnsetSuffix	Helper for Targ_End() to unset node's suffix
 *				so as to not mess up reference counting.
 *
 *	Suff_DoPaths	    	This function is used to make life easier
 *	    	  	    	when searching for a file according to its
 *	    	  	    	suffix. It takes the global search path,
 *	    	  	    	as defined using the .PATH: target, and appends
 *	    	  	    	its directories to the path of each of the
 *	    	  	    	defined suffixes, as specified using
 *	    	  	    	.PATH<suffix>: targets. In addition, all
 *	    	  	    	directories given for suffixes labeled as
 *	    	  	    	include files or libraries, using the .INCLUDES
 *	    	  	    	or .LIBS targets, are played with using
 *	    	  	    	Dir_MakeFlags to create the .INCLUDES and
 *	    	  	    	.LIBS global variables.
 *
 *	Suff_ClearSuffixes  	Clear out all the suffixes and defined
 *	    	  	    	transformations.
 *
 *	Suff_IsTransform    	Return TRUE if the passed string is the lhs
 *	    	  	    	of a transformation rule.
 *
 *	Suff_AddSuffix	    	Add the passed string as another known suffix.
 *
 *	Suff_GetPath	    	Return the search path for the given suffix.
 *
 *	Suff_AddInclude	    	Mark the given suffix as denoting an include
 *	    	  	    	file.
 *
 *	Suff_AddLib	    	Mark the given suffix as denoting a library.
 *
 *	Suff_AddTransform   	Add another transformation to the suffix
 *	    	  	    	graph. Returns  GNode suitable for framing, I
 *	    	  	    	mean, tacking commands, attributes, etc. on.
 *
 *	Suff_FindDeps	    	Find implicit sources for and the location of
 *	    	  	    	a target based on its suffix. Returns the
 *	    	  	    	bottom-most node added to the graph or NULL
 *	    	  	    	if the target had no implicit sources.
 *
 *	Suff_FindPath	    	Return the appropriate path to search in
 *				order to find the node.
 */

#include	  <assert.h>
#include	  <limits.h>
#include	  <stdarg.h>
#include    	  <stdio.h>
#include	  "make.h"
#include	  "hash.h"
#include	  "dir.h"

/*
 * Currently known suffixes:
 * All currently known suffixes are stored in sufflist (data: Suff *) and
 * the next suffix added will get sNum as its suffix number (incremented
 * after each use).   Lookup is used as a lookup table for quick rejection,
 * when it needs to be known if a string matches a known suffix.  All indexes
 * corresponding to the first character of a suffix in sufflist are true
 * and others are false.
 */
static int	sNum = 0;
static Lst	sufflist;
#define LOOKUP_SIZE 256
static int	lookup[LOOKUP_SIZE] = { 0 };

/*
 * Currently known transformations:
 * References to nodes of currently active transformation rules (i.e. all
 * which have OP_TRANSFORM set).  It is used to remove the need to iterate
 * through all targets when a transformation node needs to be accessed.
 * (data: GNode *)
 */
static Lst       transforms;

/*
 * Structure describing an individual suffix.
 *
 * When a target has a known suffix, a reference is stored in the suffix
 * field of its node.  The same field is also used in OP_TRANSFORM nodes to
 * store the /target/ of the transformation.  This enables SuffScanTargets()
 * to recognize single suffix transformations by the fact that the field
 * references emptySuff.  This also happens to be the known suffix of
 * the field if the node is used as a regular target.
 *
 * Use the provided maintenance functions for creating, destroying and moving
 * these around to keep the reference counts in order.  The suffix will be
 * free'd when the count reaches zero.
 *
 * The SUFF_NULL type flag is required for SuffAddLevelForSuffix() to
 * detect when a suffix is used as a regular suffix and when as the .NULL
 * suffix.  This is achieved by only setting it for the duration of
 * the call when the "remove a suffix" aspect of .NULL is used.  When
 * the .NULL feature is used in its "add a suffix" aspect, the transformation
 * from a suffixless file is added after the regular transformation by
 * SuffNewChildSrc() when a suffix == nullSuff is used.
 */
typedef struct _Suff {
    char	*name;		/* The suffix itself (e.g. ".c") */
    size_t	nameLen;	/* Length of the suffix */

    short	flags;		/* Filetype implied by the suffix (bitfield) */
#define SUFF_INCLUDE	  0x01		/* One which is #include'd.
					 * XXX: Not used?  Remove? */
#define SUFF_LIBRARY	  0x02		/* ar(1) archive */
#define SUFF_NULL	  0x04		/* The .NULL suffix */

    Lst		searchPath;	/* The path along which files of this suffix
				 * may be found */

    /*
     * The suffix number.  Unique for each suffix.  Doubles as suffix
     * priority, i.e. shows the relative .SUFFIXES ordering of suffixes:
     * smaller sNums are earlier in the list.
     */
    int		sNum;

    /*
     * Reference count.  Counted references are: membership in sufflist,
     * membership in parents or children list of another suffix, assignment
     * to GNode->suffix, assignment as emptySuff, and assignment as nullSuff.
     */
    int		refCount;

    /*
     * Suffixes we have a transformation to (parents) and from (children).
     * Kept in .SUFFIXES order, as implied by sNum (data: Suff *)
     * XXX: renaming these to "to" and "from" or "targets" and "sources"
     * could make sense as parents and children on a suffix are not very
     * intuitive.
     */
    Lst		parents;
    Lst		children;

#ifdef DEBUG_SRC
    /*
     * Lists this suffix is referenced in.  Contains sufflist for all but
     * emptySuff, parents list of all suffixes referenced in own children
     * list, and children list of all suffixes referenced in own parents
     * list.  (data: Lst)
     */
    Lst		ref;
#endif
} Suff;

/*
 * for SuffSuffIsSuffix
 */
typedef struct {
    char	*ename;		/* The end of the name (the final '\0') */
    size_t	len;		/* The length of the name */
} SuffixCmpData;

/*
 * Structure used in the search for implied sources.
 */
typedef struct _Src {
    char            *file;	/* The file to look for */

    /*
     * Prefix from which file was formed.  In an effort to reduce
     * unnecessary memory allocations, only those with parent != NULL
     * own their pref, others refer to their parents'.
     */
    char    	    *pref;

    Suff            *suff;	/* The suffix on the file */
    struct _Src     *parent;	/* The Src for which this is a source */
    GNode           *node;	/* The node describing the file */

    /*
     * Steps to top of chain.  Used in debug prints to show chain length
     * with indentation so multi-stage transformations are more readable.
     */
    int		    depth;
} Src;

/* Extra arguments to SuffAddSrc() via Lst_ForEach(). */
typedef struct {
    Src	*target;	/* get possible transformations for this target */
    Lst	possible;	/* list of possible transformation not yet tried */
    Lst	cleanup;	/* list of all transformations created (for freeing) */
} LstSrc;

/*
 * Empty suffix for implementing POSIX single-suffix transformation rules.
 * It won't go in sufflist lest all targets consider it as their suffix.
 * Also, single suffix rules must only be checked if the target does not
 * have a known suffix.  This suffix is always the first one created, but
 * its sNum field is forced to INT_MAX to keep it in its proper place
 * at the end of the parents list of any of its children.  (Hopefully no one
 * will create that many suffixes)
 */
static Suff 	    *emptySuff = NULL;

/*
 * XXX: replace this ugly hack of a feature with pattern rules!
 */
static Suff	    *nullSuff = NULL;

/* Flags for SuffCleanUp() */
#define SCU_CLEAR	(1 << 1)	/* called from Suff_ClearSuffixes() */
#define SCU_END		(1 << 2)	/* called from Suff_End() */
/* Flags for SuffApplyTransformation*() */
#define SAT_REGULAR	(1 << 0)	/* Standard behavior */
#define SAT_NO_EXPAND	(1 << 1)	/* Don't expand children of top node */

/* Definitions (and map) for local functions. */

/* Lst Predicates */
static const char *SuffStrIsPrefix(const char *, const char *);
static char *SuffSuffIsSuffix(const Suff *, const SuffixCmpData *);
static int SuffSuffIsSuffixP(const void *, const void *);
static int SuffSuffHasNameP(const void *, const void *);
static int SuffSuffIsPrefixP(const void *, const void *);
static int SuffGNHasNameP(const void *, const void *);
static int SuffSuffHasPriorityLEP(const void *, const void *);

/* Maintenance Functions */
static Suff *SuffNewSuff(const char *);
static void SuffFreeSuff(Suff *);
static Src *SuffNewSrc(char *, char *, Suff *, GNode *, Src *);
static Src *SuffNewTopSrc(GNode *, Suff *);
static Src *SuffNewChildSrc(Src *, Suff *, Src **);
static void SuffFreeSrc(void *);
static void SuffLinkSuffixes(Suff *, Suff *);
static int SuffUnlinkSuffixes(void *f, void *t);
static void SuffSetSuffix(GNode *, Suff *);
static void Suff_UnsetSuffix(GNode *);
static void SuffAddToList(Suff *, Lst, LstNode);
static void SuffRemoveFromList(Suff *, Lst);
static void SuffInsertIntoList(Suff *, Lst);
static void SuffUnsetOpTransform(void *);
static int SuffUnlinkChildren(void *, void *);
static void SuffFreeSuffSufflist(void *);
static void SuffCleanUp(int);

/* Parsing Helpers */
/* Suff_ClearSuffixes */
static Boolean SuffParseTransform(char *, Suff **, Suff **);
/* Suff_IsTransform */
/* Suff_AddTransform */
/* Suff_EndTransform */
static int SuffScanTargets(void *, void *);
/* Suff_AddSuffix */
/* Suff_GetPath */
/* Suff_DoPaths */
/* Suff_AddInclude */
/* Suff_AddLib */

/* Implicit Source Search Functions */
static int SuffAddSrc(void *, void *);
static void SuffAddLevel(Lst, Src *, Lst);
static Src *SuffFindThem(Lst, Lst);
static void SuffExpandChildren(GNode *, LstNode);
static void SuffExpandChild(LstNode, GNode *);
static void SuffExpandWildcards(LstNode, GNode *);
/* Suff_FindPath */
static Suff *SuffApplyTransformations(Src *, GNode *);
static Boolean SuffApplyTransformation(GNode *, GNode *, Suff *, Suff *, int);
static Suff *SuffFirstKnownSuffix(char* name);
static void SuffSetPrefixLocalVar(Suff *suffix, char *name, GNode *node);
static void SuffFindArchiveDeps(GNode *, Lst);
static void SuffFindNormalDeps(GNode *, Lst);
/* Suff_FindDeps */
/* Suff_SetNull */
/* Suff_Init */
/* Suff_End */

/* Debugging Functions */
static int SuffDebug(const char *,...) MAKE_ATTR_PRINTFLIKE(1, 2);
static void SuffDebugChain(Src *);
static int SuffPrintName(void *, void *);
static int SuffPrintSuff(void *, void *);
static int SuffPrintTrans(void *, void *);


/*
 ******************************************************************************
 *			Lst Predicates
 ******************************************************************************
 */

/*-
 *-----------------------------------------------------------------------
 * SuffStrIsPrefix  --
 *	See if pref is a prefix of str.
 *
 * Input:
 *	pref		possible prefix
 *	str		string to check
 *
 * Results:
 *	NULL if it ain't, pointer to character in str after prefix if so
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
static const char *
SuffStrIsPrefix(const char *pref, const char *str)
{
    while (*str && *pref == *str) {
	pref++;
	str++;
    }

    return (*pref ? NULL : str);
}

/*-
 *-----------------------------------------------------------------------
 * SuffSuffIsSuffix  --
 *	See if suff is a suffix of str. sd->ename should point to THE END
 *	of the string to check. (THE END == the null byte)
 *
 * Input:
 *	s		possible suffix
 *	sd		string to examine
 *
 * Results:
 *	NULL if it ain't, pointer to character in str before suffix if
 *	it is.
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
static char *
SuffSuffIsSuffix(const Suff *s, const SuffixCmpData *sd)
{
    char  *p1;	    	/* Pointer into suffix name */
    char  *p2;	    	/* Pointer into string being examined */

    if (sd->len < s->nameLen)
	return NULL;		/* this string is shorter than the suffix */

    p1 = s->name + s->nameLen;
    p2 = sd->ename;

    while (p1 >= s->name && *p1 == *p2) {
	p1--;
	p2--;
    }

    return (p1 == s->name - 1 ? p2 : NULL);
}

/*-
 *-----------------------------------------------------------------------
 * SuffSuffIsSuffixP --
 *	Predicate form of SuffSuffIsSuffix. Passed as the callback function
 *	to Lst_Find.
 *
 * Results:
 *	0 if the suffix is the one desired, non-zero if not.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static int
SuffSuffIsSuffixP(const void *s, const void *sd)
{
    return(!SuffSuffIsSuffix(s, sd));
}

/*-
 *-----------------------------------------------------------------------
 * SuffSuffHasNameP --
 *	Callback procedure for finding a suffix based on its name. Used by
 *	Suff_GetPath.
 *
 * Input:
 *	s		Suffix to check
 *	sd		Desired name
 *
 * Results:
 *	0 if the suffix is of the given name. non-zero otherwise.
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
static int
SuffSuffHasNameP(const void *s, const void *sname)
{
    return (strcmp(sname, ((const Suff *)s)->name));
}

/*-
 *-----------------------------------------------------------------------
 * SuffSuffIsPrefixP -- see if str starts with s->name.
 *
 *	Care must be taken when using this to search for transformations and
 *	what-not, since there could well be two suffixes, one of which
 *	is a prefix of the other...
 *
 * Input:
 *	s		suffix to compare
 *	str		string to examine
 *
 * Results:
 *	0 if s is a prefix of str. non-zero otherwise
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
static int
SuffSuffIsPrefixP(const void *s, const void *str)
{
    return SuffStrIsPrefix(((const Suff *)s)->name, str) == NULL;
}

/*-
 *-----------------------------------------------------------------------
 * SuffGNHasNameP  --
 *	See if the graph node has the desired name
 *
 * Input:
 *	gn		current node we're looking at
 *	name		name we're looking for
 *
 * Results:
 *	0 if it does. non-zero if it doesn't
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
static int
SuffGNHasNameP(const void *gn, const void *name)
{
    return (strcmp(name, ((const GNode *)gn)->name));
}

/*-
 *-----------------------------------------------------------------------
 * SuffSuffHasPriorityLEP -- (Less Than or Equal To [Predicate])
 *
 * Check if the second suffix should preceed the first one.  This is
 * a predicate function for Lst_Find(), used by SuffInsertIntoList()
 * for finding the insertion position of the new suffix.
 *
 * Input:
 * 	first	The first suffix. (Suff *)
 * 	second	The second suffix. (Suff *)
 *
 * Returns:
 * 	Return 0 if the first suffix has a priority less than or
 * 	equal to the second one, i.e. the second one preceeds the first
 * 	one in the .SUFFIXES list or they are in fact the same suffix.
 * 	Otherwise non-zero is returned.
 *-----------------------------------------------------------------------
 */
static int
SuffSuffHasPriorityLEP(const void *first, const void* second)
{
    return !(((const Suff *)first)->sNum >= ((const Suff *)second)->sNum);
}


/*
 ******************************************************************************
 *			Maintenance Functions
 ******************************************************************************
 */

/*
 *-----------------------------------------------------------------------
 * SuffNewSuff -- allocate and initialize a Suff structure.
 *
 * The contained lists are initialized and empty.  No flags are set and
 * the reference count is zero.  The next serial number is allocated
 * for this struct.
 *
 * Input:
 * 	name	The suffix this structure describes (e.g. ".c").  A copy
 * 		is made of this string.
 *
 * Results:
 * 	The allocated and initialized structure.
 *-----------------------------------------------------------------------
 */
static Suff*
SuffNewSuff(const char* name)
{
    Suff *s = bmake_malloc(sizeof(Suff));

    s->name =		bmake_strdup(name);
    s->nameLen =	strlen(name);
    s->flags =		(strcmp(name, LIBSUFF) == 0 ? SUFF_LIBRARY : 0);
    s->searchPath =	Lst_Init(FALSE);
    s->sNum =		sNum++;
    s->refCount =	0;
    s->children =	Lst_Init(FALSE);
    s->parents =	Lst_Init(FALSE);
#ifdef DEBUG_SRC
    s->ref =		Lst_Init(FALSE);
#endif

    return s;
}

/*
 *-----------------------------------------------------------------------
 * SuffFreeSuff -- release resources held by a Suff structure.
 *
 * There are no other resources than memory, which is free'd.
 *
 * Input:
 * 	s	the struct to free
 *-----------------------------------------------------------------------
 */
static void
SuffFreeSuff(Suff *s)
{
    /* Make reference counting bugs unpleasantly obvious. */
    if (s->refCount != 0)
	Punt("Internal error deleting suffix `%s' with reference count %d",
	    s->name, s->refCount);

    free(s->name);
    Lst_Destroy(s->searchPath, Dir_Destroy);
    Lst_Destroy(s->parents, NULL);
    Lst_Destroy(s->children, NULL);
#ifdef DEBUG_SRC
    Lst_Destroy(s->ref, NULL);
#endif
    free(s);
}

/*-
 *-----------------------------------------------------------------------
 * SuffNewSrc -- allocate and initialize a new Src structure.
 *
 * The depth field is set to 0 if parent is NULL and to parent->depth + 1
 * otherwise.
 *
 * You probably want to use the convenience functions SuffNewTopSrc() and
 * SuffNewChildSrc().  These convenience functions take the relevant
 * information from their parameters, making copies as needed.
 *
 * Input:
 *	file, pref, suffix, node, parent
 *		Contents of the corresponding fields.  No copies are
 *		made, they are used as they are!
 *
 * Results:
 *	The allocated and initialized structure.
 *-----------------------------------------------------------------------
 */
static Src*
SuffNewSrc(char* file, char* pref, Suff *suffix, GNode *node, Src *parent)
{
    Src *s = bmake_malloc(sizeof(Src));

    s->file = file;
    s->pref = pref;

    s->suff = suffix;

    s->parent = parent;
    s->depth =	0;
    if (parent != NULL)
	s->depth = parent->depth + 1;

    s->node = node;

    return s;
}

/*-
 *-----------------------------------------------------------------------
 * SuffNewTopSrc -- create a new Src for node->name, using suffix.
 *-----------------------------------------------------------------------
 */
static Src*
SuffNewTopSrc(GNode *node, Suff *suffix)
{
    return SuffNewSrc(
	bmake_strdup(node->name),
	bmake_strndup(node->name, strlen(node->name) - suffix->nameLen),
	suffix,
	node,
	NULL);
}

/*-
 *-----------------------------------------------------------------------
 * SuffNewTopSrc -- create a new Src for node->name, faking suffix as "".
 *-----------------------------------------------------------------------
 */
static Src*
SuffNewTopSrcNull(GNode *node, Suff *suffix)
{
    return SuffNewSrc(
	bmake_strdup(node->name),
	bmake_strdup(node->name),
	suffix,
	node,
	NULL);
}

/*-
 *-----------------------------------------------------------------------
 * SuffNewChildSrc -- create a new Src for (parent->pref + suffix->name).
 *
 * Input:
 *	nullChild	for storing the .NULL child
 *
 * Side Effects:
 *	If suffix == nullSuff, a possible source is created for parent->pref
 *	also and stored into nullChild.  Otherwise nullChild is set to NULL.
 *-----------------------------------------------------------------------
 */
static Src*
SuffNewChildSrc(Src *parent, Suff *suffix, Src **nullChild)
{
    if (suffix == nullSuff)
	*nullChild = SuffNewSrc(
	    bmake_strdup(parent->pref),
	    parent->pref,
	    suffix,
	    NULL,
	    parent);
    else
	*nullChild = NULL;

    return SuffNewSrc(
	str_concat(parent->pref, suffix->name, 0),
	parent->pref,
	suffix,
	NULL,
	parent);
}

/*
 *-----------------------------------------------------------------------
 * SuffFreeSrc -- release resources held by an Src structure.
 *
 * There are no other resources than memory, which is free'd.  The signature
 * is chosen so as to be able to call this from Lst_Destroy().
 *
 * Input:
 * 	sp	the struct to free
 *-----------------------------------------------------------------------
 */
static void
SuffFreeSrc(void *sp)
{
    Src *s = (Src *)sp;

    free(s->file);
    if (s->parent == NULL)
	free(s->pref);
    free(s);
}

/*-
 *-----------------------------------------------------------------------
 * SuffLinkSuffixes -- relate two suffixes.
 *
 * Makes "from" aware that "to" is a possible target and "to" aware that
 * "from" is a possible source.  Proper .SUFFIXES ordering is maintained.
 *
 * Pre-condition:
 *	The suffixes are not already linked.
 *
 * Input:
 * 	from	source file suffix
 * 	to	target file suffix
 *-----------------------------------------------------------------------
 */
static void
SuffLinkSuffixes(Suff *from, Suff *to)
{
    SuffDebug("Defining a transformation from `%s' to `%s'.\n",
	    from->name, to->name);

    SuffInsertIntoList(to, from->parents);
    SuffInsertIntoList(from, to->children);
}

/*-
 *-----------------------------------------------------------------------
 * SuffUnlinkSuffixes -- undo what SuffLinkSuffixes() did.
 *-----------------------------------------------------------------------
 */
static int
SuffUnlinkSuffixes(void *f, void *t)
{
    Suff *from = (Suff *)f;
    Suff *to = (Suff *)t;

    SuffDebug("Undefining a transformation from `%s' to `%s'.\n",
	from->name, to->name);

    SuffRemoveFromList(to, from->parents);
    SuffRemoveFromList(from, to->children);
    /* The suffixes are still in sufflist, refCount must be > 0. */
    assert(to->refCount > 0);
    assert(from->refCount > 0);

    return 0;
}

/*-
 *-----------------------------------------------------------------------
 * SuffSetSuffix -- set a new suffix for node.
 *
 * The old suffix, if any, is removed by calling Suff_UnsetSuffix().
 *
 * Input:
 * 	node	node to set the suffix for
 * 	suffix	new suffix for the node
 *
 * Side Effects:
 * 	OP_LIB is set / unset on node if SUFF_LIBRARY is set / unset on
 * 	suffix.
 *-----------------------------------------------------------------------
 */
static void
SuffSetSuffix(GNode *node, Suff *suffix)
{
    Suff_UnsetSuffix(node);

    node->suffix = suffix;

    if (suffix != NULL) {
	suffix->refCount++;
	if (suffix->flags & SUFF_LIBRARY)
	    node->type |= OP_LIB;
    }
}

/*-
 *-----------------------------------------------------------------------
 * Suff_UnsetSuffix -- remove the current suffix of a node.
 *
 * Input:
 *	node	Node whose suffix is to be cleared.
 *
 * Side Effects:
 *	OP_LIB is cleared from node if it was set.  The library status
 *	of nodes stems from their suffixes, so it can't be a library
 *	if it has no suffix.
 *-----------------------------------------------------------------------
 */
static void
Suff_UnsetSuffix(GNode *node)
{
    if (node->suffix != NULL) {
	--node->suffix->refCount;
	/* Suffix is still in sufflist, so refCount must be > 0 */
	assert(node->suffix->refCount > 0);

	node->type &= ~OP_LIB;
	node->suffix = NULL;
    }
}

/*-
 *-----------------------------------------------------------------------
 * SuffAddToList -- add a suffix to a suffix list.
 *
 * Input:
 *	suffix	suffix to add
 *	list	list to receive the suffix
 *	succ	The intended successor of the suffix in the list.
 *		NULL means no successor, i.e. append.
 *-----------------------------------------------------------------------
 */
static void
SuffAddToList(Suff *suffix, Lst list, LstNode succ)
{
    if (list != sufflist)
	SuffDebug("\t`%s'(%d) ", suffix->name, suffix->sNum);

    if (succ == NULL) {
	(void)Lst_AtEnd(list, suffix);
	if (list != sufflist)
	    SuffDebug("inserted at the end of the list.\n");
    } else {
	Suff *s = (Suff *)Lst_Datum(succ);
	(void)Lst_InsertBefore(list, succ, suffix);
	if (list != sufflist)
	    SuffDebug("inserted before `%s'(%d).\n", s->name, s->sNum);
    }
    ++suffix->refCount;

#ifdef DEBUG_SRC
    (void)Lst_AtEnd(suffix->ref, list);
#endif
}

/*-
 *-----------------------------------------------------------------------
 * SuffRemoveFromList -- remove a suffix from a suffix list.
 *
 * The reference count of the suffix might be zero afterwards.  It is
 * the responsibility of the caller to handle this.
 *
 * Input:
 * 	suffix	suffix to remove
 * 	list	list to remove the suffix from
 *-----------------------------------------------------------------------
 */
static void
SuffRemoveFromList(Suff *suffix, Lst list)
{
    if (Lst_Remove(list, Lst_Member(list, suffix)) == SUCCESS) {
	suffix->refCount--;

#ifdef DEBUG_SRC
	Lst_Remove(suffix->ref, Lst_Member(suffix->ref, list));
#endif
    }
}

/*-
 *-----------------------------------------------------------------------
 * SuffInsertIntoList  -- insert a suffix into its proper place in a list.
 *
 * The suffixes in the list are kept ordered by suffix numbers (sNum).
 *
 * Pre-condition:
 *	suffix is not already in the list.
 *
 * Input:
 * 	suffix	suffix to add
 * 	list	list to receive the suffix
 *-----------------------------------------------------------------------
 */
static void
SuffInsertIntoList(Suff *suffix, Lst list)
{
    LstNode successor;

    successor = Lst_Find(list, suffix, SuffSuffHasPriorityLEP);

    if (successor == NULL || ((Suff *)Lst_Datum(successor)) != suffix)
	SuffAddToList(suffix, list, successor);
    else
	/*
	 * This function is only used by SuffLinkSuffixes(), which in turn
	 * should only be called to link previously unlinked suffixes.
	 */
	Punt("Internal error: tried to insert duplicate suffix `%s'(%d) "
	    "into a list.\n", suffix->name, suffix->sNum);
}

/*
 *-----------------------------------------------------------------------
 * SuffUnsetOpTransform -- clear the OP_TRANSFORM flag from a target.
 *
 * Input:
 *	target	The target node whose type is modified. (GNode *)
 *
 * Side Effects:
 *	The suffix of the node is unset.
 *-----------------------------------------------------------------------
 */
static void
SuffUnsetOpTransform(void *tp)
{
    GNode *target = (GNode *)tp;
    Suff_UnsetSuffix(target);
    target->type &= ~OP_TRANSFORM;
}

/*
 *-----------------------------------------------------------------------
 * SuffUnlinkChildren --  SuffUnlinkSuffixes(child, suffix) with all chidren.
 *
 * Calling this function for all suffixes undoes all links between suffixes
 * and this is what Suff_ClearSuffixes() uses it for.
 *
 * Input:
 *	suff	undo links to children for this suffix (Suff *)
 *-----------------------------------------------------------------------
 */
static int
SuffUnlinkChildren(void *suff, void *unused)
{
    Suff *suffix = (Suff *)suff;
    (void)unused;
    Lst_ForEach(suffix->children, SuffUnlinkSuffixes, suffix);

    return 0;
}

/*
 *-----------------------------------------------------------------------
 * SuffFreeSuffSufflist -- frontend to SuffFreeSuff() for destroying sufflist.
 *
 * Reduce the reference count of the suffix by one because it is
 * no longer in sufflist, preventing SuffFreeSuff() from throwing a fit.
 *
 * Input:
 *	suff	suffix to free
 *-----------------------------------------------------------------------
 */
static void
SuffFreeSuffSufflist(void *suff)
{
    Suff *suffix = (Suff *)suff;
    --suffix->refCount;
    SuffFreeSuff(suffix);
}

/*
 *-----------------------------------------------------------------------
 * SuffCleanUp -- uninitialize the module.
 *
 * Clears OP_TRANSFORM from transformation rules and deletes all suffixes.
 *
 * This is a separate function instead of being built in to Suff_End()
 * because coincidentally this exact same functionality is needed
 * by Suff_ClearSuffixes().
 *
 * Input:
 *	flags	tweak behavior
 *		SCU_CLEAR
 *			regular behavior
 *		SCU_END
 *			do not clear transforms, they've been deleted
 *			already by Targ_End()
 *-----------------------------------------------------------------------
 */
static void
SuffCleanUp(int flags)
{
    /*
     * Undo all references between the suffixes before deleting them
     * so SuffFreeSuff() can catch any bugs we might have with reference
     * counting.
     */

    Lst_Destroy(transforms,
	((flags & SCU_CLEAR) ? SuffUnsetOpTransform : NULL));

    if (nullSuff != NULL) {
	--nullSuff->refCount;
	nullSuff = NULL;
    }

    SuffUnlinkChildren(emptySuff, NULL);
    --emptySuff->refCount;
    SuffFreeSuff(emptySuff);

    Lst_ForEach(sufflist, SuffUnlinkChildren, NULL);
    Lst_Destroy(sufflist, SuffFreeSuffSufflist);
}


/*
 ******************************************************************************
 *			Parsing Helpers
 ******************************************************************************
 */

/*-
 *-----------------------------------------------------------------------
 * Suff_ClearSuffixes -- remove all known suffixes.
 *
 * Effectively, the module is re-initialized.
 *
 * Side Effects:
 *	All OP_TRANSFORM nodes in the graph are reset to regular nodes.
 *-----------------------------------------------------------------------
 */
void
Suff_ClearSuffixes(void)
{
    SuffCleanUp(SCU_CLEAR);
    Suff_Init();
}

/*-
 *-----------------------------------------------------------------------
 * SuffParseTransform -- get component suffixes from transformation name.
 *
 * Double suffix rules are preferred over single suffix ones (i.e.
 * transformations to the empty suffix).  This only matters if any suffix
 * is a catenation of two others: if .tar.gz, .tar, and .gz are known, then
 * ".tar.gz" is double suffix transformation from .tar to .gz, not a single
 * suffix one from .tar.gz.
 *
 * Input:
 *	name		transformation name
 *	sourceSuff	place to store the source suffix
 *	targetSuff	place to store the target suffix
 *
 * Results:
 *	TRUE if the string is a valid transformation and FALSE otherwise.
 *
 * Side Effects:
 *	The passed pointers are overwritten: by the actual suffixes when
 *	TRUE is returned, with NULLs otherwise.
 *
 *-----------------------------------------------------------------------
 */
static Boolean
SuffParseTransform(char *name, Suff **sourceSuff, Suff **targetSuff)
{
    LstNode s;		/* sufflist iterator for source */
    Suff *from, *to;	/* source and target suffixes */
    Suff *single;	/* the suffix that matches the whole name */

    s = NULL;
    from = to = single = NULL;

    /* Don't bother with obviously invalid names. */
    if (!lookup[(unsigned char)name[0]])
	goto end;

    /*
     * Is it a double suffix transformation?  For each suffix that matches
     * the start of the name, see if another one matches the rest of it.
     * The first valid pair is chosen.  The first suffix that matches
     * the whole name is remembered, so the suffixes need not to be checked
     * again for a single suffix transformation.  from and to stay as
     * NULLs if no valid transformation is found.
     */
    for (s = Lst_Find(sufflist, name, SuffSuffIsPrefixP); s != NULL;
	s = Lst_FindFrom(sufflist, Lst_Succ(s), name, SuffSuffIsPrefixP))
    {
	from = (Suff *)Lst_Datum(s);
	char *targetName = name + from->nameLen;

	if (*targetName == '\0') {
	    single = from;
	} else {
	    LstNode t = Lst_Find(sufflist, targetName, SuffSuffHasNameP);
	    if (t != NULL) {
		to = (Suff *)Lst_Datum(t);
		break;
	    }
	}
	from = NULL;
    }

    /* Use the full match for a single if it wasn't a double. */
    if (from == NULL && single != NULL) {
	from = single;
	to = emptySuff;
    }

end:
    if (sourceSuff != NULL && targetSuff != NULL) {
	*sourceSuff = from;
	*targetSuff = to;
    }

    return from != NULL;
}

/*-
 *-----------------------------------------------------------------------
 * Suff_IsTransform  -- check if the given string is a transformation rule.
 *
 * Figuring out if the name is a transformation requires us to find
 * the suffixes which would make up the transformation.  The same
 * information will be calculated if Suff_AddTransform() is then called
 * with the same name.  The information from this call can be cached by
 * providing non-NULL values for fromCache and toCache and passing all
 * the same parameters to Suff_AddTransform() if this function returns
 * true..
 *
 * Input:
 *	str		string to check
 *	fromCache	opaque cache pointer
 *	toCache		opaque cache pointer
 *
 * Results:
 *	TRUE if the string is a catenation of two known suffixes or
 *	matches one suffix exactly,  FALSE otherwise
 *-----------------------------------------------------------------------
 */
Boolean
Suff_IsTransform(char *str, void **fromCache, void **toCache)
{
    return SuffParseTransform(str, (Suff **)fromCache, (Suff **)toCache);
}

/*-
 *-----------------------------------------------------------------------
 * Suff_AddTransform -- record the transformation defined by name.
 *
 * A node is created into the graph to store the transformation, if one
 * does not already exist.
 *
 * Pre-condition:
 *	If src and targ are non-NULL, they must have been parameters
 *	of a call to Suff_IsTransform() that returned true for the same
 *	set of parameters.
 *
 * Input:
 *	name		name of new transformation
 *	fromCache	opaque cache pointer
 *	toCache		opaque cache pointer
 *
 * Results:
 *	The node of the transformation.
 *-----------------------------------------------------------------------
 */
GNode *
Suff_AddTransform(char *name, void **fromCache, void **toCache)
{
    GNode *node;
    Suff *from, *to;
    if (fromCache != NULL && toCache != NULL) {
	from = (Suff *)*fromCache;
	to = (Suff *)*toCache;
    } else
	from = to = NULL;

    node = Targ_FindNode(name, TARG_CREATE);

    if ((from != NULL || SuffParseTransform(name, &from, &to))
	&& !(node->type & OP_TRANSFORM))
    {
	node->type |= OP_TRANSFORM;
	Lst_AtEnd(transforms, node);
	SuffSetSuffix(node, to);

	/* Pre-cond: this did not exist before, can't be linked yet. */
	SuffLinkSuffixes(from, to);
    }

    return node;
}

/*-
 *-----------------------------------------------------------------------
 * Suff_EndTransform --
 *	Handle the finish of a transformation definition, removing the
 *	transformation from the graph if it has neither commands nor
 *	sources. This is a callback procedure for the Parse module via
 *	Lst_ForEach
 *
 * Input:
 *	gnp		Node for transformation
 *
 * Results:
 *	=== 0
 *
 * Side Effects:
 *	If the node has no commands or children, the children and parents
 *	lists of the affected suffixes are altered.
 *
 *-----------------------------------------------------------------------
 */
int
Suff_EndTransform(void *gnp, void *unused)
{
    GNode *gn = (GNode *)gnp;

    (void)unused;

    if ((gn->type & OP_DOUBLEDEP) && !Lst_IsEmpty (gn->cohorts))
	gn = (GNode *)Lst_Datum(Lst_Last(gn->cohorts));
    if ((gn->type & OP_TRANSFORM) && Lst_IsEmpty(gn->commands) &&
	Lst_IsEmpty(gn->children))
    {
	/* So it is an empty target. */
	Suff	*s, *t;

	/* But is it a transformation? (.DEFAULT is also OP_TRANSFORM) */
	if (SuffParseTransform(gn->name, &s, &t)) {
	    SuffDebug("Deleting a transformation with no commands.\n");
	    SuffUnlinkSuffixes(s, t);
	    gn->type |= ~OP_TRANSFORM;
	}
    } else if ((gn->type & OP_TRANSFORM))
	SuffDebug("Transformation %s completed.\n", gn->name);

    return 0;
}

/*-
 *-----------------------------------------------------------------------
 * SuffScanTargets --
 *	Called from Suff_AddSuffix via Lst_ForEach to search through the
 *	list of existing targets for necessary changes.
 *
 *	Any target that was not already a transformation and according to
 *	SuffParseTransform() now is, is made into one.  Existing
 *	single suffix transformations, that are now valid two suffix ones,
 *	are converted.  There's no change in double prefix rules.
 *	The single suffix conversion could not happen if we were to accept
 *	only POSIX suffixes, but we accept everything.
 *
 *	What is so special about single suffix transformations?  Consider
 *	the case in which suffixes s1, s2, ..., and sN are given.  Then
 *	any double prefix rule is sXsY such that sX is the first suffix
 *	in the list for which the remainder sY is also in the list.
 *	Thus X and Y are both at most N.  No matter what suffix we add
 *	to the list, X and Y cannot change.
 *
 *	For a single suffix rule the case is different.  Given suffixes
 *	.a.b and .a, the transformation ".a.b" is a single prefix rule.
 *	We always give precedence for double prefix rules, so when
 *	.b is added as a suffix, the transformation should now be
 *	a double suffix rule .a -> .b.
 *
 * Input:
 * 	targetNode	Current target to check.
 * 	newSuffix	The newly added suffix.
 *
 * Results:
 *	Always 0, so Lst_ForEach() won't stop prematurely.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
static int
SuffScanTargets(void *targetGNode, void *newSuffix)
{
    GNode *target = (GNode *)targetGNode;
    Suff *from, *to;
    Suff *suffix = (Suff *)newSuffix;

    from = to = NULL;

    /* Reject obviously irrelevant targets. */
    if (!(lookup[(unsigned char)target->name[0]]
	&& strstr(target->name, suffix->name) != NULL))
    {
	goto out;
    }

    if (SuffParseTransform(target->name, &from, &to)) {
	if (target->type & OP_TRANSFORM) {
	    if (target->suffix == emptySuff && to != emptySuff)
		SuffUnlinkSuffixes(from, to); /* single to double */
	    else
		from = to = NULL; /* still a single, or was double */
	} else
	    target->type |= OP_TRANSFORM; /* regular to transformation */

	if (from != NULL && to != NULL) {
	    /* Pre-cond: new double or completely new, can't be linked yet. */
	    SuffLinkSuffixes(from, to);
            Lst_AtEnd(transforms, target);
        }
    }

out:
    return 0;
}

/*-
 *-----------------------------------------------------------------------
 * Suff_AddSuffix --
 *	Add the named suffix to the end of the list of known suffixes.
 *	Existing targets are checked to see if any of them becomes
 *	a transformation.
 *
 *	If the suffix is already in the list of known suffixes, nothing
 *	is done.
 *
 *	After this function is called, the target list should be re-scanned
 *	in case the current main target was made into a transformation rule.
 *
 * Input:
 *	name	the name of the suffix to add
 *
 * Results:
 *	TRUE if a new suffix was added, FALSE otherwise.
 *
 * Side Effects:
 *	A GNode is created for the suffix and a Suff structure is created and
 *	appended to the suffixes list unless the suffix was already known.
 *-----------------------------------------------------------------------
 */
void
Suff_AddSuffix(char *name)
{
    Suff          *s;	/* New one for adding */
    LstNode 	  old;

    old = Lst_Find(sufflist, name, SuffSuffHasNameP);
    if (old == NULL) {
	s = SuffNewSuff(name);
	SuffAddToList(s, sufflist, NULL);

	lookup[(unsigned char)s->name[0]] = 1;

	Lst_ForEach(Targ_List(), SuffScanTargets, s);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Suff_GetPath --
 *	Return the search path for the given suffix, if it's defined.
 *
 * Results:
 *	The searchPath for the desired suffix or NULL if the suffix isn't
 *	defined.
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
Lst
Suff_GetPath(char *sname)
{
    LstNode   	  ln;
    Suff    	  *s;

    ln = Lst_Find(sufflist, sname, SuffSuffHasNameP);
    if (ln != NULL)
	s = (Suff *)Lst_Datum(ln);

    return (ln == NULL ? NULL : s->searchPath);
}

/*-
 *-----------------------------------------------------------------------
 * Suff_DoPaths --
 *	Extend the search paths for all suffixes to include the default
 *	search path.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The searchPath field of all the suffixes is extended by the
 *	directories in dirSearchPath. If paths were specified for the
 *	".h" suffix, the directories are stuffed into a global variable
 *	called ".INCLUDES" with each directory preceded by a -I. The same
 *	is done for the ".a" suffix, except the variable is called
 *	".LIBS" and the flag is -L.
 *-----------------------------------------------------------------------
 */
void
Suff_DoPaths(void)
{
    Suff	   	*s;
    LstNode  		ln;
    char		*ptr;
    Lst	    	    	inIncludes; /* Cumulative .INCLUDES path */
    Lst	    	    	inLibs;	    /* Cumulative .LIBS path */

    if (Lst_Open(sufflist) == FAILURE) {
	return;
    }

    inIncludes = Lst_Init(FALSE);
    inLibs = Lst_Init(FALSE);

    while ((ln = Lst_Next(sufflist)) != NULL) {
	s = (Suff *)Lst_Datum(ln);
	if (!Lst_IsEmpty (s->searchPath)) {
#ifdef INCLUDES
	    if (s->flags & SUFF_INCLUDE) {
		Dir_Concat(inIncludes, s->searchPath);
	    }
#endif /* INCLUDES */
#ifdef LIBRARIES
	    if (s->flags & SUFF_LIBRARY) {
		Dir_Concat(inLibs, s->searchPath);
	    }
#endif /* LIBRARIES */
	    Dir_Concat(s->searchPath, dirSearchPath);
	} else {
	    Lst_Destroy(s->searchPath, Dir_Destroy);
	    s->searchPath = Lst_Duplicate(dirSearchPath, Dir_CopyDir);
	}
    }

    Var_Set(".INCLUDES", ptr = Dir_MakeFlags("-I", inIncludes), VAR_GLOBAL, 0);
    free(ptr);
    Var_Set(".LIBS", ptr = Dir_MakeFlags("-L", inLibs), VAR_GLOBAL, 0);
    free(ptr);

    Lst_Destroy(inIncludes, Dir_Destroy);
    Lst_Destroy(inLibs, Dir_Destroy);

    Lst_Close(sufflist);
}

/*-
 *-----------------------------------------------------------------------
 * Suff_AddInclude --
 *	Add the given suffix as a type of file which gets included.
 *	Called from the parse module when a .INCLUDES line is parsed.
 *	The suffix must have already been defined.
 *
 * Input:
 *	sname		Name of the suffix to mark
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The SUFF_INCLUDE bit is set in the suffix's flags field
 *
 *-----------------------------------------------------------------------
 */
void
Suff_AddInclude(char *sname)
{
    LstNode	  ln;
    Suff	  *s;

    ln = Lst_Find(sufflist, sname, SuffSuffHasNameP);
    if (ln != NULL) {
	s = (Suff *)Lst_Datum(ln);
	s->flags |= SUFF_INCLUDE;
    }
}

/*-
 *-----------------------------------------------------------------------
 * Suff_AddLib --
 *	Add the given suffix as a type of file which is a library.
 *	Called from the parse module when parsing a .LIBS line. The
 *	suffix must have been defined via .SUFFIXES before this is
 *	called.
 *
 * Input:
 *	sname		Name of the suffix to mark
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The SUFF_LIBRARY bit is set in the suffix's flags field
 *
 *-----------------------------------------------------------------------
 */
void
Suff_AddLib(char *sname)
{
    LstNode	  ln;
    Suff	  *s;

    ln = Lst_Find(sufflist, sname, SuffSuffHasNameP);
    if (ln != NULL) {
	s = (Suff *)Lst_Datum(ln);
	s->flags |= SUFF_LIBRARY;
    }
}


/*
 ******************************************************************************
 *			Implicit Source Search Functions
 ******************************************************************************
 */

/*-
 *-----------------------------------------------------------------------
 * SuffAddSrc -- add transformation possibility.
 *
 * Create an Src structure describing a transformation from a file with
 * the given suffix to the given target (target.from -> target.to).
 * If the suffix happens to be the .NULL suffix, create a transformation
 * from target's prefix also (target -> target.to).
 *
 * This is callback function for Lst_ForEach().
 *
 * Input:
 *	suffix		the source's suffix
 *	arguments	more arguments: target, transformations not yet tried,
 *			cleanup list.
 *
 * Results:
 *	Always return 0 to prevent Lst_ForEach() from exiting prematurely.
 *-----------------------------------------------------------------------
 */
static int
SuffAddSrc(void *suffix, void *arguments)
{
    Suff	*fromSuffix;
    LstSrc	*args;
    Src		*from, *nullChild, *to;

    fromSuffix = (Suff *)suffix;
    args = (LstSrc *)arguments;
    to = args->target;

    /*
     * Don't add a transformation that is already in the chain or we
     * might get stuck in an infinite loop.
     */
    for (to = args->target; to != NULL; to = to->parent) {
	if (to->suff == fromSuffix) {
	    SuffDebug("\t%*.s%s%s: ignored (already tried in this chain)\n",
		args->target->depth + 1, " ", to->pref, fromSuffix->name);
	    goto end;
	}
    }
    to = args->target;

    from = SuffNewChildSrc(to, fromSuffix, &nullChild);
#ifdef DEBUG_SRC
    fprintf(debug_file, "1 add %x %x to %x:", to, from, args->possible);
    if (nullChild != NULL)
	fprintf(debug_file, "2 add %x %x to %x:", to, nullChild,
	    args->possible);
    Lst_ForEach(args->possible, PrintAddr, NULL);
    fprintf(debug_file, "\n");
#endif
    Lst_AtEnd(args->possible, from);
    Lst_AtEnd(args->cleanup, from);
    if (nullChild != NULL) {
	Lst_AtEnd(args->possible, nullChild);
	Lst_AtEnd(args->cleanup, nullChild);
    }

end:
    return 0;
}

/*-
 *-----------------------------------------------------------------------
 * SuffAddLevelForSuffix -- get possible transformations to target node.
 *
 * A new top level Src structure (parent == NULL) is created for target
 * using the provided suffix.  Then Src structures describing all
 * the possible ways to create the target node are appended into the list
 * possible.  If the target node has multiple known suffixes, this function
 * should be called for each suffix separately to get all the possible ways
 * to create the node.
 *
 * The top level Src and the possible transformations are added into
 * cleanup.
 *
 * Input:
 *	suffix		target node's suffix
 *	possible	list to add the possible transformations into
 *			(data: Src *)
 *	end		desired transformation target
 *	cleanup		list of all Srcs for eventual cleanup (data: Src *)
 *-----------------------------------------------------------------------
 */
static void
SuffAddLevelForSuffix(Suff *suffix, Lst possible, GNode *end, Lst cleanup)
{
    Src *s;
    if (suffix != NULL) {
	if (suffix->flags & SUFF_NULL)
	    /* Only set when "remove a suffix" aspect of .NULL is used. */
	    s = SuffNewTopSrcNull(end, suffix);
	else
	    s = SuffNewTopSrc(end, suffix);
	Lst_AtEnd(cleanup, s);

	SuffAddLevel(possible, s, cleanup);
    }
}

/*-
 *-----------------------------------------------------------------------
 * SuffAddLevel -- get possible transformations to target Src.
 *
 * Src structures describing all the possible ways to create target are
 * appended into the list possible.
 *
 * The possible transformations are also added into cleanup.
 *
 * Input:
 *	possible	list to add the possible transformations to
 *			(data: Src *)
 *	target		desired transformation target
 *	cleanup		list of all Srcs for eventual clean-up (data: Src *)
 *-----------------------------------------------------------------------
 */
static void
SuffAddLevel(Lst possible, Src *target, Lst cleanup) {
    LstSrc ls;

    ls.target = target;
    ls.possible = possible;
    ls.cleanup = cleanup;

    Lst_ForEach(target->suff->children, SuffAddSrc, &ls);
}

/*-
 *-----------------------------------------------------------------------
 * SuffFindThem -- find a transformation chain from a list of possibilities.
 *
 * A valid transformation chain is one which starts from an existing
 * target node or file and can be converted into the desired target by
 * repeated application of transformation rules.  If any such chain
 * can be found, this function will return the one which is the shortest.
 * If there are multiple equally short chains, the steps required are
 * compared, starting from the /last/ one, and on the first differing
 * step the one whose target suffix occurs first in .SUFFIXES is chosen.
 *
 * Input:
 *	possible	all possible last steps in the transformation,
 *			in the proper .SUFFIXES order
 *			chain (data: Src *)
 *	cleanup		list of all Srcs for eventual clean-up (data: Src *)
 *
 * Results:
 *	The starting point of the shortest, highest priority transformation
 *	chain.  Taking the node described by the returned value and
 *	applying to it the transformations defined by the parent pointers
 *	will result in the initially desired target.
 *
 *	If no chain terminates at an existing target or file, NULL is
 *	returned.
 *-----------------------------------------------------------------------
 */
static Src *
SuffFindThem(Lst possible, Lst cleanup)
{
    Src	*i, *result, *parent;
    char *temp;

    result = NULL;
    /*
     * Parent of the current candidate.  When it changes, debug print
     * the chain of transformations so far.
     */
    parent = NULL;

    /*
     * You've been lied to.  There wont be any step priority comparisons.
     * Because the list initially contains the possible transformations
     * in the correct order, the first existing one is the correct result.
     * Each possible transformation is removed from the front of the list
     * as it is checked and if it does not exist, all the ways to create
     * it are appended to the list.  This way the one step longer chains
     * are also in the correct priority order and the item at the front
     * of the list is always the correct result, should it exist.  It is
     * then easy to keep popping the list until the result is found or all
     * possibilities are exhausted.
     */
    while ((i = (Src *)Lst_DeQueue(possible)) != NULL) {
	if (parent != i->parent) {
	    SuffDebugChain(i->parent);
	    parent = i->parent;
	}
	SuffDebug ("\t%*.s%s: ", i->depth, " ", i->file);

	/*
	 * XXX: should only targets with commands be accepted?  The node
	 * exists even if it only has had extra dependencies added.
	 */
	if (Targ_FindNode(i->file, TARG_NOCREATE) != NULL) {
#ifdef DEBUG_SRC
	    fprintf(debug_file, "remove %x from %x\n", i, possible);
#endif
	    result = i;
	    break;
	}

	if ((temp = Dir_FindFile(i->file, i->suff->searchPath)) != NULL) {
	    result = i;
#ifdef DEBUG_SRC
	    fprintf(debug_file, "remove %x from %x\n", i, possible);
#endif
	    free(temp);
	    break;
	}

	SuffDebug("not there.\n");

	SuffAddLevel(possible, i, cleanup);
    }

    if (result)
	SuffDebug("got it.\n");

    return result;
}

/*-
 *-----------------------------------------------------------------------
 * SuffExpandChildren -- call SuffExpandChild() on node's children.
 *
 * Input:
 *	parent	parent node
 *	first	child to start from
 *-----------------------------------------------------------------------
 */
static void
SuffExpandChildren(GNode *parent, LstNode first)
{
    LstNode i, next;

    /*
     * SuffExpandChild() replaces the curren node with the expanded values,
     * so we won't try to expand the already expanded values
     */
    for (i = first; i != NULL; i = next) {
	/* SuffExpandChild() might remove i, so get the successor now. */
	next = Lst_Succ(i);
	SuffExpandChild(i, parent);
    }
}

/*-
 *-----------------------------------------------------------------------
 * SuffExpandChild -- expand variables and wildcards in a child name.
 *
 * Expand variables and wildcards in a child's name and replace the child
 * with the results if an expansion happened.  This means that when
 * expansion occurred, cln will point to free'd memory!
 *
 * Input:
 *	cln		child to expand.
 *	pgn		parent node being processed
 *
 * Results:
 *	=== 0 (continue)
 *-----------------------------------------------------------------------
 */
static void
SuffExpandChild(LstNode cln, GNode *pgn)
{
    GNode   	*cgn = (GNode *)Lst_Datum(cln);
    GNode	*gn;		/* helper for adding expanded nodes */
    char	*expanded;	/* expanded child name */
    char	*cp;		/* current position in expanded value */

    if (!Lst_IsEmpty(cgn->order_pred) || !Lst_IsEmpty(cgn->order_succ))
	/* It is all too hard to process the result of .ORDER */
	return;

    if (cgn->type & OP_WAIT)
	/* Ignore these (& OP_PHONY ?) */
	return;

    /*
     * First do variable expansion -- this takes precedence over
     * wildcard expansion. If the result contains wildcards, they'll be gotten
     * to later since the resulting words are tacked on to the end of
     * the children list.
     */
    if (strchr(cgn->name, '$') == NULL) {
	SuffExpandWildcards(cln, pgn);
	return;
    }

    SuffDebug("\tVariable expanding dependency `%s'\n", cgn->name);
    cp = expanded = Var_Subst(NULL, cgn->name, pgn, TRUE);

    if (cp != NULL) {
	Lst	    members = Lst_Init(FALSE);

	if (cgn->type & OP_ARCHV) {
	    /*
	     * Node was an archive(member) target, so we want to call
	     * on the Arch module to find the nodes for us, expanding
	     * variables in the parent's context.
	     */
	    char	*sacrifice = cp;

	    (void)Arch_ParseArchive(&sacrifice, members, pgn);
	} else {
	    /*
	     * Break the result into a vector of strings whose nodes
	     * we can find, then add those nodes to the members list.
	     * Unfortunately, we can't use brk_string b/c it
	     * doesn't understand about variable specifications with
	     * spaces in them...
	     * XXX: what variable specs?!  I thought we already expanded
	     * all of the variables and there shouldn't be any variable
	     * expansion stages left before the name is used as target!
	     */
	    char	    *start;

	    for (start = cp; *start == ' ' || *start == '\t'; start++)
		continue;
	    for (cp = start; *cp != '\0'; cp++) {
		if (*cp == ' ' || *cp == '\t') {
		    /*
		     * White-space -- terminate element, find the node,
		     * add it, skip any further spaces.
		     */
		    *cp++ = '\0';
		    gn = Targ_FindNode(start, TARG_CREATE);
		    (void)Lst_AtEnd(members, gn);
		    while (*cp == ' ' || *cp == '\t') {
			cp++;
		    }
		    /*
		     * Adjust cp for increment at start of loop, but
		     * set start to first non-space.
		     */
		    start = cp--;
		} else if (*cp == '$') {
		    /*
		     * Start of a variable spec -- contact variable module
		     * to find the end so we can skip over it.
		     */
		    char	*junk;
		    int 	len;
		    void	*freeIt;

		    junk = Var_Parse(cp, pgn, TRUE, &len, &freeIt);
		    if (junk != var_Error) {
			cp += len - 1;
		    }

		    if (freeIt)
			free(freeIt);
		} else if (*cp == '\\' && *cp != '\0') {
		    /*
		     * Escaped something -- skip over it
		     */
		    cp++;
		}
	    }

	    if (cp != start) {
		/*
		 * Stuff left over -- add it to the list too
		 */
		gn = Targ_FindNode(start, TARG_CREATE);
		(void)Lst_AtEnd(members, gn);
	    }
	}

	/*
	 * Add all elements of the members list to the parent node.
	 */
	while(!Lst_IsEmpty(members)) {
	    gn = (GNode *)Lst_DeQueue(members);

	    SuffDebug("\t\t%s\n", gn->name);
	    /* Add gn to the parents child list before the original child */
	    (void)Lst_InsertBefore(pgn->children, cln, gn);
            if ((GNode *)Lst_Datum(pgn->first_local_child) == cgn)
                pgn->first_local_child = Lst_Prev(cln);
	    (void)Lst_AtEnd(gn->parents, pgn);
	    pgn->unmade++;
	    /* Expand wildcards on new node */
	    SuffExpandWildcards(Lst_Prev(cln), pgn);
	}
	Lst_Destroy(members, NULL);

	free(expanded);
	expanded = NULL;
    }

    /*
     * Now the source is expanded, remove it from the list of children to
     * keep it from being processed.
     */
    pgn->unmade--;
    Lst_Remove(pgn->children, cln);
    Lst_Remove(cgn->parents, Lst_Member(cgn->parents, pgn));
}

static void
SuffExpandWildcards(LstNode cln, GNode *pgn)
{
    GNode   	*cgn = (GNode *)Lst_Datum(cln);
    GNode	*gn;	    /* New source 8) */
    char	*cp;	    /* Expanded value */
    Lst 	explist;    /* List of expansions */

    if (!Dir_HasWildcards(cgn->name))
	return;

    /*
     * Expand the word along the chosen path
     */
    SuffDebug("\tWildcard expanding dependency `%s'\n", cgn->name);
    explist = Lst_Init(FALSE);
    Dir_Expand(cgn->name, Suff_FindPath(cgn), explist);

    while (!Lst_IsEmpty(explist)) {
	/*
	 * Fetch next expansion off the list and find its GNode
	 */
	cp = (char *)Lst_DeQueue(explist);

	SuffDebug("\t\t%s\n", cp);
	gn = Targ_FindNode(cp, TARG_CREATE);

	/* Add gn to the parents child list before the original child */
	(void)Lst_InsertBefore(pgn->children, cln, gn);
        if ((GNode *)Lst_Datum(pgn->first_local_child) == cgn)
            pgn->first_local_child = Lst_Prev(cln);
	(void)Lst_AtEnd(gn->parents, pgn);
	pgn->unmade++;
    }

    /*
     * Nuke what's left of the list
     */
    Lst_Destroy(explist, NULL);

    /*
     * Now the source is expanded, remove it from the list of children to
     * keep it from being processed.
     */
    pgn->unmade--;
    Lst_Remove(pgn->children, cln);
    Lst_Remove(cgn->parents, Lst_Member(cgn->parents, pgn));
}

/*-
 *-----------------------------------------------------------------------
 * Suff_FindPath --
 *	Find a path along which to expand the node.
 *
 *	If the word has a known suffix, use that path.
 *	If it has no known suffix, use the default system search path.
 *
 * Input:
 *	gn		Node being examined
 *
 * Results:
 *	The appropriate path to search for the GNode.
 *
 * Side Effects:
 *	XXX: We could set the suffix here so that we don't have to scan
 *	again.
 *
 *-----------------------------------------------------------------------
 */
Lst
Suff_FindPath(GNode* gn)
{
    Suff *suff = gn->suffix;

    if (suff == NULL) {
	SuffixCmpData sd;   /* Search string data */
	LstNode ln;
	sd.len = strlen(gn->name);
	sd.ename = gn->name + sd.len;
	ln = Lst_Find(sufflist, &sd, SuffSuffIsSuffixP);

	if (ln != NULL)
	    suff = (Suff *)Lst_Datum(ln);
	/* XXX: Here we can save the suffix so we don't have to do this again */
    }

    if (suff != NULL)
	return suff->searchPath;
    else
	return dirSearchPath;
}


/*-
 *-----------------------------------------------------------------------
 * SuffApplyTransformations -- apply a transformation chain.
 *
 * Apply transformations beginning at start, until the node end is
 * reached.  A node is created for each intermediate target, if one does
 * not already exist and the relevant suffixes are set on the nodes.
 *
 * Each target except for the start and end of the chain gets TARGET
 * and PREFIX set and their children expanded.  The start of the chain
 * cannot be expanded as it could turn out to be the result of another
 * transformation chain and have a different suffix as a part of that
 * chain.  The end of the chain cannot be expanded because the node's
 * name might be a fake one (see SuffFindArchiveDeps()).
 *
 * Input:
 *	start	transformation chain's starting
 *	end	transformation chain's end,  i.e. the node for which we were
 *		initially looking dependencies for
 *
 * Results:
 *	The suffix that was set on end.
 *-----------------------------------------------------------------------
 */
static Suff *
SuffApplyTransformations(Src *start, GNode *end)
{
    Src *target, *source;

    if (start->node == NULL)
	start->node = Targ_FindNode(start->file, TARG_CREATE);

    for (source = start; source->parent != NULL; source = source->parent) {
	target = source->parent;

	SuffSetSuffix(source->node, source->suff);

	if (target->node == NULL)
	    target->node = Targ_FindNode(target->file, TARG_CREATE);

	if (target->node != end) {
	    /*
	     * Dependency search for intermediate targets is finished:
	     * if they had dependencies to check, they would have
	     * a target or an existing file and therefore wouldn't be
	     * intermediate targets.
	     */
	    target->node->type |= OP_DEPS_FOUND;
	    Var_Set(TARGET, target->node->name, target->node, 0);
	    Var_Set(PREFIX, target->pref, target->node, 0);
	}

	SuffApplyTransformation(target->node, source->node,
	    target->suff, source->suff,
	    (target->node == end ? SAT_NO_EXPAND : SAT_REGULAR));
    }

    SuffSetSuffix(end, source->suff);

    return end->suffix;
}

/*-
 *-----------------------------------------------------------------------
 * SuffApplyTransformation -- apply a transformation from source to target.
 *
 * Input:
 *	tGn	Target node
 *	sGn	Source node
 *	t	Target suffix
 *	s	Source suffix
 *	flags	Request modifications for standard behavior.
 *		SAT_REGULAR:
 *			Normal behavior.
 *		SAT_NO_EXPAND:
 *			Do not expand children.
 *
 * Results:
 *	TRUE if successful, FALSE if not.
 *
 * Side Effects:
 *	The source and target are linked and the commands from the
 *	transformation are added to the target node's commands list.
 *	All attributes but OP_DEPMASK and OP_TRANSFORM are applied
 *	to the target. The target also inherits all the sources for
 *	the transformation rule.
 *
 *-----------------------------------------------------------------------
 */
static Boolean
SuffApplyTransformation(GNode *tGn, GNode *sGn, Suff *t, Suff *s, int flags)
{
    LstNode 	ln;         /* General node */
    char    	*tname;	    /* Name of transformation rule */
    GNode   	*gn;	    /* Node for same */

    /*
     * Form the proper links between the target and source.
     */
    (void)Lst_AtEnd(tGn->children, sGn);
    (void)Lst_AtEnd(sGn->parents, tGn);
    tGn->unmade += 1;

    /*
     * Locate the transformation rule itself
     */
    tname = str_concat(s->name, t->name, 0);
    ln = Lst_Find(transforms, tname, SuffGNHasNameP);
    free(tname);

    gn = (GNode *)Lst_Datum(ln);

    SuffDebug("\tApplying `%s' -> `%s' to `%s'\n", s->name, t->name,
	tGn->name);

    /* Record last child so only the new children get expanded. */
    ln = Lst_Last(tGn->children);

    /*
     * Pass the buck to Make_HandleUse to apply the rule
     */
    (void)Make_HandleUse(gn, tGn);

    /*
     * Deal with wildcards and variables in any acquired sources
     */
    if (!(flags & SAT_NO_EXPAND))
	SuffExpandChildren(tGn, Lst_Succ(ln));

    /*
     * Keep track of another parent to which this beast is transformed so
     * the .IMPSRC variable can be set correctly for the parent.
     */
    (void)Lst_AtEnd(sGn->iParents, tGn);

    return(TRUE);
}


/*-
 *-----------------------------------------------------------------------
 * SuffFirstKnownSuffix -- find the first suffix that fits name.
 *
 * Used by SuffFindArchiveDeps() and SuffFindNormalDeps() to figure
 * out suffixes for explicit rules.
 *
 * Input:
 *	name	The name to try the suffixes on.
 *
 * Results:
 *	The first matching suffix structure or NULL if there isn't one.
 *-----------------------------------------------------------------------
 */
static Suff*
SuffFirstKnownSuffix(char* name)
{
    LstNode ln;
    SuffixCmpData sd;

    sd.len = strlen(name);
    sd.ename = name + sd.len;

    ln = Lst_Find(sufflist, &sd, SuffSuffIsSuffixP);
    return (ln == NULL ? NULL : (Suff *)Lst_Datum(ln));
}

/*-
 *-----------------------------------------------------------------------
 * SuffSetPrefixLocalVar -- set .PREFIX properly on target.
 *
 * The value of the .PREFIX variable is the node's name less suffix->name.
 * Used by SuffFindArchiveDeps() and SuffFindNormalDeps().
 *
 * Input:
 *	suffix	The suffix structure to base the prefix on.  If it is NULL,
 *		the whole name is used.
 *	name	The name to get the prefix from, as it is not necessarily
 *		the node's name.
 *	node	The context to set the variable in.
 *-----------------------------------------------------------------------
 */
static void
SuffSetPrefixLocalVar(Suff *suffix, char *name, GNode *node)
{
    if (suffix != NULL) {
	char save, *save_pos;

	save_pos = name + (strlen(name) - suffix->nameLen);
	save = *save_pos;
	*save_pos = '\0';
	Var_Set(PREFIX, name, node, 0);
	*save_pos = save;
    }
    else
	Var_Set(PREFIX, name, node, 0);
}

/*-
 *-----------------------------------------------------------------------
 * SuffFindArchiveDeps --
 *	Locate dependencies for an OP_ARCHV node.
 *
 * Input:
 *	target	Node for which to locate dependencies
 *	cleanup	List to add all created Srcs into, so the caller can
 *		destroy them.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Same as Suff_FindDeps.  ARCHIVE and MEMBER variables are set as
 *	well as gn->type is modified to include OP_MEMBER
 *	on the relevant nodes and are so the modifications
 *
 *-----------------------------------------------------------------------
 */
static void
SuffFindArchiveDeps(GNode *target, Lst cleanup)
{
    Lst possible;	/* Possible transformation starting points. */
    Src *start;		/* The start of the chain of transformations that
			 * results in this target being built. */
    Suff *suffix;	/* Suffix of the member. */
    char *temp;

    char *lib, *member;		/* Copies of lib and member parts, */
    char *libEnd, *memberEnd;   /* their terminating NULs, and */
    size_t libLen, memberLen;	/* their lengths. */

    possible = Lst_Init(FALSE);
    start = NULL;
    suffix = NULL;

    lib = target->name;
    member = strchr(lib, '(') + 1;
    libLen = member - target->name - 1;
    memberLen = strchr(member, ')') - member;

    lib = bmake_strndup(lib, libLen);
    member = bmake_strndup(member, memberLen);
    libEnd = lib + libLen;
    memberEnd = member + memberLen;

    /*
     * Explicit rules always take precedence.  Explicit <=> appeared as
     * a target in a rule with commands.
     */
    if (!OP_NOP(target->type) && !Lst_IsEmpty(target->commands)) {
	SuffDebug("\tIt has an explicit rule.\n");

	suffix = SuffFirstKnownSuffix(member);
	goto expand_children;
    }

    /*
     * Try with POSIX semantics first, if applicable: library rules are only
     * supported for "lib(member.o)" and ".s2.a" is the transformation rule
     * from "member.s2" to "lib(member.o)", regardless of the suffix, if any,
     * "lib" might have.  We pretend to be "member.a" and look for
     * transformations.
     */
    if (memberLen > 2 && strcmp(memberEnd - 2, ".o") == 0) {
	LstNode ln;
	SuffDebug("\tTrying POSIX \".s2.a\" transformation.\n");

	*(memberEnd - 1) = 'a';
	temp = target->name; target->name = member;
	ln = Lst_Find(sufflist, ".a", SuffSuffHasNameP);
	SuffAddLevelForSuffix((ln == NULL ? NULL : (Suff *)Lst_Datum(ln)),
	    possible, target, cleanup);
	target->name = temp;
	*(memberEnd - 1) = 'o';

	start = SuffFindThem(possible, cleanup);
    }

    /*
     * Given lib.s2(member.s1), if a ".s1.s2" transformation exists, try
     * to find a way to make member.s1 so it could be added to lib.s2.
     * The problem is, all suffixes are accepted, not just POSIX ones
     * (\.[^./]+), so more than one may match either lib or member and
     * not all permutations are valid.
     *
     * Here are a couple of examples of what has to be taken into account.
     * Consider "lib.a.b(member.c)" in a case where "member.d" exists, and
     * with ".SUFFIXES: .b .a.b .c .d".  The transformations are ".d.c" and
     * ".c.b".  If one were to swap the longest suffixes and try with
     * "member.a.b", the transformation would not be found.  Now consider
     * if the transformation ".d.a.b" also existed.  If one were to try
     * in order each suffix of the lib against each suffix of the member
     * and take the first one that works, the chain ".d" -> ".c" -> ".b"
     * would be found but traditionally transformations choose the shortest
     * chain, which would be ".d" -> ".a.b".
     *
     * These issues mean that trying to shoehorn SuffFindNormalDeps()
     * to be useful here would be more trouble than it is worth.  Gladly
     * that use would have mainly been as a frontend for SuffFindThem() and
     * SuffApplyTransformations() and things actually become cleaner if
     * they are used directly.
     *
     * Single suffix rules are not acceptable because they're usually used
     * to create executables.
     */
    if (start == NULL) {
	/* One set for lib (l) and one for member (m). */
	LstNode l, m;		/* Suffix list iterators. */
	SuffixCmpData ld, md;	/* Parameters for predicates. */
	Suff *ls, *ms;		/* Current suffix. */

	SuffDebug("\tTrying \".s1.s2\" transformation extension.\n");

	ld.len = libLen;
	ld.ename = libEnd;
	md.len = memberLen;
	md.ename = memberEnd;

	/* Get all possible transformations from member to lib. */
	for (l = Lst_Find(sufflist, &ld, SuffSuffIsSuffixP); l != NULL;
	    l = Lst_FindFrom(sufflist, Lst_Succ(l), &ld, SuffSuffIsSuffixP))
	{
	    ls = ((Suff *)Lst_Datum(l));

	    for (m = Lst_Find(ls->children, &md, SuffSuffIsSuffixP);
		m != NULL;
		m = Lst_FindFrom(ls->children, Lst_Succ(m), &md,
		    SuffSuffIsSuffixP))
	    {
		char *fakename, save, *save_pos;

		ms = (Suff *)Lst_Datum(m);
		save_pos = memberEnd - ms->nameLen;
		save = *save_pos;

		*save_pos = '\0';
		fakename = str_concat(member, ls->name, 0);
		*save_pos = save;

		temp = target->name; target->name = fakename;
		SuffAddLevelForSuffix(ms, possible, target, cleanup);
		target->name = temp;
		free(fakename);
	    }
	}

	start = SuffFindThem(possible, cleanup);
    }

    if (start != NULL)
	suffix = SuffApplyTransformations(start, target);

expand_children:
    /*
     * POSIX: in a lib(member.o) and a .s2.a rule, the values for
     * the local variables are:
     *	$* = member	$@ = lib	$? = member.s2 	$% = member.o.
     * Additionally, for the .s2.a inference rule:
     *	$< = member.s2
     *
     * ARCHIVE and MEMBER are used by Arch_MTime() to find the modification
     * time.
     */
    Var_Set(ARCHIVE, lib, target, 0);
    Var_Set(MEMBER, member, target, 0);
    Var_Set(TARGET, lib, target, 0);

    if (suffix == NULL && memberLen > 2 && strcmp(memberEnd - 2, ".o") == 0)
    {
	/*
	 * POSIX compatibility: in case ".o" was not a known suffix, force it.
	 * The target is not POSIX compliant if a suffix other than ".o" was
	 * already matched, so there's no need to try to force e.g. ".fo.o"
	 * or "o" into ".o".  (Compliant suffixes start with a period
	 * and contain neither periods nor slashes.)
	 */
	*(memberEnd - 2) = '\0';
	Var_Set(PREFIX, member, target, 0);
	*(memberEnd - 2) = '.';
    } else
	SuffSetPrefixLocalVar(suffix, member, target);

    SuffExpandChildren(target, Lst_First(target->children));

    free(lib);
    free(member);
}

/*-
 *-----------------------------------------------------------------------
 * SuffFindNormalDeps -- locate dependencies for regular targets.
 *
 * If the target does not have an explicit rule a suffix transformation
 * rule search is performed to see if it can be inferred.
 *
 * Input:
 *	target	Node for which to find sources.
 *	cleanup	List to add all created Srcs into, so the caller can
 *		destroy them.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Same as Suff_FindDeps.
 *-----------------------------------------------------------------------
 */

static void
SuffFindNormalDeps(GNode *target, Lst cleanup)
{
    Lst possible;	/* List of possible transformations (Src). */
    Suff *suffix;	/* The suffix that applies to target. */
    Src *bottom;	/* Start of found transformation chain. */

    possible = Lst_Init(FALSE);
    suffix = NULL;
    bottom = NULL;

    /*
     * Explicit rules always take precedence.  Explicit <=> appeared as
     * a target in a rule with commands.
     */
    if (!OP_NOP(target->type) && !Lst_IsEmpty(target->commands)) {
	SuffDebug("\tIt has an explicit rule.\n");
	suffix = SuffFirstKnownSuffix(target->name);
    	goto expand_children;
    }

    /*
     * Try transformation rules.  They're used to make files from other files,
     * so don't try them for .PHONY targets, which are not actual files.
     * All suffixes are accepted, not just POSIX ones (\.[^./]+), so more
     * than one may match.  Matching ones are collected in .SUFFIXES order
     * and the one to give the shortest working chain is used for creating
     * the missing links and setting PREFIX.  In the case that at least one
     * suffix matched but none resulted in a chain, use the first one to set
     * PREFIX.
     */
    if (!(target->type & OP_PHONY)) {
	LstNode ln;
	SuffixCmpData sd;

	sd.len = strlen(target->name);
	sd.ename = target->name + sd.len;

	SuffDebug("\tTrying double suffix transformations.\n");
	for (ln = Lst_Find(sufflist, &sd, SuffSuffIsSuffixP); ln != NULL;
	    ln = Lst_FindFrom(sufflist, Lst_Succ(ln), &sd, SuffSuffIsSuffixP))
	{
            if (suffix == NULL)
                suffix = (Suff *)Lst_Datum(ln);
	    SuffAddLevelForSuffix((Suff *)Lst_Datum(ln), possible, target,
		cleanup);
	}

	if (suffix == NULL) {
	    SuffDebug("\tNo known suffix, trying single suffix "
		"transformations.\n");
	    SuffAddLevelForSuffix(emptySuff, possible, target, cleanup);

	    if (nullSuff != NULL) {
		SuffDebug("\tTrying with .NULL too.\n");
		nullSuff->flags |= SUFF_NULL;
		SuffAddLevelForSuffix(nullSuff, possible, target, cleanup);
		nullSuff->flags &= ~SUFF_NULL;
	    }
	}

	bottom = SuffFindThem(possible, cleanup);
    }

    if (bottom != NULL) {
	suffix = SuffApplyTransformations(bottom, target);
    } else {
	/* If there's no transformation, try search paths. */
	SuffDebug("\tNo transformations, trying path search.\n");

	if ((target->type & (OP_PHONY|OP_NOPATH)) == 0) {
	    free(target->path);
	    target->path = Dir_FindFile(target->name,
		(suffix == NULL ? dirSearchPath : suffix->searchPath));
	}
    }

expand_children:
    SuffSetSuffix(target, suffix);

    Var_Set(TARGET, (target->path ? target->path : target->name), target, 0);
    if (bottom != NULL)
	/* Only because of nullSuff: use "" instead of the real suffix. */
	SuffSetPrefixLocalVar(NULL, bottom->pref, target);
    else
	SuffSetPrefixLocalVar(target->suffix, target->name, target);

    SuffExpandChildren(target, Lst_First(target->children));
}


/*-
 *-----------------------------------------------------------------------
 * Suff_FindDeps  --
 *	Do suffix transformation rule search for the given target.
 *	Explicit rules always take precedence, so the search will not be
 *	done, if an explicit rule is detected.
 *
 * Input:
 *	gn	node to check inference rules for
 *
 * Results:
 *	Nothing.
 *
 * Side Effects:
 *	Nodes may be added as dependencies to the target if implied so
 *	by suffix search.  Any newly created nodes are also added to
 *	the graph.  The implied sources are linked via their iParents
 *	field to the target that uses them so the target will get its
 *	IMPSRC variable filled in properly later.
 *
 *	The TARGET, PREFIX, ARCHIVE and MEMBER variables get set on
 *	the target and its children as needed.
 *
 * Notes:
 *	The path found by this target is the shortest path in the
 *	transformation graph, which may pass through non-existent targets,
 *	to an existing target. The search continues on all paths from the
 *	root suffix until a file or an existing target node is found.
 *	I.e. if there's a path .o -> .c -> .l -> .l,v from the root and
 *	the .l,v file exists but the .c and .l files don't, the search
 *	will branch out in all directions from .o and again from all
 *	the nodes on the next level until the .l,v node is encountered.
 *
 *-----------------------------------------------------------------------
 */

void
Suff_FindDeps(GNode *target)
{
    /*
     * Storage for all Src structures created during the search.  This is
     * purely a convenience for the implementation of the helper functions.
     * This way there does not need to be logic for deciding what is safe
     * to remove and what is not.  After the search is complete, they can
     * all be safely removed.
     */
    Lst cleanup = Lst_Init(FALSE);

    if (target->type & OP_DEPS_FOUND) {
	/*
	 * If dependencies already found, no need to do it again...
	 */
	return;
    } else {
	target->type |= OP_DEPS_FOUND;
    }
    /*
     * Make sure we have these set, may get revised below.
     */
    Var_Set(TARGET, target->path ? target->path : target->name, target, 0);
    Var_Set(PREFIX, target->name, target, 0);

    SuffDebug("SuffFindDeps (%s)\n", target->name);

    if (target->type & OP_ARCHV)
	SuffFindArchiveDeps(target, cleanup);
    else if (target->type & OP_LIB) {
	/*
	 * If the node is a library, it is the arch module's job to find it
	 * and set the TARGET variable accordingly. We merely provide the
	 * search path, assuming all libraries end in ".a" (if the suffix
	 * hasn't been defined, there's nothing we can do for it, so we just
	 * set the TARGET variable to the node's name in order to give it a
	 * value).
	 * XXX: try all suffixes with SUFF_LIBRARY set?
	 */
	LstNode	ln;
	Suff	*s;

	ln = Lst_Find(sufflist, LIBSUFF, SuffSuffHasNameP);
	s = (ln == NULL ? NULL : (Suff *)Lst_Datum(ln));
	SuffSetSuffix(target, s);
	if (s != NULL)
	    Arch_FindLib(target, s->searchPath);
	else
	    Var_Set(TARGET, target->name, target, 0);
	/*
	 * Because a library (-lfoo) target doesn't follow the standard
	 * filesystem conventions, we don't set the regular variables for
	 * the thing. .PREFIX is simply made empty...
	 */
	Var_Set(PREFIX, "", target, 0);
    } else
	SuffFindNormalDeps(target, cleanup);

    Lst_Destroy(cleanup, SuffFreeSrc);
}
/*-
 *-----------------------------------------------------------------------
 * Suff_SetNull -- define which suffix is the .NULL suffix.
 *
 * Input:
 *	name	Name of null suffix
 *-----------------------------------------------------------------------
 */
void
Suff_SetNull(char *name)
{
    LstNode i;

    i = Lst_Find(sufflist, name, SuffSuffHasNameP);
    if (i != NULL) {
	if (nullSuff != NULL)
	    --nullSuff->refCount;
	nullSuff = (Suff *)Lst_Datum(i);
	++nullSuff->refCount;
    }
    else
	Parse_Error(PARSE_WARNING,
	    ".NULL not set because %s is not in .SUFFIXES.", name);
}

/*-
 *-----------------------------------------------------------------------
 * Suff_Init --
 *	Initialize suffixes module so all of its services can be used.
 *-----------------------------------------------------------------------
 */
void
Suff_Init(void)
{
    sufflist = Lst_Init(FALSE);
    /*
     * Do explicit initialization for .SUFFIXES related things with static
     * initialization because we get called by Suff_ClearSuffixes() too.
     */
    sNum = 0;
    memset(lookup, 0, sizeof(lookup[0]) * LOOKUP_SIZE);

    transforms = Lst_Init(FALSE);

    emptySuff = SuffNewSuff("");
    emptySuff->sNum = INT_MAX;
    ++emptySuff->refCount;
    Dir_Concat(emptySuff->searchPath, dirSearchPath);

    nullSuff = NULL;
}


/*-
 *----------------------------------------------------------------------
 * Suff_End -- release resources used by the module.
 *
 * It is not safe to call functions from this module after calling this.
 *----------------------------------------------------------------------
 */
void
Suff_End(void)
{
#ifdef CLEANUP
    SuffCleanUp(SCU_END);
#endif
}


/*
 ******************************************************************************
 *			Debugging Functions
 ******************************************************************************
 */

/*
 *----------------------------------------------------------------------
 * SuffDebug -- print a message to debug_file if debugging is enabled.
 *
 * Input:
 *	fmt	printf format specification
 *	...	print arguments
 *
 * Results:
 *	See vfprintf().
 *----------------------------------------------------------------------
 */
static int
SuffDebug(const char * fmt, ...)
{
    va_list ap;
    int rv;

    rv = 0;
    if (DEBUG(SUFF)) {
	va_start(ap, fmt);
	rv = vfprintf(debug_file, fmt, ap);
	va_end(ap);
    }
    return rv;
}

/*
 *----------------------------------------------------------------------
 * SuffDebugChain -- print transformation chain to debug_file.
 *
 * Print the transformation chain that begins with start in reverse
 * order (end result first).  Each suffix in the chain is printed
 * on one line separated by " <- ".
 *
 * Input:
 *	start	chain's first transformation
 *----------------------------------------------------------------------
 */
static void
SuffDebugChain(Src *start)
{
    if (DEBUG(SUFF)){
	Lst tmp = Lst_Init(FALSE);
	Src *i;
	LstNode j;

	for (i = start; i != NULL; i = i->parent)
	    Lst_AtFront(tmp, i);
	fprintf(debug_file, "\t");
	for (j = Lst_First(tmp); j != NULL; j = Lst_Succ(j)) {
	    i = (Src *)Lst_Datum(j);
	    fprintf(debug_file, "`%s' <- ", i->suff->name);
	}
	fprintf(debug_file, "\n");

	Lst_Destroy(tmp, NULL);
    }
}

static int SuffPrintName(void *s, void *unused)
{
    (void)unused;
    fprintf(debug_file, "`%s' ", ((Suff *)s)->name);
    return 0;
}

static int
SuffPrintSuff(void *sp, void *unused)
{
    Suff    *s = (Suff *)sp;
    int	    flags;
    int	    flag;

    (void)unused;

    fprintf(debug_file, "# `%s' [%d] ", s->name, s->refCount);

    flags = s->flags;
    if (flags) {
	fputs(" (", debug_file);
	while (flags) {
	    flag = 1 << (ffs(flags) - 1);
	    flags &= ~flag;
	    switch (flag) {
		case SUFF_INCLUDE:
		    fprintf(debug_file, "INCLUDE");
		    break;
		case SUFF_LIBRARY:
		    fprintf(debug_file, "LIBRARY");
		    break;
	    }
	    fputc(flags ? '|' : ')', debug_file);
	}
    }
    fputc('\n', debug_file);
    fprintf(debug_file, "#\tTo: ");
    Lst_ForEach(s->parents, SuffPrintName, NULL);
    fputc('\n', debug_file);
    fprintf(debug_file, "#\tFrom: ");
    Lst_ForEach(s->children, SuffPrintName, NULL);
    fputc('\n', debug_file);
    fprintf(debug_file, "#\tSearch Path: ");
    Dir_PrintPath(s->searchPath);
    fputc('\n', debug_file);
    return 0;
}

static int
SuffPrintTrans(void *tp, void *unused)
{
    GNode   *t = (GNode *)tp;

    (void)unused;

    fprintf(debug_file, "%-16s: ", t->name);
    Targ_PrintType(t->type);
    fputc('\n', debug_file);
    Lst_ForEach(t->commands, Targ_PrintCmd, NULL);
    fputc('\n', debug_file);
    return 0;
}

void
Suff_PrintAll(void)
{
    fprintf(debug_file, "#*** Suffixes:\n");
    Lst_ForEach(sufflist, SuffPrintSuff, NULL);
    SuffPrintSuff(emptySuff, NULL);

    fprintf(debug_file, "#*** Transformations:\n");
    Lst_ForEach(transforms, SuffPrintTrans, NULL);
}
