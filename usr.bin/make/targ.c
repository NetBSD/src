/*	$NetBSD: targ.c,v 1.43 2006/10/15 08:38:22 dsl Exp $	*/

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
static char rcsid[] = "$NetBSD: targ.c,v 1.43 2006/10/15 08:38:22 dsl Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)targ.c	8.2 (Berkeley) 3/19/94";
#else
__RCSID("$NetBSD: targ.c,v 1.43 2006/10/15 08:38:22 dsl Exp $");
#endif
#endif /* not lint */
#endif

/*-
 * targ.c --
 *	Functions for maintaining the Lst allTargets. Target nodes are
 * kept in two structures: a Lst, maintained by the list library, and a
 * hash table, maintained by the hash library.
 *
 * Interface:
 *	Targ_Init 	    	Initialization procedure.
 *
 *	Targ_End 	    	Cleanup the module
 *
 *	Targ_List 	    	Return the list of all targets so far.
 *
 *	Targ_NewGN	    	Create a new GNode for the passed target
 *	    	  	    	(string). The node is *not* placed in the
 *	    	  	    	hash table, though all its fields are
 *	    	  	    	initialized.
 *
 *	Targ_FindNode	    	Find the node for a given target, creating
 *	    	  	    	and storing it if it doesn't exist and the
 *	    	  	    	flags are right (TARG_CREATE)
 *
 *	Targ_FindList	    	Given a list of names, find nodes for all
 *	    	  	    	of them. If a name doesn't exist and the
 *	    	  	    	TARG_NOCREATE flag was given, an error message
 *	    	  	    	is printed. Else, if a name doesn't exist,
 *	    	  	    	its node is created.
 *
 *	Targ_Ignore	    	Return TRUE if errors should be ignored when
 *	    	  	    	creating the given target.
 *
 *	Targ_Silent	    	Return TRUE if we should be silent when
 *	    	  	    	creating the given target.
 *
 *	Targ_Precious	    	Return TRUE if the target is precious and
 *	    	  	    	should not be removed if we are interrupted.
 *
 *	Targ_Propagate		Propagate information between related
 *				nodes.	Should be called after the
 *				makefiles are parsed but before any
 *				action is taken.
 *
 * Debugging:
 *	Targ_PrintGraph	    	Print out the entire graphm all variables
 *	    	  	    	and statistics for the directory cache. Should
 *	    	  	    	print something for suffixes, too, but...
 */

#include	  <stdio.h>
#include	  <time.h>

#include	  "make.h"
#include	  "hash.h"
#include	  "dir.h"

static Lst        allTargets;	/* the list of all targets found so far */
#ifdef CLEANUP
static Lst	  allGNs;	/* List of all the GNodes */
#endif
static Hash_Table targets;	/* a hash table of same */

#define HTSIZE	191		/* initial size of hash table */

static int TargPrintOnlySrc(ClientData, ClientData);
static int TargPrintName(ClientData, ClientData);
#ifdef CLEANUP
static void TargFreeGN(ClientData);
#endif
static int TargPropagateCohort(ClientData, ClientData);
static int TargPropagateNode(ClientData, ClientData);

/*-
 *-----------------------------------------------------------------------
 * Targ_Init --
 *	Initialize this module
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	The allTargets list and the targets hash table are initialized
 *-----------------------------------------------------------------------
 */
void
Targ_Init(void)
{
    allTargets = Lst_Init(FALSE);
    Hash_InitTable(&targets, HTSIZE);
}

/*-
 *-----------------------------------------------------------------------
 * Targ_End --
 *	Finalize this module
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	All lists and gnodes are cleared
 *-----------------------------------------------------------------------
 */
void
Targ_End(void)
{
#ifdef CLEANUP
    Lst_Destroy(allTargets, NOFREE);
    if (allGNs)
	Lst_Destroy(allGNs, TargFreeGN);
    Hash_DeleteTable(&targets);
#endif
}

/*-
 *-----------------------------------------------------------------------
 * Targ_List --
 *	Return the list of all targets
 *
 * Results:
 *	The list of all targets.
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
Lst
Targ_List(void)
{
    return allTargets;
}

/*-
 *-----------------------------------------------------------------------
 * Targ_NewGN  --
 *	Create and initialize a new graph node
 *
 * Input:
 *	name		the name to stick in the new node
 *
 * Results:
 *	An initialized graph node with the name field filled with a copy
 *	of the passed name
 *
 * Side Effects:
 *	The gnode is added to the list of all gnodes.
 *-----------------------------------------------------------------------
 */
GNode *
Targ_NewGN(const char *name)
{
    GNode *gn;

    gn = emalloc(sizeof(GNode));
    gn->name = estrdup(name);
    gn->uname = NULL;
    gn->path = NULL;
    if (name[0] == '-' && name[1] == 'l') {
	gn->type = OP_LIB;
    } else {
	gn->type = 0;
    }
    gn->unmade =    	0;
    gn->unmade_cohorts = 0;
    gn->centurion =    	NULL;
    gn->made = 	    	UNMADE;
    gn->flags = 	0;
    gn->order =		0;
    gn->mtime = gn->cmtime = 0;
    gn->iParents =  	Lst_Init(FALSE);
    gn->cohorts =   	Lst_Init(FALSE);
    gn->parents =   	Lst_Init(FALSE);
    gn->ancestors =   	Lst_Init(FALSE);
    gn->children =  	Lst_Init(FALSE);
    gn->successors = 	Lst_Init(FALSE);
    gn->preds =     	Lst_Init(FALSE);
    gn->recpreds =	Lst_Init(FALSE);
    Hash_InitTable(&gn->context, 0);
    gn->commands =  	Lst_Init(FALSE);
    gn->suffix =	NULL;
    gn->lineno =	0;
    gn->fname = 	NULL;

#ifdef CLEANUP
    if (allGNs == NULL)
	allGNs = Lst_Init(FALSE);
    Lst_AtEnd(allGNs, (ClientData) gn);
#endif

    return (gn);
}

#ifdef CLEANUP
/*-
 *-----------------------------------------------------------------------
 * TargFreeGN  --
 *	Destroy a GNode
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *-----------------------------------------------------------------------
 */
static void
TargFreeGN(ClientData gnp)
{
    GNode *gn = (GNode *)gnp;


    free(gn->name);
    if (gn->uname)
	free(gn->uname);
    if (gn->path)
	free(gn->path);
    if (gn->fname)
	free(gn->fname);

    Lst_Destroy(gn->iParents, NOFREE);
    Lst_Destroy(gn->cohorts, NOFREE);
    Lst_Destroy(gn->parents, NOFREE);
    Lst_Destroy(gn->ancestors, NOFREE);
    Lst_Destroy(gn->children, NOFREE);
    Lst_Destroy(gn->successors, NOFREE);
    Lst_Destroy(gn->preds, NOFREE);
    Lst_Destroy(gn->recpreds, NOFREE);
    Hash_DeleteTable(&gn->context);
    Lst_Destroy(gn->commands, NOFREE);
    free(gn);
}
#endif


/*-
 *-----------------------------------------------------------------------
 * Targ_FindNode  --
 *	Find a node in the list using the given name for matching
 *
 * Input:
 *	name		the name to find
 *	flags		flags governing events when target not
 *			found
 *
 * Results:
 *	The node in the list if it was. If it wasn't, return NILGNODE of
 *	flags was TARG_NOCREATE or the newly created and initialized node
 *	if it was TARG_CREATE
 *
 * Side Effects:
 *	Sometimes a node is created and added to the list
 *-----------------------------------------------------------------------
 */
GNode *
Targ_FindNode(const char *name, int flags)
{
    GNode         *gn;	      /* node in that element */
    Hash_Entry	  *he;	      /* New or used hash entry for node */
    Boolean	  isNew;      /* Set TRUE if Hash_CreateEntry had to create */
			      /* an entry for the node */


    if (flags & TARG_CREATE) {
	he = Hash_CreateEntry(&targets, name, &isNew);
	if (isNew) {
	    gn = Targ_NewGN(name);
	    Hash_SetValue(he, gn);
	    Var_Append(".ALLTARGETS", name, VAR_GLOBAL);
	    (void)Lst_AtEnd(allTargets, (ClientData)gn);
	}
    } else {
	he = Hash_FindEntry(&targets, name);
    }

    if (he == NULL) {
	return (NILGNODE);
    } else {
	return ((GNode *)Hash_GetValue(he));
    }
}

/*-
 *-----------------------------------------------------------------------
 * Targ_FindList --
 *	Make a complete list of GNodes from the given list of names
 *
 * Input:
 *	name		list of names to find
 *	flags		flags used if no node is found for a given name
 *
 * Results:
 *	A complete list of graph nodes corresponding to all instances of all
 *	the names in names.
 *
 * Side Effects:
 *	If flags is TARG_CREATE, nodes will be created for all names in
 *	names which do not yet have graph nodes. If flags is TARG_NOCREATE,
 *	an error message will be printed for each name which can't be found.
 * -----------------------------------------------------------------------
 */
Lst
Targ_FindList(Lst names, int flags)
{
    Lst            nodes;	/* result list */
    LstNode	   ln;		/* name list element */
    GNode	   *gn;		/* node in tLn */
    char    	   *name;

    nodes = Lst_Init(FALSE);

    if (Lst_Open(names) == FAILURE) {
	return (nodes);
    }
    while ((ln = Lst_Next(names)) != NILLNODE) {
	name = (char *)Lst_Datum(ln);
	gn = Targ_FindNode(name, flags);
	if (gn != NILGNODE) {
	    /*
	     * Note: Lst_AtEnd must come before the Lst_Concat so the nodes
	     * are added to the list in the order in which they were
	     * encountered in the makefile.
	     */
	    (void)Lst_AtEnd(nodes, (ClientData)gn);
	} else if (flags == TARG_NOCREATE) {
	    Error("\"%s\" -- target unknown.", name);
	}
    }
    Lst_Close(names);
    return (nodes);
}

/*-
 *-----------------------------------------------------------------------
 * Targ_Ignore  --
 *	Return true if should ignore errors when creating gn
 *
 * Input:
 *	gn		node to check for
 *
 * Results:
 *	TRUE if should ignore errors
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
Boolean
Targ_Ignore(GNode *gn)
{
    if (ignoreErrors || gn->type & OP_IGNORE) {
	return (TRUE);
    } else {
	return (FALSE);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Targ_Silent  --
 *	Return true if be silent when creating gn
 *
 * Input:
 *	gn		node to check for
 *
 * Results:
 *	TRUE if should be silent
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
Boolean
Targ_Silent(GNode *gn)
{
    if (beSilent || gn->type & OP_SILENT) {
	return (TRUE);
    } else {
	return (FALSE);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Targ_Precious --
 *	See if the given target is precious
 *
 * Input:
 *	gn		the node to check
 *
 * Results:
 *	TRUE if it is precious. FALSE otherwise
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
Boolean
Targ_Precious(GNode *gn)
{
    if (allPrecious || (gn->type & (OP_PRECIOUS|OP_DOUBLEDEP))) {
	return (TRUE);
    } else {
	return (FALSE);
    }
}

/******************* DEBUG INFO PRINTING ****************/

static GNode	  *mainTarg;	/* the main target, as set by Targ_SetMain */
/*-
 *-----------------------------------------------------------------------
 * Targ_SetMain --
 *	Set our idea of the main target we'll be creating. Used for
 *	debugging output.
 *
 * Input:
 *	gn		The main target we'll create
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	"mainTarg" is set to the main target's node.
 *-----------------------------------------------------------------------
 */
void
Targ_SetMain(GNode *gn)
{
    mainTarg = gn;
}

#define PrintWait (ClientData)1
#define PrintPath (ClientData)2

static int
TargPrintName(ClientData gnp, ClientData pflags)
{
    static int last_order;
    GNode *gn = (GNode *)gnp;

    if (pflags == PrintWait && gn->order > last_order)
	fprintf(debug_file, ".WAIT ");
    last_order = gn->order;

    fprintf(debug_file, "%s ", gn->name);

#ifdef notdef
    if (pflags == PrintPath) {
	if (gn->path) {
	    fprintf(debug_file, "[%s]  ", gn->path);
	}
	if (gn == mainTarg) {
	    fprintf(debug_file, "(MAIN NAME)  ");
	}
    }
#endif /* notdef */

    return 0;
}


int
Targ_PrintCmd(ClientData cmd, ClientData dummy)
{
    fprintf(debug_file, "\t%s\n", (char *)cmd);
    return (dummy ? 0 : 0);
}

/*-
 *-----------------------------------------------------------------------
 * Targ_FmtTime --
 *	Format a modification time in some reasonable way and return it.
 *
 * Results:
 *	The time reformatted.
 *
 * Side Effects:
 *	The time is placed in a static area, so it is overwritten
 *	with each call.
 *
 *-----------------------------------------------------------------------
 */
char *
Targ_FmtTime(time_t tm)
{
    struct tm	  	*parts;
    static char	  	buf[128];

    parts = localtime(&tm);
    (void)strftime(buf, sizeof buf, "%k:%M:%S %b %d, %Y", parts);
    return(buf);
}

/*-
 *-----------------------------------------------------------------------
 * Targ_PrintType --
 *	Print out a type field giving only those attributes the user can
 *	set.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
Targ_PrintType(int type)
{
    int    tbit;

#define PRINTBIT(attr)	case CONCAT(OP_,attr): fprintf(debug_file, "." #attr " "); break
#define PRINTDBIT(attr) case CONCAT(OP_,attr): if (DEBUG(TARG))fprintf(debug_file, "." #attr " "); break

    type &= ~OP_OPMASK;

    while (type) {
	tbit = 1 << (ffs(type) - 1);
	type &= ~tbit;

	switch(tbit) {
	    PRINTBIT(OPTIONAL);
	    PRINTBIT(USE);
	    PRINTBIT(EXEC);
	    PRINTBIT(IGNORE);
	    PRINTBIT(PRECIOUS);
	    PRINTBIT(SILENT);
	    PRINTBIT(MAKE);
	    PRINTBIT(JOIN);
	    PRINTBIT(INVISIBLE);
	    PRINTBIT(NOTMAIN);
	    PRINTDBIT(LIB);
	    /*XXX: MEMBER is defined, so CONCAT(OP_,MEMBER) gives OP_"%" */
	    case OP_MEMBER: if (DEBUG(TARG))fprintf(debug_file, ".MEMBER "); break;
	    PRINTDBIT(ARCHV);
	    PRINTDBIT(MADE);
	    PRINTDBIT(PHONY);
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * TargPrintNode --
 *	print the contents of a node
 *-----------------------------------------------------------------------
 */
int
Targ_PrintNode(ClientData gnp, ClientData passp)
{
    GNode         *gn = (GNode *)gnp;
    int	    	  pass = passp ? *(int *)passp : 0;
    if (!OP_NOP(gn->type)) {
	fprintf(debug_file, "#\n");
	if (gn == mainTarg) {
	    fprintf(debug_file, "# *** MAIN TARGET ***\n");
	}
	if (pass == 2) {
	    if (gn->unmade) {
		fprintf(debug_file, "# %d unmade children\n", gn->unmade);
	    } else {
		fprintf(debug_file, "# No unmade children\n");
	    }
	    if (! (gn->type & (OP_JOIN|OP_USE|OP_USEBEFORE|OP_EXEC))) {
		if (gn->mtime != 0) {
		    fprintf(debug_file, "# last modified %s: %s\n",
			      Targ_FmtTime(gn->mtime),
			      (gn->made == UNMADE ? "unmade" :
			       (gn->made == MADE ? "made" :
				(gn->made == UPTODATE ? "up-to-date" :
				 "error when made"))));
		} else if (gn->made != UNMADE) {
		    fprintf(debug_file, "# non-existent (maybe): %s\n",
			      (gn->made == MADE ? "made" :
			       (gn->made == UPTODATE ? "up-to-date" :
				(gn->made == ERROR ? "error when made" :
				 "aborted"))));
		} else {
		    fprintf(debug_file, "# unmade\n");
		}
	    }
	    if (!Lst_IsEmpty (gn->iParents)) {
		fprintf(debug_file, "# implicit parents: ");
		Lst_ForEach(gn->iParents, TargPrintName, (ClientData)0);
		fprintf(debug_file, "\n");
	    }
	} else {
	    if (gn->unmade)
		fprintf(debug_file, "# %d unmade children\n", gn->unmade);
	}
	if (!Lst_IsEmpty (gn->parents)) {
	    fprintf(debug_file, "# parents: ");
	    Lst_ForEach(gn->parents, TargPrintName, (ClientData)0);
	    fprintf(debug_file, "\n");
	}
	if (!Lst_IsEmpty (gn->children)) {
	    fprintf(debug_file, "# children: ");
	    Lst_ForEach(gn->children, TargPrintName, (ClientData)0);
	    fprintf(debug_file, "\n");
	}
	if (!Lst_IsEmpty (gn->preds)) {
	    fprintf(debug_file, "# preds: ");
	    Lst_ForEach(gn->preds, TargPrintName, (ClientData)0);
	    fprintf(debug_file, "\n");
	}
	if (!Lst_IsEmpty (gn->recpreds)) {
	    fprintf(debug_file, "# recpreds: ");
	    Lst_ForEach(gn->recpreds, TargPrintName, (ClientData)0);
	    fprintf(debug_file, "\n");
	}
	if (!Lst_IsEmpty (gn->successors)) {
	    fprintf(debug_file, "# successors: ");
	    Lst_ForEach(gn->successors, TargPrintName, (ClientData)0);
	    fprintf(debug_file, "\n");
	}

	fprintf(debug_file, "%-16s", gn->name);
	switch (gn->type & OP_OPMASK) {
	    case OP_DEPENDS:
		fprintf(debug_file, ": "); break;
	    case OP_FORCE:
		fprintf(debug_file, "! "); break;
	    case OP_DOUBLEDEP:
		fprintf(debug_file, ":: "); break;
	}
	Targ_PrintType(gn->type);
	Lst_ForEach(gn->children, TargPrintName, PrintWait);
	fprintf(debug_file, "\n");
	Lst_ForEach(gn->commands, Targ_PrintCmd, (ClientData)0);
	fprintf(debug_file, "\n\n");
	if (gn->type & OP_DOUBLEDEP) {
	    Lst_ForEach(gn->cohorts, Targ_PrintNode, (ClientData)&pass);
	}
    }
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * TargPrintOnlySrc --
 *	Print only those targets that are just a source.
 *
 * Results:
 *	0.
 *
 * Side Effects:
 *	The name of each file is printed preceded by #\t
 *
 *-----------------------------------------------------------------------
 */
static int
TargPrintOnlySrc(ClientData gnp, ClientData dummy)
{
    GNode   	  *gn = (GNode *)gnp;
    if (OP_NOP(gn->type))
	fprintf(debug_file, "#\t%s [%s]\n", gn->name, gn->path ? gn->path : gn->name);

    return (dummy ? 0 : 0);
}

/*-
 *-----------------------------------------------------------------------
 * Targ_PrintGraph --
 *	print the entire graph. heh heh
 *
 * Input:
 *	pass		Which pass this is. 1 => no processing
 *			2 => processing done
 *
 * Results:
 *	none
 *
 * Side Effects:
 *	lots o' output
 *-----------------------------------------------------------------------
 */
void
Targ_PrintGraph(int pass)
{
    fprintf(debug_file, "#*** Input graph:\n");
    Lst_ForEach(allTargets, Targ_PrintNode, (ClientData)&pass);
    fprintf(debug_file, "\n\n");
    fprintf(debug_file, "#\n#   Files that are only sources:\n");
    Lst_ForEach(allTargets, TargPrintOnlySrc, (ClientData) 0);
    fprintf(debug_file, "#*** Global Variables:\n");
    Var_Dump(VAR_GLOBAL);
    fprintf(debug_file, "#*** Command-line Variables:\n");
    Var_Dump(VAR_CMD);
    fprintf(debug_file, "\n");
    Dir_PrintDirectories();
    fprintf(debug_file, "\n");
    Suff_PrintAll();
}

/*-
 *-----------------------------------------------------------------------
 * TargAppendAncestor -
 *	Appends a single ancestor to a node's list of ancestors,
 *	ignoring duplicates.
 *
 * Input:
 *	ancestorgnp	An ancestor to be added to a list.
 *	thisgnp		The node whose ancestor list will be changed.
 *
 * Results:
 *	Always returns 0, for the benefit of Lst_ForEach().
 *
 * Side Effects:
 *	May modify the ancestors list of the node we are
 *	examining.
 *-----------------------------------------------------------------------
 */
static int
TargAppendAncestor(ClientData ancestorgnp, ClientData thisgnp)
{
    GNode	  *ancestorgn = (GNode *)ancestorgnp;
    GNode	  *thisgn = (GNode *)thisgnp;

    if (Lst_Member(thisgn->ancestors, ancestorgn) == NILLNODE) {
	(void)Lst_AtEnd(thisgn->ancestors, (ClientData)ancestorgn);
    }
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * TargAppendParentAncestors -
 *	Appends all ancestors of a parent node to the ancestor list of a
 *	given node, ignoring duplicates.
 *
 * Input:
 *	parentgnp	A parent node whose ancestor list will be
 *			propagated to another node.
 *	thisgnp		The node whose ancestor list will be changed.
 *
 * Results:
 *	Always returns 0, for the benefit of Lst_ForEach().
 *
 * Side Effects:
 *	May modify the ancestors list of the node we are
 *	examining.
 *-----------------------------------------------------------------------
 */
static int
TargAppendParentAncestors(ClientData parentgnp, ClientData thisgnp)
{
    GNode	  *parentgn = (GNode *)parentgnp;
    GNode	  *thisgn = (GNode *)thisgnp;

    Lst_ForEach(parentgn->ancestors, TargAppendAncestor, thisgn);
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * TargInitAncestors -
 *	Initialises the ancestor list of a node and all the
 *	node's ancestors.
 *
 * Input:
 *	thisgnp		The node that we are examining.
 *
 * Results:
 *	Always returns 0, for the benefit of Lst_ForEach().
 *
 * Side Effects:
 *	May initialise the ancestors list of the node we are
 *	examining and all its ancestors.  Does nothing if the
 *	list has already been initialised.
 *-----------------------------------------------------------------------
 */
static int
TargInitAncestors(ClientData thisgnp, ClientData junk __unused)
{
    GNode	  *thisgn = (GNode *)thisgnp;

    if (Lst_IsEmpty (thisgn->ancestors)) {
	/*
	 * Add our parents to our ancestor list before recursing, to
	 * ensure that loops in the dependency graph will not result in
	 * infinite recursion.
	 */
	Lst_ForEach(thisgn->parents, TargAppendAncestor, thisgn);
	Lst_ForEach(thisgn->iParents, TargAppendAncestor, thisgn);
	/* Recursively initialise our parents' ancestor lists */
	Lst_ForEach(thisgn->parents, TargInitAncestors, (ClientData)0);
	Lst_ForEach(thisgn->iParents, TargInitAncestors, (ClientData)0);
	/* Our parents' ancestors are also our ancestors */
	Lst_ForEach(thisgn->parents, TargAppendParentAncestors, thisgn);
	Lst_ForEach(thisgn->iParents, TargAppendParentAncestors, thisgn);
    }
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * TargHasAncestor -
 *	Checks whether one node is an ancestor of another node.
 *
 *	If called with both arguments pointing to the
 *	same node, checks whether the node is part of a cycle
 *	in the graph.
 *
 * Input:
 *	thisgnp		The node whose ancestor list we are examining.
 *	seekgnp		The node that we are seeking in the
 *			ancestor list.
 *
 * Results:
 *	TRUE if seekgn is an ancestor of thisgn; FALSE if not.
 *
 * Side Effects:
 *	Initialises the ancestors list in thisgnp and all its ancestors.
 *-----------------------------------------------------------------------
 */
static Boolean
TargHasAncestor(ClientData thisgnp, ClientData seekgnp)
{
    GNode	  *thisgn = (GNode *)thisgnp;
    GNode	  *seekgn = (GNode *)seekgnp;

    TargInitAncestors(thisgn, (ClientData)0);
    if (Lst_Member(thisgn->ancestors, seekgn) != NILLNODE) {
	return (TRUE);
    }
    return (FALSE);
}

/*-
 *-----------------------------------------------------------------------
 * TargPropagateRecpredChild --
 *	Create a new predecessor/successor relationship between a pair
 *	of nodes, and recursively propagate the relationship to all
 *	children of the successor node.
 *
 *	If there is already a predecessor/successor relationship
 *	in the opposite direction, or if there is already an
 *	ancestor/descendent relationship in the oposite direction, then
 *	we avoid adding the new relationship, because that would cause a
 *	cycle in the graph.
 *
 * Input:
 *	succgnp		Successor node.
 *	predgnp		Predecessor node.
 *
 * Results:
 *	Always returns 0, for the benefit of Lst_ForEach().
 *
 * Side Effects:
 *	preds/successors information is modified for predgnp, succgnp,
 *	and recursively for all children of succgnp.
 *-----------------------------------------------------------------------
 */
static int
TargPropagateRecpredChild(ClientData succgnp, ClientData predgnp)
{
    GNode	  *succgn = (GNode *)succgnp;
    GNode	  *predgn = (GNode *)predgnp;
    Boolean	  debugmore = FALSE;	/* how much debugging? */

    /* Ignore if succgn == predgn */
    if (succgn == predgn) {
	if (DEBUG(TARG) && debugmore) {
	    fprintf(debug_file, "# TargPropagateRecpredChild: not propagating %s - %s (identical)\n",
		    predgn->name, succgn->name);
	}
	return (0);
    }
    /* Pre-existing pred/successor relationship
     * in the opposite direction takes precedence. */
    if (Lst_Member(succgn->successors, predgn) != NILLNODE) {
	if (DEBUG(TARG) && debugmore) {
	    fprintf(debug_file, "# TargPropagateRecpredChild: not propagating %s - %s (opposite)\n",
		    predgn->name, succgn->name);
	}
	return (0);
    }
    /* Pre-existing descendent/ancestor relationship in the opposite
     * direction takes precedence. */
    if (TargHasAncestor(succgn, predgn)) {
	if (DEBUG(TARG) && debugmore) {
	    fprintf(debug_file, "# TargPropagateRecpredChild: not propagating %s - %s (ancestor)\n",
		    predgn->name, succgn->name);
	}
	return (0);
    }
    /* Note the new pred/successor relationship. */
    if (DEBUG(TARG)) {
	fprintf(debug_file, "# TargPropagateRecpredChild: propagating %s - %s\n",
		predgn->name, succgn->name);
    }
    if (Lst_Member(succgn->preds, predgn) == NILLNODE) {
	(void)Lst_AtEnd(succgn->preds, (ClientData)predgn);
	(void)Lst_AtEnd(predgn->successors, (ClientData)succgn);
    }
    /* Recurse, provided there's not a cycle. */
    if (! TargHasAncestor(succgn, succgn)) {
	Lst_ForEach(succgn->children, TargPropagateRecpredChild, predgn);
    }
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * TargPropagateRecpred --
 *	Recursively propagate information about a single predecessor
 *	node, from a single successor node to all children of the
 *	successor node.
 *
 * Input:
 *	predgnp		Predecessor node.
 *	succgnp		Successor node.
 *
 * Results:
 *	Always returns 0, for the benefit of Lst_ForEach().
 *
 * Side Effects:
 *	preds/successors information is modified for predgnp, succgnp,
 *	and recursively for all children of succgnp.
 *	
 *	The real work is done by TargPropagateRecpredChild(), which
 *	will be called for each child of the successor node.
 *-----------------------------------------------------------------------
 */
static int
TargPropagateRecpred(ClientData predgnp, ClientData succgnp)
{
    GNode	  *predgn = (GNode *)predgnp;
    GNode	  *succgn = (GNode *)succgnp;

    Lst_ForEach(succgn->children, TargPropagateRecpredChild, predgn);
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * TargPropagateNode --
 *	Propagate information from a single node to related nodes if
 *	appropriate.
 *
 * Input:
 *	gnp		The node that we are processing.
 *
 * Results:
 *	Always returns 0, for the benefit of Lst_ForEach().
 *
 * Side Effects:
 *	Information is propagated from this node to cohort or child
 *	nodes.
 *
 *	If the node was defined with "::", then TargPropagateCohort()
 *	will be called for each cohort node.
 *
 *	If the node has recursive predecessors, then
 *	TargPropagateRecpred() will be called for each recursive
 *	predecessor.
 *-----------------------------------------------------------------------
 */
static int
TargPropagateNode(ClientData gnp, ClientData junk __unused)
{
    GNode	  *gn = (GNode *)gnp;
    if (gn->type & OP_DOUBLEDEP)
	Lst_ForEach(gn->cohorts, TargPropagateCohort, gnp);
    Lst_ForEach(gn->recpreds, TargPropagateRecpred, gnp);
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * TargPropagateCohort --
 *	Propagate some bits in the type mask from a node to
 *	a related cohort node.
 *
 * Input:
 *	cnp		The node that we are processing.
 *	gnp		Another node that has cnp as a cohort.
 *
 * Results:
 *	Always returns 0, for the benefit of Lst_ForEach().
 *
 * Side Effects:
 *	cnp's type bitmask is modified to incorporate some of the
 *	bits from gnp's type bitmask.  (XXX need a better explanation.)
 *-----------------------------------------------------------------------
 */
static int
TargPropagateCohort(ClientData cgnp, ClientData pgnp)
{
    GNode	  *cgn = (GNode *)cgnp;
    GNode	  *pgn = (GNode *)pgnp;

    cgn->type |= pgn->type & ~OP_OPMASK;
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * Targ_Propagate --
 *	Propagate information between related nodes.  Should be called
 *	after the makefiles are parsed but before any action is taken.
 *
 * Results:
 *	none
 *
 * Side Effects:
 *	Information is propagated between related nodes throughout the
 *	graph.
 *-----------------------------------------------------------------------
 */
void
Targ_Propagate(void)
{
    Lst_ForEach(allTargets, TargPropagateNode, (ClientData)0);
}
