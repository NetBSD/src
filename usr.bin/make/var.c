/*	$NetBSD: var.c,v 1.344 2020/07/27 22:30:00 rillig Exp $	*/

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
static char rcsid[] = "$NetBSD: var.c,v 1.344 2020/07/27 22:30:00 rillig Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)var.c	8.3 (Berkeley) 3/19/94";
#else
__RCSID("$NetBSD: var.c,v 1.344 2020/07/27 22:30:00 rillig Exp $");
#endif
#endif /* not lint */
#endif

/*-
 * var.c --
 *	Variable-handling functions
 *
 * Interface:
 *	Var_Set		    Set the value of a variable in the given
 *			    context. The variable is created if it doesn't
 *			    yet exist.
 *
 *	Var_Append	    Append more characters to an existing variable
 *			    in the given context. The variable needn't
 *			    exist already -- it will be created if it doesn't.
 *			    A space is placed between the old value and the
 *			    new one.
 *
 *	Var_Exists	    See if a variable exists.
 *
 *	Var_Value 	    Return the unexpanded value of a variable in a
 *			    context or NULL if the variable is undefined.
 *
 *	Var_Subst 	    Substitute either a single variable or all
 *			    variables in a string, using the given context.
 *
 *	Var_Parse 	    Parse a variable expansion from a string and
 *			    return the result and the number of characters
 *			    consumed.
 *
 *	Var_Delete	    Delete a variable in a context.
 *
 *	Var_Init  	    Initialize this module.
 *
 * Debugging:
 *	Var_Dump  	    Print out all variables defined in the given
 *			    context.
 *
 * XXX: There's a lot of duplication in these functions.
 */

#include    <sys/stat.h>
#ifndef NO_REGEX
#include    <sys/types.h>
#include    <regex.h>
#endif
#include    <ctype.h>
#include    <inttypes.h>
#include    <limits.h>
#include    <stdlib.h>
#include    <time.h>

#include    "make.h"
#include    "buf.h"
#include    "dir.h"
#include    "job.h"
#include    "metachar.h"

#define VAR_DEBUG(fmt, ...)	\
    if (!DEBUG(VAR))		\
	(void) 0;		\
    else			\
	fprintf(debug_file, fmt, __VA_ARGS__)

/*
 * This lets us tell if we have replaced the original environ
 * (which we cannot free).
 */
char **savedEnv = NULL;

/*
 * This is a harmless return value for Var_Parse that can be used by Var_Subst
 * to determine if there was an error in parsing -- easier than returning
 * a flag, as things outside this module don't give a hoot.
 */
char var_Error[] = "";

/*
 * Similar to var_Error, but returned when the 'VARE_UNDEFERR' flag for
 * Var_Parse is not set. Why not just use a constant? Well, GCC likes
 * to condense identical string instances...
 */
static char varNoError[] = "";

/*
 * Traditionally we consume $$ during := like any other expansion.
 * Other make's do not.
 * This knob allows controlling the behavior.
 * FALSE to consume $$ during := assignment.
 * TRUE to preserve $$ during := assignment.
 */
#define SAVE_DOLLARS ".MAKE.SAVE_DOLLARS"
static Boolean save_dollars = TRUE;

/*
 * Internally, variables are contained in four different contexts.
 *	1) the environment. They cannot be changed. If an environment
 *	    variable is appended to, the result is placed in the global
 *	    context.
 *	2) the global context. Variables set in the Makefile are located in
 *	    the global context.
 *	3) the command-line context. All variables set on the command line
 *	   are placed in this context. They are UNALTERABLE once placed here.
 *	4) the local context. Each target has associated with it a context
 *	   list. On this list are located the structures describing such
 *	   local variables as $(@) and $(*)
 * The four contexts are searched in the reverse order from which they are
 * listed (but see checkEnvFirst).
 */
GNode          *VAR_INTERNAL;	/* variables from make itself */
GNode          *VAR_GLOBAL;	/* variables from the makefile */
GNode          *VAR_CMD;	/* variables defined on the command-line */

typedef enum {
    FIND_CMD		= 0x01,	/* look in VAR_CMD when searching */
    FIND_GLOBAL		= 0x02,	/* look in VAR_GLOBAL as well */
    FIND_ENV		= 0x04	/* look in the environment also */
} VarFindFlags;

typedef enum {
    VAR_IN_USE		= 0x01,	/* Variable's value is currently being used
				 * by Var_Parse or Var_Subst.
				 * Used to avoid endless recursion */
    VAR_FROM_ENV	= 0x02,	/* Variable comes from the environment */
    VAR_JUNK		= 0x04,	/* Variable is a junk variable that
				 * should be destroyed when done with
				 * it. Used by Var_Parse for undefined,
				 * modified variables */
    VAR_KEEP		= 0x08,	/* Variable is VAR_JUNK, but we found
				 * a use for it in some modifier and
				 * the value is therefore valid */
    VAR_EXPORTED	= 0x10,	/* Variable is exported */
    VAR_REEXPORT	= 0x20,	/* Indicate if var needs re-export.
				 * This would be true if it contains $'s */
    VAR_FROM_CMD	= 0x40	/* Variable came from command line */
} Var_Flags;

typedef struct Var {
    char          *name;	/* the variable's name */
    Buffer	  val;		/* its value */
    Var_Flags	  flags;    	/* miscellaneous status flags */
}  Var;

/*
 * Exporting vars is expensive so skip it if we can
 */
typedef enum {
    VAR_EXPORTED_NONE,
    VAR_EXPORTED_YES,
    VAR_EXPORTED_ALL
} VarExportedMode;
static VarExportedMode var_exportedVars = VAR_EXPORTED_NONE;

typedef enum {
    /*
     * We pass this to Var_Export when doing the initial export
     * or after updating an exported var.
     */
    VAR_EXPORT_PARENT	= 0x01,
    /*
     * We pass this to Var_Export1 to tell it to leave the value alone.
     */
    VAR_EXPORT_LITERAL	= 0x02
} VarExportFlags;

/* Flags for pattern matching in the :S and :C modifiers */
typedef enum {
    VARP_SUB_GLOBAL	= 0x01,	/* Apply substitution globally */
    VARP_SUB_ONE	= 0x02,	/* Apply substitution to one word */
    VARP_SUB_MATCHED	= 0x04,	/* There was a match */
    VARP_ANCHOR_START	= 0x08,	/* Match at start of word */
    VARP_ANCHOR_END	= 0x10	/* Match at end of word */
} VarPatternFlags;

typedef enum {
    VAR_NO_EXPORT	= 0x01	/* do not export */
} VarSet_Flags;

#define BROPEN	'{'
#define BRCLOSE	'}'
#define PROPEN	'('
#define PRCLOSE	')'

/*-
 *-----------------------------------------------------------------------
 * VarFind --
 *	Find the given variable in the given context and any other contexts
 *	indicated.
 *
 * Input:
 *	name		name to find
 *	ctxt		context in which to find it
 *	flags		FIND_GLOBAL	look in VAR_GLOBAL as well
 *			FIND_CMD	look in VAR_CMD as well
 *			FIND_ENV	look in the environment as well
 *
 * Results:
 *	A pointer to the structure describing the desired variable or
 *	NULL if the variable does not exist.
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
static Var *
VarFind(const char *name, GNode *ctxt, VarFindFlags flags)
{
    /*
     * If the variable name begins with a '.', it could very well be one of
     * the local ones.  We check the name against all the local variables
     * and substitute the short version in for 'name' if it matches one of
     * them.
     */
    if (*name == '.' && isupper((unsigned char) name[1])) {
	switch (name[1]) {
	case 'A':
	    if (strcmp(name, ".ALLSRC") == 0)
		name = ALLSRC;
	    if (strcmp(name, ".ARCHIVE") == 0)
		name = ARCHIVE;
	    break;
	case 'I':
	    if (strcmp(name, ".IMPSRC") == 0)
		name = IMPSRC;
	    break;
	case 'M':
	    if (strcmp(name, ".MEMBER") == 0)
		name = MEMBER;
	    break;
	case 'O':
	    if (strcmp(name, ".OODATE") == 0)
		name = OODATE;
	    break;
	case 'P':
	    if (strcmp(name, ".PREFIX") == 0)
		name = PREFIX;
	    break;
	case 'T':
	    if (strcmp(name, ".TARGET") == 0)
		name = TARGET;
	    break;
	}
    }

#ifdef notyet
    /* for compatibility with gmake */
    if (name[0] == '^' && name[1] == '\0')
	name = ALLSRC;
#endif

    /*
     * First look for the variable in the given context. If it's not there,
     * look for it in VAR_CMD, VAR_GLOBAL and the environment, in that order,
     * depending on the FIND_* flags in 'flags'
     */
    Hash_Entry *var = Hash_FindEntry(&ctxt->context, name);

    if (var == NULL && (flags & FIND_CMD) && ctxt != VAR_CMD) {
	var = Hash_FindEntry(&VAR_CMD->context, name);
    }
    if (!checkEnvFirst && var == NULL && (flags & FIND_GLOBAL) &&
	ctxt != VAR_GLOBAL)
    {
	var = Hash_FindEntry(&VAR_GLOBAL->context, name);
	if (var == NULL && ctxt != VAR_INTERNAL) {
	    /* VAR_INTERNAL is subordinate to VAR_GLOBAL */
	    var = Hash_FindEntry(&VAR_INTERNAL->context, name);
	}
    }
    if (var == NULL && (flags & FIND_ENV)) {
	char *env;

	if ((env = getenv(name)) != NULL) {
	    Var *v = bmake_malloc(sizeof(Var));
	    v->name = bmake_strdup(name);

	    int len = (int)strlen(env);
	    Buf_Init(&v->val, len + 1);
	    Buf_AddBytes(&v->val, len, env);

	    v->flags = VAR_FROM_ENV;
	    return v;
	} else if (checkEnvFirst && (flags & FIND_GLOBAL) &&
		   ctxt != VAR_GLOBAL)
	{
	    var = Hash_FindEntry(&VAR_GLOBAL->context, name);
	    if (var == NULL && ctxt != VAR_INTERNAL) {
		var = Hash_FindEntry(&VAR_INTERNAL->context, name);
	    }
	    if (var == NULL) {
		return NULL;
	    } else {
		return (Var *)Hash_GetValue(var);
	    }
	} else {
	    return NULL;
	}
    } else if (var == NULL) {
	return NULL;
    } else {
	return (Var *)Hash_GetValue(var);
    }
}

/*-
 *-----------------------------------------------------------------------
 * VarFreeEnv  --
 *	If the variable is an environment variable, free it
 *
 * Input:
 *	v		the variable
 *	destroy		true if the value buffer should be destroyed.
 *
 * Results:
 *	1 if it is an environment variable 0 ow.
 *
 * Side Effects:
 *	The variable is free'ed if it is an environent variable.
 *-----------------------------------------------------------------------
 */
static Boolean
VarFreeEnv(Var *v, Boolean destroy)
{
    if (!(v->flags & VAR_FROM_ENV))
	return FALSE;
    free(v->name);
    Buf_Destroy(&v->val, destroy);
    free(v);
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * VarAdd  --
 *	Add a new variable of name name and value val to the given context
 *
 * Input:
 *	name		name of variable to add
 *	val		value to set it to
 *	ctxt		context in which to set it
 *
 * Side Effects:
 *	The new variable is placed at the front of the given context
 *	The name and val arguments are duplicated so they may
 *	safely be freed.
 *-----------------------------------------------------------------------
 */
static void
VarAdd(const char *name, const char *val, GNode *ctxt)
{
    Var *v = bmake_malloc(sizeof(Var));

    int len = val != NULL ? (int)strlen(val) : 0;
    Buf_Init(&v->val, len + 1);
    Buf_AddBytes(&v->val, len, val);

    v->flags = 0;

    Hash_Entry *h = Hash_CreateEntry(&ctxt->context, name, NULL);
    Hash_SetValue(h, v);
    v->name = h->name;
    if (DEBUG(VAR) && !(ctxt->flags & INTERNAL)) {
	fprintf(debug_file, "%s:%s = %s\n", ctxt->name, name, val);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Var_Delete --
 *	Remove a variable from a context.
 *
 * Side Effects:
 *	The Var structure is removed and freed.
 *
 *-----------------------------------------------------------------------
 */
void
Var_Delete(const char *name, GNode *ctxt)
{
    Hash_Entry 	  *ln;
    char *cp;

    if (strchr(name, '$') != NULL) {
	cp = Var_Subst(NULL, name, VAR_GLOBAL, VARE_WANTRES);
    } else {
	cp = UNCONST(name);
    }
    ln = Hash_FindEntry(&ctxt->context, cp);
    if (DEBUG(VAR)) {
	fprintf(debug_file, "%s:delete %s%s\n",
	    ctxt->name, cp, ln ? "" : " (not found)");
    }
    if (cp != name)
	free(cp);
    if (ln != NULL) {
	Var *v = (Var *)Hash_GetValue(ln);
	if (v->flags & VAR_EXPORTED)
	    unsetenv(v->name);
	if (strcmp(MAKE_EXPORTED, v->name) == 0)
	    var_exportedVars = VAR_EXPORTED_NONE;
	if (v->name != ln->name)
	    free(v->name);
	Hash_DeleteEntry(&ctxt->context, ln);
	Buf_Destroy(&v->val, TRUE);
	free(v);
    }
}


/*
 * Export a var.
 * We ignore make internal variables (those which start with '.')
 * Also we jump through some hoops to avoid calling setenv
 * more than necessary since it can leak.
 * We only manipulate flags of vars if 'parent' is set.
 */
static int
Var_Export1(const char *name, VarExportFlags flags)
{
    char tmp[BUFSIZ];
    Var *v;
    char *val = NULL;
    int n;
    VarExportFlags parent = flags & VAR_EXPORT_PARENT;

    if (*name == '.')
	return 0;		/* skip internals */
    if (!name[1]) {
	/*
	 * A single char.
	 * If it is one of the vars that should only appear in
	 * local context, skip it, else we can get Var_Subst
	 * into a loop.
	 */
	switch (name[0]) {
	case '@':
	case '%':
	case '*':
	case '!':
	    return 0;
	}
    }
    v = VarFind(name, VAR_GLOBAL, 0);
    if (v == NULL)
	return 0;
    if (!parent &&
	(v->flags & (VAR_EXPORTED | VAR_REEXPORT)) == VAR_EXPORTED) {
	return 0;			/* nothing to do */
    }
    val = Buf_GetAll(&v->val, NULL);
    if ((flags & VAR_EXPORT_LITERAL) == 0 && strchr(val, '$')) {
	if (parent) {
	    /*
	     * Flag this as something we need to re-export.
	     * No point actually exporting it now though,
	     * the child can do it at the last minute.
	     */
	    v->flags |= (VAR_EXPORTED | VAR_REEXPORT);
	    return 1;
	}
	if (v->flags & VAR_IN_USE) {
	    /*
	     * We recursed while exporting in a child.
	     * This isn't going to end well, just skip it.
	     */
	    return 0;
	}
	n = snprintf(tmp, sizeof(tmp), "${%s}", name);
	if (n < (int)sizeof(tmp)) {
	    val = Var_Subst(NULL, tmp, VAR_GLOBAL, VARE_WANTRES);
	    setenv(name, val, 1);
	    free(val);
	}
    } else {
	if (parent)
	    v->flags &= ~VAR_REEXPORT;	/* once will do */
	if (parent || !(v->flags & VAR_EXPORTED))
	    setenv(name, val, 1);
    }
    /*
     * This is so Var_Set knows to call Var_Export again...
     */
    if (parent) {
	v->flags |= VAR_EXPORTED;
    }
    return 1;
}

static void
Var_ExportVars_callback(void *entry, void *unused MAKE_ATTR_UNUSED)
{
    Var *var = entry;
    Var_Export1(var->name, 0);
}

/*
 * This gets called from our children.
 */
void
Var_ExportVars(void)
{
    char tmp[BUFSIZ];
    char *val;
    int n;

    /*
     * Several make's support this sort of mechanism for tracking
     * recursion - but each uses a different name.
     * We allow the makefiles to update MAKELEVEL and ensure
     * children see a correctly incremented value.
     */
    snprintf(tmp, sizeof(tmp), "%d", makelevel + 1);
    setenv(MAKE_LEVEL_ENV, tmp, 1);

    if (VAR_EXPORTED_NONE == var_exportedVars)
	return;

    if (VAR_EXPORTED_ALL == var_exportedVars) {
	/* Ouch! This is crazy... */
	Hash_ForEach(&VAR_GLOBAL->context, Var_ExportVars_callback, NULL);
	return;
    }
    /*
     * We have a number of exported vars,
     */
    n = snprintf(tmp, sizeof(tmp), "${" MAKE_EXPORTED ":O:u}");
    if (n < (int)sizeof(tmp)) {
	char **av;
	char *as;
	int ac;
	int i;

	val = Var_Subst(NULL, tmp, VAR_GLOBAL, VARE_WANTRES);
	if (*val) {
	    av = brk_string(val, &ac, FALSE, &as);
	    for (i = 0; i < ac; i++)
		Var_Export1(av[i], 0);
	    free(as);
	    free(av);
	}
	free(val);
    }
}

/*
 * This is called when .export is seen or
 * .MAKE.EXPORTED is modified.
 * It is also called when any exported var is modified.
 */
void
Var_Export(char *str, int isExport)
{
    char **av;
    char *as;
    VarExportFlags flags;
    int ac;
    int i;

    if (isExport && (!str || !str[0])) {
	var_exportedVars = VAR_EXPORTED_ALL; /* use with caution! */
	return;
    }

    flags = 0;
    if (strncmp(str, "-env", 4) == 0) {
	str += 4;
    } else if (strncmp(str, "-literal", 8) == 0) {
	str += 8;
	flags |= VAR_EXPORT_LITERAL;
    } else {
	flags |= VAR_EXPORT_PARENT;
    }

    char *val = Var_Subst(NULL, str, VAR_GLOBAL, VARE_WANTRES);
    if (*val) {
	av = brk_string(val, &ac, FALSE, &as);
	for (i = 0; i < ac; i++) {
	    const char *name = av[i];
	    if (!name[1]) {
		/*
		 * A single char.
		 * If it is one of the vars that should only appear in
		 * local context, skip it, else we can get Var_Subst
		 * into a loop.
		 */
		switch (name[0]) {
		case '@':
		case '%':
		case '*':
		case '!':
		    continue;
		}
	    }
	    if (Var_Export1(name, flags)) {
		if (VAR_EXPORTED_ALL != var_exportedVars)
		    var_exportedVars = VAR_EXPORTED_YES;
		if (isExport && (flags & VAR_EXPORT_PARENT)) {
		    Var_Append(MAKE_EXPORTED, name, VAR_GLOBAL);
		}
	    }
	}
	free(as);
	free(av);
    }
    free(val);
}


extern char **environ;

/*
 * This is called when .unexport[-env] is seen.
 *
 * str must have the form "unexport[-env] varname...".
 */
void
Var_UnExport(char *str)
{
    char tmp[BUFSIZ];
    char *vlist;
    char *cp;
    Boolean unexport_env;
    int n;

    vlist = NULL;

    str += strlen("unexport");
    unexport_env = (strncmp(str, "-env", 4) == 0);
    if (unexport_env) {
	char **newenv;

	cp = getenv(MAKE_LEVEL_ENV);	/* we should preserve this */
	if (environ == savedEnv) {
	    /* we have been here before! */
	    newenv = bmake_realloc(environ, 2 * sizeof(char *));
	} else {
	    if (savedEnv) {
		free(savedEnv);
		savedEnv = NULL;
	    }
	    newenv = bmake_malloc(2 * sizeof(char *));
	}
	if (!newenv)
	    return;
	/* Note: we cannot safely free() the original environ. */
	environ = savedEnv = newenv;
	newenv[0] = NULL;
	newenv[1] = NULL;
	if (cp && *cp)
	    setenv(MAKE_LEVEL_ENV, cp, 1);
    } else {
	for (; *str != '\n' && isspace((unsigned char) *str); str++)
	    continue;
	if (str[0] && str[0] != '\n') {
	    vlist = str;
	}
    }

    if (!vlist) {
	/* Using .MAKE.EXPORTED */
	vlist = Var_Subst(NULL, "${" MAKE_EXPORTED ":O:u}", VAR_GLOBAL,
			  VARE_WANTRES);
    }
    if (vlist) {
	Var *v;
	char **av;
	char *as;
	int ac;
	int i;

	av = brk_string(vlist, &ac, FALSE, &as);
	for (i = 0; i < ac; i++) {
	    v = VarFind(av[i], VAR_GLOBAL, 0);
	    if (!v)
		continue;
	    if (!unexport_env &&
		(v->flags & (VAR_EXPORTED | VAR_REEXPORT)) == VAR_EXPORTED)
		unsetenv(v->name);
	    v->flags &= ~(VAR_EXPORTED | VAR_REEXPORT);
	    /*
	     * If we are unexporting a list,
	     * remove each one from .MAKE.EXPORTED.
	     * If we are removing them all,
	     * just delete .MAKE.EXPORTED below.
	     */
	    if (vlist == str) {
		n = snprintf(tmp, sizeof(tmp),
			     "${" MAKE_EXPORTED ":N%s}", v->name);
		if (n < (int)sizeof(tmp)) {
		    cp = Var_Subst(NULL, tmp, VAR_GLOBAL, VARE_WANTRES);
		    Var_Set(MAKE_EXPORTED, cp, VAR_GLOBAL);
		    free(cp);
		}
	    }
	}
	free(as);
	free(av);
	if (vlist != str) {
	    Var_Delete(MAKE_EXPORTED, VAR_GLOBAL);
	    free(vlist);
	}
    }
}

static void
Var_Set_with_flags(const char *name, const char *val, GNode *ctxt,
		   VarSet_Flags flags)
{
    Var *v;
    char *expanded_name = NULL;

    /*
     * We only look for a variable in the given context since anything set
     * here will override anything in a lower context, so there's not much
     * point in searching them all just to save a bit of memory...
     */
    if (strchr(name, '$') != NULL) {
	expanded_name = Var_Subst(NULL, name, ctxt, VARE_WANTRES);
	if (expanded_name[0] == '\0') {
	    if (DEBUG(VAR)) {
		fprintf(debug_file, "Var_Set(\"%s\", \"%s\", ...) "
			"name expands to empty string - ignored\n",
			name, val);
	    }
	    free(expanded_name);
	    return;
	}
	name = expanded_name;
    }
    if (ctxt == VAR_GLOBAL) {
	v = VarFind(name, VAR_CMD, 0);
	if (v != NULL) {
	    if ((v->flags & VAR_FROM_CMD)) {
		if (DEBUG(VAR)) {
		    fprintf(debug_file, "%s:%s = %s ignored!\n", ctxt->name, name, val);
		}
		goto out;
	    }
	    VarFreeEnv(v, TRUE);
	}
    }
    v = VarFind(name, ctxt, 0);
    if (v == NULL) {
	if (ctxt == VAR_CMD && (flags & VAR_NO_EXPORT) == 0) {
	    /*
	     * This var would normally prevent the same name being added
	     * to VAR_GLOBAL, so delete it from there if needed.
	     * Otherwise -V name may show the wrong value.
	     */
	    Var_Delete(name, VAR_GLOBAL);
	}
	VarAdd(name, val, ctxt);
    } else {
	Buf_Empty(&v->val);
	if (val)
	    Buf_AddStr(&v->val, val);

	if (DEBUG(VAR)) {
	    fprintf(debug_file, "%s:%s = %s\n", ctxt->name, name, val);
	}
	if ((v->flags & VAR_EXPORTED)) {
	    Var_Export1(name, VAR_EXPORT_PARENT);
	}
    }
    /*
     * Any variables given on the command line are automatically exported
     * to the environment (as per POSIX standard)
     */
    if (ctxt == VAR_CMD && (flags & VAR_NO_EXPORT) == 0) {
	if (v == NULL) {
	    /* we just added it */
	    v = VarFind(name, ctxt, 0);
	}
	if (v != NULL)
	    v->flags |= VAR_FROM_CMD;
	/*
	 * If requested, don't export these in the environment
	 * individually.  We still put them in MAKEOVERRIDES so
	 * that the command-line settings continue to override
	 * Makefile settings.
	 */
	if (varNoExportEnv != TRUE)
	    setenv(name, val ? val : "", 1);

	Var_Append(MAKEOVERRIDES, name, VAR_GLOBAL);
    }
    if (name[0] == '.' && strcmp(name, SAVE_DOLLARS) == 0)
	save_dollars = s2Boolean(val, save_dollars);

out:
    free(expanded_name);
    if (v != NULL)
	VarFreeEnv(v, TRUE);
}

/*-
 *-----------------------------------------------------------------------
 * Var_Set --
 *	Set the variable name to the value val in the given context.
 *
 * Input:
 *	name		name of variable to set
 *	val		value to give to the variable
 *	ctxt		context in which to set it
 *
 * Side Effects:
 *	If the variable doesn't yet exist, a new record is created for it.
 *	Else the old value is freed and the new one stuck in its place
 *
 * Notes:
 *	The variable is searched for only in its context before being
 *	created in that context. I.e. if the context is VAR_GLOBAL,
 *	only VAR_GLOBAL->context is searched. Likewise if it is VAR_CMD, only
 *	VAR_CMD->context is searched. This is done to avoid the literally
 *	thousands of unnecessary strcmp's that used to be done to
 *	set, say, $(@) or $(<).
 *	If the context is VAR_GLOBAL though, we check if the variable
 *	was set in VAR_CMD from the command line and skip it if so.
 *-----------------------------------------------------------------------
 */
void
Var_Set(const char *name, const char *val, GNode *ctxt)
{
    Var_Set_with_flags(name, val, ctxt, 0);
}

/*-
 *-----------------------------------------------------------------------
 * Var_Append --
 *	The variable of the given name has the given value appended to it in
 *	the given context.
 *
 * Input:
 *	name		name of variable to modify
 *	val		String to append to it
 *	ctxt		Context in which this should occur
 *
 * Side Effects:
 *	If the variable doesn't exist, it is created. Else the strings
 *	are concatenated (with a space in between).
 *
 * Notes:
 *	Only if the variable is being sought in the global context is the
 *	environment searched.
 *	XXX: Knows its calling circumstances in that if called with ctxt
 *	an actual target, it will only search that context since only
 *	a local variable could be being appended to. This is actually
 *	a big win and must be tolerated.
 *-----------------------------------------------------------------------
 */
void
Var_Append(const char *name, const char *val, GNode *ctxt)
{
    Var *v;
    Hash_Entry *h;
    char *expanded_name = NULL;

    if (strchr(name, '$') != NULL) {
	expanded_name = Var_Subst(NULL, name, ctxt, VARE_WANTRES);
	if (expanded_name[0] == '\0') {
	    if (DEBUG(VAR)) {
		fprintf(debug_file, "Var_Append(\"%s\", \"%s\", ...) "
			"name expands to empty string - ignored\n",
			name, val);
	    }
	    free(expanded_name);
	    return;
	}
	name = expanded_name;
    }

    v = VarFind(name, ctxt, ctxt == VAR_GLOBAL ? (FIND_CMD | FIND_ENV) : 0);

    if (v == NULL) {
	Var_Set(name, val, ctxt);
    } else if (ctxt == VAR_CMD || !(v->flags & VAR_FROM_CMD)) {
	Buf_AddByte(&v->val, ' ');
	Buf_AddStr(&v->val, val);

	if (DEBUG(VAR)) {
	    fprintf(debug_file, "%s:%s = %s\n", ctxt->name, name,
		    Buf_GetAll(&v->val, NULL));
	}

	if (v->flags & VAR_FROM_ENV) {
	    /*
	     * If the original variable came from the environment, we
	     * have to install it in the global context (we could place
	     * it in the environment, but then we should provide a way to
	     * export other variables...)
	     */
	    v->flags &= ~VAR_FROM_ENV;
	    h = Hash_CreateEntry(&ctxt->context, name, NULL);
	    Hash_SetValue(h, v);
	}
    }
    free(expanded_name);
}

/*-
 *-----------------------------------------------------------------------
 * Var_Exists --
 *	See if the given variable exists.
 *
 * Input:
 *	name		Variable to find
 *	ctxt		Context in which to start search
 *
 * Results:
 *	TRUE if it does, FALSE if it doesn't
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Boolean
Var_Exists(const char *name, GNode *ctxt)
{
    Var		  *v;
    char          *cp;

    if ((cp = strchr(name, '$')) != NULL)
	cp = Var_Subst(NULL, name, ctxt, VARE_WANTRES);
    v = VarFind(cp ? cp : name, ctxt, FIND_CMD | FIND_GLOBAL | FIND_ENV);
    free(cp);
    if (v == NULL)
	return FALSE;

    (void)VarFreeEnv(v, TRUE);
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * Var_Value --
 *	Return the unexpanded value of the given variable in the given
 *	context.
 *
 * Input:
 *	name		name to find
 *	ctxt		context in which to search for it
 *
 * Results:
 *	The value if the variable exists, NULL if it doesn't.
 *	If the returned value is not NULL, the caller must free *freeIt
 *	as soon as the returned value is no longer needed.
 *-----------------------------------------------------------------------
 */
char *
Var_Value(const char *name, GNode *ctxt, char **freeIt)
{
    Var *v = VarFind(name, ctxt, FIND_ENV | FIND_GLOBAL | FIND_CMD);
    *freeIt = NULL;
    if (v == NULL)
	return NULL;

    char *p = Buf_GetAll(&v->val, NULL);
    if (VarFreeEnv(v, FALSE))
	*freeIt = p;
    return p;
}


/* SepBuf is a string being built from "words", interleaved with separators. */
typedef struct {
    Buffer buf;
    Boolean needSep;
    char sep;
} SepBuf;

static void
SepBuf_Init(SepBuf *buf, char sep)
{
    Buf_Init(&buf->buf, 32 /* bytes */);
    buf->needSep = FALSE;
    buf->sep = sep;
}

static void
SepBuf_Sep(SepBuf *buf)
{
    buf->needSep = TRUE;
}

static void
SepBuf_AddBytes(SepBuf *buf, const char *mem, size_t mem_size)
{
    if (mem_size == 0)
	return;
    if (buf->needSep && buf->sep != '\0') {
	Buf_AddByte(&buf->buf, buf->sep);
	buf->needSep = FALSE;
    }
    Buf_AddBytes(&buf->buf, mem_size, mem);
}

static void
SepBuf_AddBytesBetween(SepBuf *buf, const char *start, const char *end)
{
    SepBuf_AddBytes(buf, start, (size_t)(end - start));
}

static void
SepBuf_AddStr(SepBuf *buf, const char *str)
{
    SepBuf_AddBytes(buf, str, strlen(str));
}

static char *
SepBuf_Destroy(SepBuf *buf, Boolean free_buf)
{
    return Buf_Destroy(&buf->buf, free_buf);
}


/* This callback for ModifyWords gets a single word from an expression and
 * typically adds a modification of this word to the buffer. It may also do
 * nothing or add several words. */
typedef void (*ModifyWordsCallback)(const char *word, SepBuf *buf, void *data);


/* Callback for ModifyWords to implement the :H modifier.
 * Add the dirname of the given word to the buffer. */
static void
ModifyWord_Head(const char *word, SepBuf *buf, void *dummy MAKE_ATTR_UNUSED)
{
    const char *slash = strrchr(word, '/');
    if (slash != NULL)
	SepBuf_AddBytesBetween(buf, word, slash);
    else
	SepBuf_AddStr(buf, ".");
}

/* Callback for ModifyWords to implement the :T modifier.
 * Add the basename of the given word to the buffer. */
static void
ModifyWord_Tail(const char *word, SepBuf *buf, void *dummy MAKE_ATTR_UNUSED)
{
    const char *slash = strrchr(word, '/');
    const char *base = slash != NULL ? slash + 1 : word;
    SepBuf_AddStr(buf, base);
}

/* Callback for ModifyWords to implement the :E modifier.
 * Add the filename suffix of the given word to the buffer, if it exists. */
static void
ModifyWord_Suffix(const char *word, SepBuf *buf, void *dummy MAKE_ATTR_UNUSED)
{
    const char *dot = strrchr(word, '.');
    if (dot != NULL)
	SepBuf_AddStr(buf, dot + 1);
}

/* Callback for ModifyWords to implement the :R modifier.
 * Add the basename of the given word to the buffer. */
static void
ModifyWord_Root(const char *word, SepBuf *buf, void *dummy MAKE_ATTR_UNUSED)
{
    const char *dot = strrchr(word, '.');
    size_t len = dot != NULL ? (size_t)(dot - word) : strlen(word);
    SepBuf_AddBytes(buf, word, len);
}

/* Callback for ModifyWords to implement the :M modifier.
 * Place the word in the buffer if it matches the given pattern. */
static void
ModifyWord_Match(const char *word, SepBuf *buf, void *data)
{
    const char *pattern = data;
    if (DEBUG(VAR))
	fprintf(debug_file, "VarMatch [%s] [%s]\n", word, pattern);
    if (Str_Match(word, pattern))
	SepBuf_AddStr(buf, word);
}

/* Callback for ModifyWords to implement the :N modifier.
 * Place the word in the buffer if it doesn't match the given pattern. */
static void
ModifyWord_NoMatch(const char *word, SepBuf *buf, void *data)
{
    const char *pattern = data;
    if (!Str_Match(word, pattern))
	SepBuf_AddStr(buf, word);
}

#ifdef SYSVVARSUB
/*-
 *-----------------------------------------------------------------------
 * Str_SYSVMatch --
 *	Check word against pattern for a match (% is wild),
 *
 * Input:
 *	word		Word to examine
 *	pattern		Pattern to examine against
 *	len		Number of characters to substitute
 *
 * Results:
 *	Returns the beginning position of a match or null. The number
 *	of characters matched is returned in len.
 *-----------------------------------------------------------------------
 */
static const char *
Str_SYSVMatch(const char *word, const char *pattern, size_t *len,
    Boolean *hasPercent)
{
    const char *p = pattern;
    const char *w = word;
    const char *m;

    *hasPercent = FALSE;
    if (*p == '\0') {
	/* Null pattern is the whole string */
	*len = strlen(w);
	return w;
    }

    if ((m = strchr(p, '%')) != NULL) {
	*hasPercent = TRUE;
	if (*w == '\0') {
		/* empty word does not match pattern */
		return NULL;
	}
	/* check that the prefix matches */
	for (; p != m && *w && *w == *p; w++, p++)
	     continue;

	if (p != m)
	    return NULL;	/* No match */

	if (*++p == '\0') {
	    /* No more pattern, return the rest of the string */
	    *len = strlen(w);
	    return w;
	}
    }

    m = w;

    /* Find a matching tail */
    do {
	if (strcmp(p, w) == 0) {
	    *len = w - m;
	    return m;
	}
    } while (*w++ != '\0');

    return NULL;
}


/*-
 *-----------------------------------------------------------------------
 * Str_SYSVSubst --
 *	Substitute '%' on the pattern with len characters from src.
 *	If the pattern does not contain a '%' prepend len characters
 *	from src.
 *
 * Side Effects:
 *	Places result on buf
 *-----------------------------------------------------------------------
 */
static void
Str_SYSVSubst(SepBuf *buf, const char *pat, const char *src, size_t len,
	      Boolean lhsHasPercent)
{
    const char *m;

    if ((m = strchr(pat, '%')) != NULL && lhsHasPercent) {
	/* Copy the prefix */
	SepBuf_AddBytesBetween(buf, pat, m);
	/* skip the % */
	pat = m + 1;
    }
    if (m != NULL || !lhsHasPercent) {
	/* Copy the pattern */
	SepBuf_AddBytes(buf, src, len);
    }

    /* append the rest */
    SepBuf_AddStr(buf, pat);
}


typedef struct {
    GNode *ctx;
    const char *lhs;
    const char *rhs;
} ModifyWord_SYSVSubstArgs;

/* Callback for ModifyWords to implement the :%.from=%.to modifier. */
static void
ModifyWord_SYSVSubst(const char *word, SepBuf *buf, void *data)
{
    const ModifyWord_SYSVSubstArgs *args = data;

    size_t len;
    Boolean hasPercent;
    const char *ptr = Str_SYSVMatch(word, args->lhs, &len, &hasPercent);
    if (ptr != NULL) {
	char *varexp = Var_Subst(NULL, args->rhs, args->ctx, VARE_WANTRES);
	Str_SYSVSubst(buf, varexp, ptr, len, hasPercent);
	free(varexp);
    } else {
	SepBuf_AddStr(buf, word);
    }
}
#endif


typedef struct {
    const char	*lhs;
    size_t	lhsLen;
    const char	*rhs;
    size_t	rhsLen;
    VarPatternFlags pflags;
} ModifyWord_SubstArgs;

/* Callback for ModifyWords to implement the :S,from,to, modifier.
 * Perform a string substitution on the given word. */
static void
ModifyWord_Subst(const char *word, SepBuf *buf, void *data)
{
    size_t wordLen = strlen(word);
    ModifyWord_SubstArgs *args = data;
    const VarPatternFlags pflags = args->pflags;

    if ((pflags & VARP_SUB_ONE) && (pflags & VARP_SUB_MATCHED))
	goto nosub;

    if (args->pflags & VARP_ANCHOR_START) {
	if (wordLen < args->lhsLen ||
	    memcmp(word, args->lhs, args->lhsLen) != 0)
	    goto nosub;

	if (args->pflags & VARP_ANCHOR_END) {
	    if (wordLen != args->lhsLen)
		goto nosub;

	    SepBuf_AddBytes(buf, args->rhs, args->rhsLen);
	    args->pflags |= VARP_SUB_MATCHED;
	} else {
	    SepBuf_AddBytes(buf, args->rhs, args->rhsLen);
	    SepBuf_AddBytes(buf, word + args->lhsLen, wordLen - args->lhsLen);
	    args->pflags |= VARP_SUB_MATCHED;
	}
	return;
    }

    if (args->pflags & VARP_ANCHOR_END) {
	if (wordLen < args->lhsLen)
	    goto nosub;

	const char *start = word + (wordLen - args->lhsLen);
	if (memcmp(start, args->lhs, args->lhsLen) != 0)
	    goto nosub;

	SepBuf_AddBytesBetween(buf, word, start);
	SepBuf_AddBytes(buf, args->rhs, args->rhsLen);
	args->pflags |= VARP_SUB_MATCHED;
	return;
    }

    /* unanchored */
    const char *match;
    while ((match = Str_FindSubstring(word, args->lhs)) != NULL) {
	SepBuf_AddBytesBetween(buf, word, match);
	SepBuf_AddBytes(buf, args->rhs, args->rhsLen);
	args->pflags |= VARP_SUB_MATCHED;
	wordLen -= (match - word) + args->lhsLen;
	word += (match - word) + args->lhsLen;
	if (wordLen == 0 || !(args->pflags & VARP_SUB_GLOBAL))
	    break;
    }
nosub:
    SepBuf_AddBytes(buf, word, wordLen);
}

#ifndef NO_REGEX
/*-
 *-----------------------------------------------------------------------
 * VarREError --
 *	Print the error caused by a regcomp or regexec call.
 *
 * Side Effects:
 *	An error gets printed.
 *
 *-----------------------------------------------------------------------
 */
static void
VarREError(int reerr, regex_t *pat, const char *str)
{
    char *errbuf;
    int errlen;

    errlen = regerror(reerr, pat, 0, 0);
    errbuf = bmake_malloc(errlen);
    regerror(reerr, pat, errbuf, errlen);
    Error("%s: %s", str, errbuf);
    free(errbuf);
}

typedef struct {
    regex_t	   re;
    int		   nsub;
    char 	  *replace;
    VarPatternFlags pflags;
} ModifyWord_SubstRegexArgs;

/* Callback for ModifyWords to implement the :C/from/to/ modifier.
 * Perform a regex substitution on the given word. */
static void
ModifyWord_SubstRegex(const char *word, SepBuf *buf, void *data)
{
    ModifyWord_SubstRegexArgs *args = data;
    int xrv;
    const char *wp = word;
    char *rp;
    int flags = 0;
    regmatch_t m[10];

    if ((args->pflags & VARP_SUB_ONE) && (args->pflags & VARP_SUB_MATCHED))
	goto nosub;

tryagain:
    xrv = regexec(&args->re, wp, args->nsub, m, flags);

    switch (xrv) {
    case 0:
	args->pflags |= VARP_SUB_MATCHED;
	SepBuf_AddBytes(buf, wp, m[0].rm_so);

	for (rp = args->replace; *rp; rp++) {
	    if (*rp == '\\' && (rp[1] == '&' || rp[1] == '\\')) {
		SepBuf_AddBytes(buf, rp + 1, 1);
		rp++;
	    } else if (*rp == '&' ||
		(*rp == '\\' && isdigit((unsigned char)rp[1]))) {
		int n;
		char errstr[3];

		if (*rp == '&') {
		    n = 0;
		    errstr[0] = '&';
		    errstr[1] = '\0';
		} else {
		    n = rp[1] - '0';
		    errstr[0] = '\\';
		    errstr[1] = rp[1];
		    errstr[2] = '\0';
		    rp++;
		}

		if (n >= args->nsub) {
		    Error("No subexpression %s", errstr);
		} else if (m[n].rm_so == -1 && m[n].rm_eo == -1) {
		    Error("No match for subexpression %s", errstr);
		} else {
		    SepBuf_AddBytesBetween(buf, wp + m[n].rm_so,
					   wp + m[n].rm_eo);
		}

	    } else {
		SepBuf_AddBytes(buf, rp, 1);
	    }
	}
	wp += m[0].rm_eo;
	if (args->pflags & VARP_SUB_GLOBAL) {
	    flags |= REG_NOTBOL;
	    if (m[0].rm_so == 0 && m[0].rm_eo == 0) {
		SepBuf_AddBytes(buf, wp, 1);
		wp++;
	    }
	    if (*wp)
		goto tryagain;
	}
	if (*wp) {
	    SepBuf_AddStr(buf, wp);
	}
	break;
    default:
	VarREError(xrv, &args->re, "Unexpected regex error");
	/* fall through */
    case REG_NOMATCH:
    nosub:
	SepBuf_AddStr(buf, wp);
	break;
    }
}
#endif


typedef struct {
    GNode	*ctx;
    char	*tvar;		/* name of temporary variable */
    char	*str;		/* string to expand */
    VarEvalFlags eflags;
} ModifyWord_LoopArgs;

/* Callback for ModifyWords to implement the :@var@...@ modifier of ODE make. */
static void
ModifyWord_Loop(const char *word, SepBuf *buf, void *data)
{
    if (word[0] == '\0')
	return;

    const ModifyWord_LoopArgs *args = data;
    Var_Set_with_flags(args->tvar, word, args->ctx, VAR_NO_EXPORT);
    char *s = Var_Subst(NULL, args->str, args->ctx, args->eflags);
    if (DEBUG(VAR)) {
	fprintf(debug_file,
		"ModifyWord_Loop: in \"%s\", replace \"%s\" with \"%s\" "
		"to \"%s\"\n",
		word, args->tvar, args->str, s ? s : "(null)");
    }

    if (s != NULL && s[0] != '\0') {
	if (s[0] == '\n' || (buf->buf.count > 0 &&
	    buf->buf.buffer[buf->buf.count - 1] == '\n'))
	    buf->needSep = FALSE;
	SepBuf_AddStr(buf, s);
    }
    free(s);
}


/*-
 * Implements the :[first..last] modifier.
 * This is a special case of ModifyWords since we want to be able
 * to scan the list backwards if first > last.
 */
static char *
VarSelectWords(Byte sep, Boolean oneBigWord, const char *str, int first,
	       int last)
{
    SepBuf buf;
    char **av;			/* word list */
    char *as;			/* word list memory */
    int ac, i;
    int start, end, step;

    SepBuf_Init(&buf, sep);

    if (oneBigWord) {
	/* fake what brk_string() would do if there were only one word */
	ac = 1;
	av = bmake_malloc((ac + 1) * sizeof(char *));
	as = bmake_strdup(str);
	av[0] = as;
	av[1] = NULL;
    } else {
	av = brk_string(str, &ac, FALSE, &as);
    }

    /*
     * Now sanitize seldata.
     * If seldata->start or seldata->end are negative, convert them to
     * the positive equivalents (-1 gets converted to argc, -2 gets
     * converted to (argc-1), etc.).
     */
    if (first < 0)
	first += ac + 1;
    if (last < 0)
	last += ac + 1;

    /*
     * We avoid scanning more of the list than we need to.
     */
    if (first > last) {
	start = MIN(ac, first) - 1;
	end = MAX(0, last - 1);
	step = -1;
    } else {
	start = MAX(0, first - 1);
	end = MIN(ac, last);
	step = 1;
    }

    for (i = start; (step < 0) == (i >= end); i += step) {
	SepBuf_AddStr(&buf, av[i]);
	SepBuf_Sep(&buf);
    }

    free(as);
    free(av);

    return SepBuf_Destroy(&buf, FALSE);
}


/* Callback for ModifyWords to implement the :tA modifier.
 * Replace each word with the result of realpath() if successful. */
static void
ModifyWord_Realpath(const char *word, SepBuf *buf, void *data MAKE_ATTR_UNUSED)
{
    struct stat st;
    char rbuf[MAXPATHLEN];

    const char *rp = cached_realpath(word, rbuf);
    if (rp != NULL && *rp == '/' && stat(rp, &st) == 0)
	word = rp;

    SepBuf_AddStr(buf, word);
}

/*-
 *-----------------------------------------------------------------------
 * Modify each of the words of the passed string using the given function.
 *
 * Input:
 *	str		String whose words should be modified
 *	modifyWord	Function that modifies a single word
 *	data		Custom data for modifyWord
 *
 * Results:
 *	A string of all the words modified appropriately.
 *-----------------------------------------------------------------------
 */
static char *
ModifyWords(GNode *ctx, Byte sep, Boolean oneBigWord,
	    const char *str, ModifyWordsCallback modifyWord, void *data)
{
    if (oneBigWord) {
	SepBuf result;
	SepBuf_Init(&result, sep);
	modifyWord(str, &result, data);
	return SepBuf_Destroy(&result, FALSE);
    }

    SepBuf result;
    char **av;			/* word list */
    char *as;			/* word list memory */
    int ac, i;

    SepBuf_Init(&result, sep);

    av = brk_string(str, &ac, FALSE, &as);

    if (DEBUG(VAR)) {
	fprintf(debug_file, "ModifyWords: split \"%s\" into %d words\n",
		str, ac);
    }

    for (i = 0; i < ac; i++) {
	size_t orig_count = result.buf.count;
	modifyWord(av[i], &result, data);
	size_t count = result.buf.count;
	if (count != orig_count)
	    SepBuf_Sep(&result);
    }

    free(as);
    free(av);

    return SepBuf_Destroy(&result, FALSE);
}


static int
VarWordCompare(const void *a, const void *b)
{
    int r = strcmp(*(const char * const *)a, *(const char * const *)b);
    return r;
}

static int
VarWordCompareReverse(const void *a, const void *b)
{
    int r = strcmp(*(const char * const *)b, *(const char * const *)a);
    return r;
}

/*-
 *-----------------------------------------------------------------------
 * VarOrder --
 *	Order the words in the string.
 *
 * Input:
 *	str		String whose words should be sorted.
 *	otype		How to order: s - sort, x - random.
 *
 * Results:
 *	A string containing the words ordered.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static char *
VarOrder(const char *str, const char otype)
{
    Buffer buf;			/* Buffer for the new string */
    char **av;			/* word list */
    char *as;			/* word list memory */
    int ac, i;

    Buf_Init(&buf, 0);

    av = brk_string(str, &ac, FALSE, &as);

    if (ac > 0) {
	switch (otype) {
	case 'r':		/* reverse sort alphabetically */
	    qsort(av, ac, sizeof(char *), VarWordCompareReverse);
	    break;
	case 's':		/* sort alphabetically */
	    qsort(av, ac, sizeof(char *), VarWordCompare);
	    break;
	case 'x':		/* randomize */
	    /*
	     * We will use [ac..2] range for mod factors. This will produce
	     * random numbers in [(ac-1)..0] interval, and minimal
	     * reasonable value for mod factor is 2 (the mod 1 will produce
	     * 0 with probability 1).
	     */
	    for (i = ac - 1; i > 0; i--) {
		int rndidx = random() % (i + 1);
		char *t = av[i];
		av[i] = av[rndidx];
		av[rndidx] = t;
	    }
	}
    }

    for (i = 0; i < ac; i++) {
	if (i != 0)
	    Buf_AddByte(&buf, ' ');
	Buf_AddStr(&buf, av[i]);
    }

    free(as);
    free(av);

    return Buf_Destroy(&buf, FALSE);
}


/* Remove adjacent duplicate words. */
static char *
VarUniq(const char *str)
{
    Buffer	  buf;		/* Buffer for new string */
    char 	**av;		/* List of words to affect */
    char 	 *as;		/* Word list memory */
    int 	  ac, i, j;

    Buf_Init(&buf, 0);
    av = brk_string(str, &ac, FALSE, &as);

    if (ac > 1) {
	for (j = 0, i = 1; i < ac; i++)
	    if (strcmp(av[i], av[j]) != 0 && (++j != i))
		av[j] = av[i];
	ac = j + 1;
    }

    for (i = 0; i < ac; i++) {
	if (i != 0)
	    Buf_AddByte(&buf, ' ');
	Buf_AddStr(&buf, av[i]);
    }

    free(as);
    free(av);

    return Buf_Destroy(&buf, FALSE);
}

/*-
 *-----------------------------------------------------------------------
 * VarRange --
 *	Return an integer sequence
 *
 * Input:
 *	str		String whose words provide default range
 *	ac		range length, if 0 use str words
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static char *
VarRange(const char *str, int ac)
{
    Buffer	  buf;		/* Buffer for new string */
    char 	**av;		/* List of words to affect */
    char 	 *as;		/* Word list memory */
    int 	  i;

    Buf_Init(&buf, 0);
    if (ac > 0) {
	as = NULL;
	av = NULL;
    } else {
	av = brk_string(str, &ac, FALSE, &as);
    }
    for (i = 0; i < ac; i++) {
	if (i != 0)
	    Buf_AddByte(&buf, ' ');
	Buf_AddInt(&buf, 1 + i);
    }

    free(as);
    free(av);

    return Buf_Destroy(&buf, FALSE);
}


/*-
 * Parse a text part of a modifier such as the "from" and "to" in :S/from/to/
 * or the :@ modifier, until the next unescaped delimiter.  The delimiter, as
 * well as the backslash or the dollar, can be escaped with a backslash.
 *
 * Return the parsed (and possibly expanded) string, or NULL if no delimiter
 * was found.
 *
 * Nested variables in the text are expanded unless VARE_NOSUBST is set.
 *
 * If out_length is specified, store the length of the returned string, just
 * to save another strlen call.
 *
 * If out_pflags is specified and the last character of the pattern is a $,
 * set the VARP_ANCHOR_END bit of mpflags (for the first part of the :S
 * modifier).
 *
 * If subst is specified, handle escaped ampersands and replace unescaped
 * ampersands with the lhs of the pattern (for the second part of the :S
 * modifier).
 */
static char *
ParseModifierPart(const char **tstr, int delim, VarEvalFlags eflags,
		  GNode *ctxt, size_t *out_length,
		  VarPatternFlags *out_pflags, ModifyWord_SubstArgs *subst)
{
    const char *cp;
    char *rstr;
    Buffer buf;
    VarEvalFlags errnum = eflags & VARE_UNDEFERR;

    Buf_Init(&buf, 0);

    /*
     * Skim through until the matching delimiter is found;
     * pick up variable substitutions on the way. Also allow
     * backslashes to quote the delimiter, $, and \, but don't
     * touch other backslashes.
     */
    for (cp = *tstr; *cp != '\0' && *cp != delim; cp++) {
	Boolean is_escaped = cp[0] == '\\' && (
	    cp[1] == delim || cp[1] == '\\' || cp[1] == '$' ||
	    (cp[1] == '&' && subst != NULL));
	if (is_escaped) {
	    Buf_AddByte(&buf, cp[1]);
	    cp++;
	} else if (*cp == '$') {
	    if (cp[1] == delim) {	/* Unescaped $ at end of pattern */
		if (out_pflags != NULL)
		    *out_pflags |= VARP_ANCHOR_END;
		else
		    Buf_AddByte(&buf, *cp);
	    } else {
		if (eflags & VARE_WANTRES) {
		    const char *cp2;
		    int     len;
		    void   *freeIt;

		    /*
		     * If unescaped dollar sign not before the
		     * delimiter, assume it's a variable
		     * substitution and recurse.
		     */
		    cp2 = Var_Parse(cp, ctxt, errnum | (eflags & VARE_WANTRES),
				    &len, &freeIt);
		    Buf_AddStr(&buf, cp2);
		    free(freeIt);
		    cp += len - 1;
		} else {
		    const char *cp2 = &cp[1];

		    if (*cp2 == PROPEN || *cp2 == BROPEN) {
			/*
			 * Find the end of this variable reference
			 * and suck it in without further ado.
			 * It will be interpreted later.
			 */
			int have = *cp2;
			int want = (*cp2 == PROPEN) ? PRCLOSE : BRCLOSE;
			int depth = 1;

			for (++cp2; *cp2 != '\0' && depth > 0; ++cp2) {
			    if (cp2[-1] != '\\') {
				if (*cp2 == have)
				    ++depth;
				if (*cp2 == want)
				    --depth;
			    }
			}
			Buf_AddBytesBetween(&buf, cp, cp2);
			cp = --cp2;
		    } else
			Buf_AddByte(&buf, *cp);
		}
	    }
	} else if (subst != NULL && *cp == '&')
	    Buf_AddBytes(&buf, subst->lhsLen, subst->lhs);
	else
	    Buf_AddByte(&buf, *cp);
    }

    if (*cp != delim) {
	*tstr = cp;
	return NULL;
    }

    *tstr = ++cp;
    if (out_length != NULL)
	*out_length = Buf_Size(&buf);
    rstr = Buf_Destroy(&buf, FALSE);
    if (DEBUG(VAR))
	fprintf(debug_file, "Modifier part: \"%s\"\n", rstr);
    return rstr;
}

/*-
 *-----------------------------------------------------------------------
 * VarQuote --
 *	Quote shell meta-characters and space characters in the string
 *	if quoteDollar is set, also quote and double any '$' characters.
 *
 * Results:
 *	The quoted string
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static char *
VarQuote(char *str, Boolean quoteDollar)
{
    Buffer buf;
    Buf_Init(&buf, 0);

    for (; *str != '\0'; str++) {
	if (*str == '\n') {
	    const char *newline = Shell_GetNewline();
	    if (newline == NULL)
		newline = "\\\n";
	    Buf_AddStr(&buf, newline);
	    continue;
	}
	if (isspace((unsigned char)*str) || ismeta((unsigned char)*str))
	    Buf_AddByte(&buf, '\\');
	Buf_AddByte(&buf, *str);
	if (quoteDollar && *str == '$')
	    Buf_AddStr(&buf, "\\$");
    }

    str = Buf_Destroy(&buf, FALSE);
    if (DEBUG(VAR))
	fprintf(debug_file, "QuoteMeta: [%s]\n", str);
    return str;
}

/*-
 *-----------------------------------------------------------------------
 * VarHash --
 *      Hash the string using the MurmurHash3 algorithm.
 *      Output is computed using 32bit Little Endian arithmetic.
 *
 * Input:
 *	str		String to modify
 *
 * Results:
 *      Hash value of str, encoded as 8 hex digits.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------
 */
static char *
VarHash(const char *str)
{
    static const char    hexdigits[16] = "0123456789abcdef";
    Buffer         buf;
    size_t         len, len2;
    const unsigned char *ustr = (const unsigned char *)str;
    uint32_t       h, k, c1, c2;

    h  = 0x971e137bU;
    c1 = 0x95543787U;
    c2 = 0x2ad7eb25U;
    len2 = strlen(str);

    for (len = len2; len; ) {
	k = 0;
	switch (len) {
	default:
	    k = ((uint32_t)ustr[3] << 24) |
		((uint32_t)ustr[2] << 16) |
		((uint32_t)ustr[1] << 8) |
		(uint32_t)ustr[0];
	    len -= 4;
	    ustr += 4;
	    break;
	case 3:
	    k |= (uint32_t)ustr[2] << 16;
	    /* FALLTHROUGH */
	case 2:
	    k |= (uint32_t)ustr[1] << 8;
	    /* FALLTHROUGH */
	case 1:
	    k |= (uint32_t)ustr[0];
	    len = 0;
	}
	c1 = c1 * 5 + 0x7b7d159cU;
	c2 = c2 * 5 + 0x6bce6396U;
	k *= c1;
	k = (k << 11) ^ (k >> 21);
	k *= c2;
	h = (h << 13) ^ (h >> 19);
	h = h * 5 + 0x52dce729U;
	h ^= k;
    }
    h ^= len2;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    Buf_Init(&buf, 0);
    for (len = 0; len < 8; ++len) {
	Buf_AddByte(&buf, hexdigits[h & 15]);
	h >>= 4;
    }

    return Buf_Destroy(&buf, FALSE);
}

static char *
VarStrftime(const char *fmt, int zulu, time_t utc)
{
    char buf[BUFSIZ];

    if (!utc)
	time(&utc);
    if (!*fmt)
	fmt = "%c";
    strftime(buf, sizeof(buf), fmt, zulu ? gmtime(&utc) : localtime(&utc));

    buf[sizeof(buf) - 1] = '\0';
    return bmake_strdup(buf);
}

typedef struct {
    /* const parameters */
    int startc;			/* '\0' or '{' or '(' */
    int endc;
    Var *v;
    GNode *ctxt;
    VarEvalFlags eflags;

    /* read-write */
    char *nstr;
    const char *cp;		/* The position where parsing continues
				 * after the current modifier. */
    char termc;			/* Character which terminated scan */
    char missing_delim;		/* For error reporting */

    Byte	sep;		/* Word separator in expansions */
    Boolean	oneBigWord;	/* TRUE if we will treat the variable as a
				 * single big word, even if it contains
				 * embedded spaces (as opposed to the
				 * usual behaviour of treating it as
				 * several space-separated words). */

    /* result */
    char *newStr;		/* New value to return */
} ApplyModifiersState;

/* we now have some modifiers with long names */
static Boolean
ModMatch(const char *mod, const char *modname, char endc)
{
    size_t n = strlen(modname);
    return strncmp(mod, modname, n) == 0 &&
	   (mod[n] == endc || mod[n] == ':');
}

static inline Boolean
ModMatchEq(const char *mod, const char *modname, char endc)
{
    size_t n = strlen(modname);
    return strncmp(mod, modname, n) == 0 &&
	   (mod[n] == endc || mod[n] == ':' || mod[n] == '=');
}

/* :@var@...${var}...@ */
static Boolean
ApplyModifier_Loop(const char *mod, ApplyModifiersState *st) {
    ModifyWord_LoopArgs args;

    args.ctx = st->ctxt;
    st->cp = mod + 1;
    char delim = '@';
    args.tvar = ParseModifierPart(&st->cp, delim, st->eflags & ~VARE_WANTRES,
				  st->ctxt, NULL, NULL, NULL);
    if (args.tvar == NULL) {
	st->missing_delim = delim;
	return FALSE;
    }

    args.str = ParseModifierPart(&st->cp, delim, st->eflags & ~VARE_WANTRES,
				 st->ctxt, NULL, NULL, NULL);
    if (args.str == NULL) {
	st->missing_delim = delim;
	return FALSE;
    }

    st->termc = *st->cp;

    args.eflags = st->eflags & (VARE_UNDEFERR | VARE_WANTRES);
    int prev_sep = st->sep;
    st->sep = ' ';		/* XXX: this is inconsistent */
    st->newStr = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->nstr,
			     ModifyWord_Loop, &args);
    st->sep = prev_sep;
    Var_Delete(args.tvar, st->ctxt);
    free(args.tvar);
    free(args.str);
    return TRUE;
}

/* :Ddefined or :Uundefined */
static void
ApplyModifier_Defined(const char *mod, ApplyModifiersState *st)
{
    Buffer buf;			/* Buffer for patterns */
    VarEvalFlags neflags;

    if (st->eflags & VARE_WANTRES) {
	Boolean wantres;
	if (*mod == 'U')
	    wantres = ((st->v->flags & VAR_JUNK) != 0);
	else
	    wantres = ((st->v->flags & VAR_JUNK) == 0);
	neflags = st->eflags & ~VARE_WANTRES;
	if (wantres)
	    neflags |= VARE_WANTRES;
    } else
	neflags = st->eflags;

    /*
     * Pass through mod looking for 1) escaped delimiters,
     * '$'s and backslashes (place the escaped character in
     * uninterpreted) and 2) unescaped $'s that aren't before
     * the delimiter (expand the variable substitution).
     * The result is left in the Buffer buf.
     */
    Buf_Init(&buf, 0);
    const char *p = mod + 1;
    while (*p != st->endc && *p != ':' && *p != '\0') {
	if (*p == '\\' &&
	    (p[1] == ':' || p[1] == '$' || p[1] == st->endc || p[1] == '\\')) {
	    Buf_AddByte(&buf, p[1]);
	    p += 2;
	} else if (*p == '$') {
	    /*
	     * If unescaped dollar sign, assume it's a
	     * variable substitution and recurse.
	     */
	    const char *cp2;
	    int	    len;
	    void    *freeIt;

	    cp2 = Var_Parse(p, st->ctxt, neflags, &len, &freeIt);
	    Buf_AddStr(&buf, cp2);
	    free(freeIt);
	    p += len;
	} else {
	    Buf_AddByte(&buf, *p);
	    p++;
	}
    }

    st->cp = p;
    st->termc = *st->cp;

    if (st->v->flags & VAR_JUNK)
	st->v->flags |= VAR_KEEP;
    if (neflags & VARE_WANTRES) {
	st->newStr = Buf_Destroy(&buf, FALSE);
    } else {
	st->newStr = st->nstr;
	Buf_Destroy(&buf, TRUE);
    }
}

/* :gmtime */
static Boolean
ApplyModifier_Gmtime(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatchEq(mod, "gmtime", st->endc)) {
	st->cp = mod + 1;
	return FALSE;
    }

    time_t utc;
    if (mod[6] == '=') {
	char *ep;
	utc = strtoul(mod + 7, &ep, 10);
	st->cp = ep;
    } else {
	utc = 0;
	st->cp = mod + 6;
    }
    st->newStr = VarStrftime(st->nstr, 1, utc);
    st->termc = *st->cp;
    return TRUE;
}

/* :localtime */
static Boolean
ApplyModifier_Localtime(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatchEq(mod, "localtime", st->endc)) {
	st->cp = mod + 1;
	return FALSE;
    }

    time_t utc;
    if (mod[9] == '=') {
	char *ep;
	utc = strtoul(mod + 10, &ep, 10);
	st->cp = ep;
    } else {
	utc = 0;
	st->cp = mod + 9;
    }
    st->newStr = VarStrftime(st->nstr, 0, utc);
    st->termc = *st->cp;
    return TRUE;
}

/* :hash */
static Boolean
ApplyModifier_Hash(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatch(mod, "hash", st->endc)) {
	st->cp = mod + 1;
	return FALSE;
    }

    st->newStr = VarHash(st->nstr);
    st->cp = mod + 4;
    st->termc = *st->cp;
    return TRUE;
}

/* :P */
static void
ApplyModifier_Path(const char *mod, ApplyModifiersState *st)
{
    if (st->v->flags & VAR_JUNK)
	st->v->flags |= VAR_KEEP;
    GNode *gn = Targ_FindNode(st->v->name, TARG_NOCREATE);
    if (gn == NULL || gn->type & OP_NOPATH) {
	st->newStr = NULL;
    } else if (gn->path) {
	st->newStr = bmake_strdup(gn->path);
    } else {
	st->newStr = Dir_FindFile(st->v->name, Suff_FindPath(gn));
    }
    if (!st->newStr)
	st->newStr = bmake_strdup(st->v->name);
    st->cp = mod + 1;
    st->termc = *st->cp;
}

/* :!cmd! */
static Boolean
ApplyModifier_Exclam(const char *mod, ApplyModifiersState *st)
{
    st->cp = mod + 1;
    char delim = '!';
    char *cmd = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				  NULL, NULL, NULL);
    if (cmd == NULL) {
	st->missing_delim = delim;
	return FALSE;
    }

    const char *emsg = NULL;
    if (st->eflags & VARE_WANTRES)
	st->newStr = Cmd_Exec(cmd, &emsg);
    else
	st->newStr = varNoError;
    free(cmd);

    if (emsg)
	Error(emsg, st->nstr);

    st->termc = *st->cp;
    if (st->v->flags & VAR_JUNK)
	st->v->flags |= VAR_KEEP;
    return TRUE;
}

/* :range */
static Boolean
ApplyModifier_Range(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatchEq(mod, "range", st->endc)) {
	st->cp = mod + 1;
	return FALSE;
    }

    int n;
    if (mod[5] == '=') {
	char *ep;
	n = strtoul(mod + 6, &ep, 10);
	st->cp = ep;
    } else {
	n = 0;
	st->cp = mod + 5;
    }
    st->newStr = VarRange(st->nstr, n);
    st->termc = *st->cp;
    return TRUE;
}

/* :Mpattern or :Npattern */
static void
ApplyModifier_Match(const char *mod, ApplyModifiersState *st)
{
    Boolean copy = FALSE;	/* pattern should be, or has been, copied */
    Boolean needSubst = FALSE;
    /*
     * In the loop below, ignore ':' unless we are at (or back to) the
     * original brace level.
     * XXX This will likely not work right if $() and ${} are intermixed.
     */
    int nest = 1;
    for (st->cp = mod + 1;
	 *st->cp != '\0' && !(*st->cp == ':' && nest == 1);
	 st->cp++) {
	if (*st->cp == '\\' &&
	    (st->cp[1] == ':' || st->cp[1] == st->endc ||
	     st->cp[1] == st->startc)) {
	    if (!needSubst)
		copy = TRUE;
	    st->cp++;
	    continue;
	}
	if (*st->cp == '$')
	    needSubst = TRUE;
	if (*st->cp == '(' || *st->cp == '{')
	    ++nest;
	if (*st->cp == ')' || *st->cp == '}') {
	    --nest;
	    if (nest == 0)
		break;
	}
    }
    st->termc = *st->cp;
    const char *endpat = st->cp;

    char *pattern = NULL;
    if (copy) {
	/*
	 * Need to compress the \:'s out of the pattern, so
	 * allocate enough room to hold the uncompressed
	 * pattern (note that st->cp started at mod+1, so
	 * st->cp - mod takes the null byte into account) and
	 * compress the pattern into the space.
	 */
	pattern = bmake_malloc(st->cp - mod);
	char *cp2;
	for (cp2 = pattern, st->cp = mod + 1;
	     st->cp < endpat;
	     st->cp++, cp2++) {
	    if ((*st->cp == '\\') && (st->cp+1 < endpat) &&
		(st->cp[1] == ':' || st->cp[1] == st->endc))
		st->cp++;
	    *cp2 = *st->cp;
	}
	*cp2 = '\0';
	endpat = cp2;
    } else {
	/*
	 * Either Var_Subst or ModifyWords will need a
	 * nul-terminated string soon, so construct one now.
	 */
	pattern = bmake_strndup(mod + 1, endpat - (mod + 1));
    }
    if (needSubst) {
	/* pattern contains embedded '$', so use Var_Subst to expand it. */
	char *old_pattern = pattern;
	pattern = Var_Subst(NULL, pattern, st->ctxt, st->eflags);
	free(old_pattern);
    }
    if (DEBUG(VAR))
	fprintf(debug_file, "Pattern[%s] for [%s] is [%s]\n",
	    st->v->name, st->nstr, pattern);
    ModifyWordsCallback callback = mod[0] == 'M'
	? ModifyWord_Match : ModifyWord_NoMatch;
    st->newStr = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->nstr,
			     callback, pattern);
    free(pattern);
}

/* :S,from,to, */
static Boolean
ApplyModifier_Subst(const char * const mod, ApplyModifiersState *st)
{
    ModifyWord_SubstArgs args;
    Boolean oneBigWord = st->oneBigWord;
    char delim = mod[1];

    st->cp = mod + 2;

    /*
     * If pattern begins with '^', it is anchored to the
     * start of the word -- skip over it and flag pattern.
     */
    args.pflags = 0;
    if (*st->cp == '^') {
	args.pflags |= VARP_ANCHOR_START;
	st->cp++;
    }

    char *lhs = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				  &args.lhsLen, &args.pflags, NULL);
    if (lhs == NULL) {
	st->missing_delim = delim;
	return FALSE;
    }
    args.lhs = lhs;

    char *rhs = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				  &args.rhsLen, NULL, &args);
    if (rhs == NULL) {
	st->missing_delim = delim;
	return FALSE;
    }
    args.rhs = rhs;

    /*
     * Check for global substitution. If 'g' after the final
     * delimiter, substitution is global and is marked that
     * way.
     */
    for (;; st->cp++) {
	switch (*st->cp) {
	case 'g':
	    args.pflags |= VARP_SUB_GLOBAL;
	    continue;
	case '1':
	    args.pflags |= VARP_SUB_ONE;
	    continue;
	case 'W':
	    oneBigWord = TRUE;
	    continue;
	}
	break;
    }

    st->termc = *st->cp;
    st->newStr = ModifyWords(st->ctxt, st->sep, oneBigWord, st->nstr,
			     ModifyWord_Subst, &args);

    free(lhs);
    free(rhs);
    return TRUE;
}

#ifndef NO_REGEX

/* :C,from,to, */
static Boolean
ApplyModifier_Regex(const char *mod, ApplyModifiersState *st)
{
    ModifyWord_SubstRegexArgs args;

    args.pflags = 0;
    Boolean oneBigWord = st->oneBigWord;
    char delim = mod[1];

    st->cp = mod + 2;

    char *re = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				 NULL, NULL, NULL);
    if (re == NULL) {
	st->missing_delim = delim;
	return FALSE;
    }

    args.replace = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				     NULL, NULL, NULL);
    if (args.replace == NULL) {
	free(re);
	st->missing_delim = delim;
	return FALSE;
    }

    for (;; st->cp++) {
	switch (*st->cp) {
	case 'g':
	    args.pflags |= VARP_SUB_GLOBAL;
	    continue;
	case '1':
	    args.pflags |= VARP_SUB_ONE;
	    continue;
	case 'W':
	    oneBigWord = TRUE;
	    continue;
	}
	break;
    }

    st->termc = *st->cp;

    int error = regcomp(&args.re, re, REG_EXTENDED);
    free(re);
    if (error) {
	VarREError(error, &args.re, "RE substitution error");
	free(args.replace);
	return FALSE;
    }

    args.nsub = args.re.re_nsub + 1;
    if (args.nsub < 1)
	args.nsub = 1;
    if (args.nsub > 10)
	args.nsub = 10;
    st->newStr = ModifyWords(st->ctxt, st->sep, oneBigWord, st->nstr,
			     ModifyWord_SubstRegex, &args);
    regfree(&args.re);
    free(args.replace);
    return TRUE;
}
#endif

static void
ModifyWord_Copy(const char *word, SepBuf *buf, void *data MAKE_ATTR_UNUSED)
{
    SepBuf_AddStr(buf, word);
}

/* :ts<separator> */
static Boolean
ApplyModifier_ToSep(const char *sep, ApplyModifiersState *st)
{
    if (sep[0] != st->endc && (sep[1] == st->endc || sep[1] == ':')) {
	/* ":ts<unrecognised><endc>" or ":ts<unrecognised>:" */
	st->sep = sep[0];
	st->cp = sep + 1;
    } else if (sep[0] == st->endc || sep[0] == ':') {
	/* ":ts<endc>" or ":ts:" */
	st->sep = '\0';		/* no separator */
	st->cp = sep;
    } else if (sep[0] == '\\') {
	const char *xp = sep + 1;
	int base = 8;		/* assume octal */

	switch (sep[1]) {
	case 'n':
	    st->sep = '\n';
	    st->cp = sep + 2;
	    break;
	case 't':
	    st->sep = '\t';
	    st->cp = sep + 2;
	    break;
	case 'x':
	    base = 16;
	    xp++;
	    goto get_numeric;
	case '0':
	    base = 0;
	    goto get_numeric;
	default:
	    if (!isdigit((unsigned char)sep[1]))
		return FALSE;	/* ":ts<backslash><unrecognised>". */

	    char *end;
	get_numeric:
	    st->sep = strtoul(sep + 1 + (sep[1] == 'x'), &end, base);
	    if (*end != ':' && *end != st->endc)
	        return FALSE;
	    st->cp = end;
	    break;
	}
    } else {
	return FALSE;		/* Found ":ts<unrecognised><unrecognised>". */
    }

    st->termc = *st->cp;
    st->newStr = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->nstr,
			     ModifyWord_Copy, NULL);
    return TRUE;
}

/* :tA, :tu, :tl, :ts<separator>, etc. */
static Boolean
ApplyModifier_To(const char *mod, ApplyModifiersState *st)
{
    st->cp = mod + 1;	/* make sure it is set */
    if (mod[1] == st->endc || mod[1] == ':')
	return FALSE;		/* Found ":t<endc>" or ":t:". */

    if (mod[1] == 's')
	return ApplyModifier_ToSep(mod + 2, st);

    if (mod[2] != st->endc && mod[2] != ':')
	return FALSE;		/* Found ":t<unrecognised><unrecognised>". */

    /* Check for two-character options: ":tu", ":tl" */
    if (mod[1] == 'A') {	/* absolute path */
	st->newStr = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->nstr,
				 ModifyWord_Realpath, NULL);
	st->cp = mod + 2;
	st->termc = *st->cp;
    } else if (mod[1] == 'u') {
	char *dp = bmake_strdup(st->nstr);
	for (st->newStr = dp; *dp; dp++)
	    *dp = toupper((unsigned char)*dp);
	st->cp = mod + 2;
	st->termc = *st->cp;
    } else if (mod[1] == 'l') {
	char *dp = bmake_strdup(st->nstr);
	for (st->newStr = dp; *dp; dp++)
	    *dp = tolower((unsigned char)*dp);
	st->cp = mod + 2;
	st->termc = *st->cp;
    } else if (mod[1] == 'W' || mod[1] == 'w') {
	st->oneBigWord = mod[1] == 'W';
	st->newStr = st->nstr;
	st->cp = mod + 2;
	st->termc = *st->cp;
    } else {
	/* Found ":t<unrecognised>:" or ":t<unrecognised><endc>". */
	return FALSE;
    }
    return TRUE;
}

/* :[#], :[1], etc. */
static int
ApplyModifier_Words(const char *mod, ApplyModifiersState *st)
{
    st->cp = mod + 1;		/* point to char after '[' */
    char delim = ']';		/* look for closing ']' */
    char *estr = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				   NULL, NULL, NULL);
    if (estr == NULL) {
	st->missing_delim = delim;
	return 'c';
    }

    /* now st->cp points just after the closing ']' */
    if (st->cp[0] != ':' && st->cp[0] != st->endc)
	goto bad_modifier;	/* Found junk after ']' */

    if (estr[0] == '\0')
	goto bad_modifier;	/* empty square brackets in ":[]". */

    if (estr[0] == '#' && estr[1] == '\0') { /* Found ":[#]" */
	if (st->oneBigWord) {
	    st->newStr = bmake_strdup("1");
	} else {
	    /* XXX: brk_string() is a rather expensive
	     * way of counting words. */
	    char *as;
	    int ac;
	    char **av = brk_string(st->nstr, &ac, FALSE, &as);
	    free(as);
	    free(av);

	    Buffer buf;
	    Buf_Init(&buf, 4);	/* 3 digits + '\0' */
	    Buf_AddInt(&buf, ac);
	    st->newStr = Buf_Destroy(&buf, FALSE);
	}
	goto ok;
    }

    if (estr[0] == '*' && estr[1] == '\0') {
	/* Found ":[*]" */
	st->oneBigWord = TRUE;
	st->newStr = st->nstr;
	goto ok;
    }

    if (estr[0] == '@' && estr[1] == '\0') {
	/* Found ":[@]" */
	st->oneBigWord = FALSE;
	st->newStr = st->nstr;
	goto ok;
    }

    /*
     * We expect estr to contain a single integer for :[N], or two integers
     * separated by ".." for :[start..end].
     */
    char *ep;
    int first = strtol(estr, &ep, 0);
    int last;
    if (ep == estr)		/* Found junk instead of a number */
	goto bad_modifier;

    if (ep[0] == '\0') {	/* Found only one integer in :[N] */
	last = first;
    } else if (ep[0] == '.' && ep[1] == '.' && ep[2] != '\0') {
	/* Expecting another integer after ".." */
	ep += 2;
	last = strtol(ep, &ep, 0);
	if (ep[0] != '\0')	/* Found junk after ".." */
	    goto bad_modifier;
    } else
	goto bad_modifier;	/* Found junk instead of ".." */

    /*
     * Now seldata is properly filled in, but we still have to check for 0 as
     * a special case.
     */
    if (first == 0 && last == 0) {
	/* ":[0]" or perhaps ":[0..0]" */
	st->oneBigWord = TRUE;
	st->newStr = st->nstr;
	goto ok;
    }

    /* ":[0..N]" or ":[N..0]" */
    if (first == 0 || last == 0)
	goto bad_modifier;

    /* Normal case: select the words described by seldata. */
    st->newStr = VarSelectWords(st->sep, st->oneBigWord, st->nstr, first, last);

ok:
    st->termc = *st->cp;
    free(estr);
    return 0;

bad_modifier:
    free(estr);
    return 'b';
}

/* :O or :Ox */
static Boolean
ApplyModifier_Order(const char *mod, ApplyModifiersState *st)
{
    char otype;

    st->cp = mod + 1;	/* skip to the rest in any case */
    if (mod[1] == st->endc || mod[1] == ':') {
	otype = 's';
	st->termc = *st->cp;
    } else if ((mod[1] == 'r' || mod[1] == 'x') &&
	       (mod[2] == st->endc || mod[2] == ':')) {
	otype = mod[1];
	st->cp = mod + 2;
	st->termc = *st->cp;
    } else {
	return FALSE;
    }
    st->newStr = VarOrder(st->nstr, otype);
    return TRUE;
}

/* :? then : else */
static Boolean
ApplyModifier_IfElse(const char *mod, ApplyModifiersState *st)
{
    Boolean value = FALSE;
    int cond_rc = 0;
    VarEvalFlags then_eflags = st->eflags & ~VARE_WANTRES;
    VarEvalFlags else_eflags = st->eflags & ~VARE_WANTRES;

    if (st->eflags & VARE_WANTRES) {
	cond_rc = Cond_EvalExpression(NULL, st->v->name, &value, 0, FALSE);
	if (cond_rc != COND_INVALID && value)
	    then_eflags |= VARE_WANTRES;
	if (cond_rc != COND_INVALID && !value)
	    else_eflags |= VARE_WANTRES;
    }

    st->cp = mod + 1;
    char delim = ':';
    char *then_expr = ParseModifierPart(&st->cp, delim, then_eflags, st->ctxt,
					NULL, NULL, NULL);
    if (then_expr == NULL) {
	st->missing_delim = delim;
	return FALSE;
    }

    delim = st->endc;		/* BRCLOSE or PRCLOSE */
    char *else_expr = ParseModifierPart(&st->cp, delim, else_eflags, st->ctxt,
					NULL, NULL, NULL);
    if (else_expr == NULL) {
	st->missing_delim = delim;
	return FALSE;
    }

    st->termc = *--st->cp;
    if (cond_rc == COND_INVALID) {
	Error("Bad conditional expression `%s' in %s?%s:%s",
	    st->v->name, st->v->name, then_expr, else_expr);
	return FALSE;
    }

    if (value) {
	st->newStr = then_expr;
	free(else_expr);
    } else {
	st->newStr = else_expr;
	free(then_expr);
    }
    if (st->v->flags & VAR_JUNK)
	st->v->flags |= VAR_KEEP;
    return TRUE;
}

/*
 * The ::= modifiers actually assign a value to the variable.
 * Their main purpose is in supporting modifiers of .for loop
 * iterators and other obscure uses.  They always expand to
 * nothing.  In a target rule that would otherwise expand to an
 * empty line they can be preceded with @: to keep make happy.
 * Eg.
 *
 * foo:	.USE
 * .for i in ${.TARGET} ${.TARGET:R}.gz
 * 	@: ${t::=$i}
 *	@echo blah ${t:T}
 * .endfor
 *
 *	  ::=<str>	Assigns <str> as the new value of variable.
 *	  ::?=<str>	Assigns <str> as value of variable if
 *			it was not already set.
 *	  ::+=<str>	Appends <str> to variable.
 *	  ::!=<cmd>	Assigns output of <cmd> as the new value of
 *			variable.
 */
static int
ApplyModifier_Assign(const char *mod, ApplyModifiersState *st)
{
    const char *op = mod + 1;
    if (!(op[0] == '=' ||
	(op[1] == '=' &&
	 (op[0] == '!' || op[0] == '+' || op[0] == '?'))))
	return 'd';		/* "::<unrecognised>" */

    GNode *v_ctxt;		/* context where v belongs */

    if (st->v->name[0] == 0)
	return 'b';

    v_ctxt = st->ctxt;
    char *sv_name = NULL;
    if (st->v->flags & VAR_JUNK) {
	/*
	 * We need to bmake_strdup() it incase ParseModifierPart() recurses.
	 */
	sv_name = st->v->name;
	st->v->name = bmake_strdup(st->v->name);
    } else if (st->ctxt != VAR_GLOBAL) {
	Var *gv = VarFind(st->v->name, st->ctxt, 0);
	if (gv == NULL)
	    v_ctxt = VAR_GLOBAL;
	else
	    VarFreeEnv(gv, TRUE);
    }

    switch (op[0]) {
    case '+':
    case '?':
    case '!':
	st->cp = mod + 3;
	break;
    default:
	st->cp = mod + 2;
	break;
    }

    char delim = st->startc == PROPEN ? PRCLOSE : BRCLOSE;
    char *val = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				  NULL, NULL, NULL);
    if (st->v->flags & VAR_JUNK) {
	/* restore original name */
	free(st->v->name);
	st->v->name = sv_name;
    }
    if (val == NULL) {
	st->missing_delim = delim;
	return 'c';
    }

    st->termc = *--st->cp;

    if (st->eflags & VARE_WANTRES) {
	switch (op[0]) {
	case '+':
	    Var_Append(st->v->name, val, v_ctxt);
	    break;
	case '!': {
	    const char *emsg;
	    st->newStr = Cmd_Exec(val, &emsg);
	    if (emsg)
		Error(emsg, st->nstr);
	    else
		Var_Set(st->v->name, st->newStr, v_ctxt);
	    free(st->newStr);
	    break;
	}
	case '?':
	    if (!(st->v->flags & VAR_JUNK))
		break;
	    /* FALLTHROUGH */
	default:
	    Var_Set(st->v->name, val, v_ctxt);
	    break;
	}
    }
    free(val);
    st->newStr = varNoError;
    return 0;
}

/* remember current value */
static Boolean
ApplyModifier_Remember(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatchEq(mod, "_", st->endc)) {
	st->cp = mod + 1;
	return FALSE;
    }

    if (mod[1] == '=') {
	size_t n = strcspn(mod + 2, ":)}");
	char *name = bmake_strndup(mod + 2, n);
	Var_Set(name, st->nstr, st->ctxt);
	free(name);
	st->cp = mod + 2 + n;
    } else {
	Var_Set("_", st->nstr, st->ctxt);
	st->cp = mod + 1;
    }
    st->newStr = st->nstr;
    st->termc = *st->cp;
    return TRUE;
}

#ifdef SYSVVARSUB
/* :from=to */
static int
ApplyModifier_SysV(const char *mod, ApplyModifiersState *st)
{
    Boolean eqFound = FALSE;

    /*
     * First we make a pass through the string trying
     * to verify it is a SYSV-make-style translation:
     * it must be: <string1>=<string2>)
     */
    st->cp = mod;
    int nest = 1;
    while (*st->cp != '\0' && nest > 0) {
	if (*st->cp == '=') {
	    eqFound = TRUE;
	    /* continue looking for st->endc */
	} else if (*st->cp == st->endc)
	    nest--;
	else if (*st->cp == st->startc)
	    nest++;
	if (nest > 0)
	    st->cp++;
    }
    if (*st->cp != st->endc || !eqFound)
	return 0;

    char delim = '=';
    st->cp = mod;
    char *lhs = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				  NULL, NULL, NULL);
    if (lhs == NULL) {
	st->missing_delim = delim;
	return 'c';
    }

    delim = st->endc;
    char *rhs = ParseModifierPart(&st->cp, delim, st->eflags, st->ctxt,
				  NULL, NULL, NULL);
    if (rhs == NULL) {
	st->missing_delim = delim;
	return 'c';
    }

    /*
     * SYSV modifications happen through the whole
     * string. Note the pattern is anchored at the end.
     */
    st->termc = *--st->cp;
    if (lhs[0] == '\0' && *st->nstr == '\0') {
	st->newStr = st->nstr;	/* special case */
    } else {
	ModifyWord_SYSVSubstArgs args = { st->ctxt, lhs, rhs };
	st->newStr = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->nstr,
				 ModifyWord_SYSVSubst, &args);
    }
    free(lhs);
    free(rhs);
    return '=';
}
#endif

/*
 * Now we need to apply any modifiers the user wants applied.
 * These are:
 *  	  :M<pattern>	words which match the given <pattern>.
 *  			<pattern> is of the standard file
 *  			wildcarding form.
 *  	  :N<pattern>	words which do not match the given <pattern>.
 *  	  :S<d><pat1><d><pat2><d>[1gW]
 *  			Substitute <pat2> for <pat1> in the value
 *  	  :C<d><pat1><d><pat2><d>[1gW]
 *  			Substitute <pat2> for regex <pat1> in the value
 *  	  :H		Substitute the head of each word
 *  	  :T		Substitute the tail of each word
 *  	  :E		Substitute the extension (minus '.') of
 *  			each word
 *  	  :R		Substitute the root of each word
 *  			(pathname minus the suffix).
 *	  :O		("Order") Alphabeticaly sort words in variable.
 *	  :Ox		("intermiX") Randomize words in variable.
 *	  :u		("uniq") Remove adjacent duplicate words.
 *	  :tu		Converts the variable contents to uppercase.
 *	  :tl		Converts the variable contents to lowercase.
 *	  :ts[c]	Sets varSpace - the char used to
 *			separate words to 'c'. If 'c' is
 *			omitted then no separation is used.
 *	  :tW		Treat the variable contents as a single
 *			word, even if it contains spaces.
 *			(Mnemonic: one big 'W'ord.)
 *	  :tw		Treat the variable contents as multiple
 *			space-separated words.
 *			(Mnemonic: many small 'w'ords.)
 *	  :[index]	Select a single word from the value.
 *	  :[start..end]	Select multiple words from the value.
 *	  :[*] or :[0]	Select the entire value, as a single
 *			word.  Equivalent to :tW.
 *	  :[@]		Select the entire value, as multiple
 *			words.	Undoes the effect of :[*].
 *			Equivalent to :tw.
 *	  :[#]		Returns the number of words in the value.
 *
 *	  :?<true-value>:<false-value>
 *			If the variable evaluates to true, return
 *			true value, else return the second value.
 *    	  :lhs=rhs  	Like :S, but the rhs goes to the end of
 *    			the invocation.
 *	  :sh		Treat the current value as a command
 *			to be run, new value is its output.
 * The following added so we can handle ODE makefiles.
 *	  :@<tmpvar>@<newval>@
 *			Assign a temporary local variable <tmpvar>
 *			to the current value of each word in turn
 *			and replace each word with the result of
 *			evaluating <newval>
 *	  :D<newval>	Use <newval> as value if variable defined
 *	  :U<newval>	Use <newval> as value if variable undefined
 *	  :L		Use the name of the variable as the value.
 *	  :P		Use the path of the node that has the same
 *			name as the variable as the value.  This
 *			basically includes an implied :L so that
 *			the common method of refering to the path
 *			of your dependent 'x' in a rule is to use
 *			the form '${x:P}'.
 *	  :!<cmd>!	Run cmd much the same as :sh run's the
 *			current value of the variable.
 * Assignment operators (see ApplyModifier_Assign).
 */
static char *
ApplyModifiers(char *nstr, const char * const tstr,
	       int const startc, int const endc,
	       Var * const v, GNode * const ctxt, VarEvalFlags const eflags,
	       int * const lengthPtr, void ** const freePtr)
{
    ApplyModifiersState st = {
	startc, endc, v, ctxt, eflags,
	nstr, tstr, '\0', '\0', ' ', FALSE, NULL
    };

    const char *p = tstr;
    while (*p != '\0' && *p != endc) {

	if (*p == '$') {
	    /*
	     * We may have some complex modifiers in a variable.
	     */
	    void *freeIt;
	    const char *rval;
	    int rlen;
	    int c;

	    rval = Var_Parse(p, st.ctxt, st.eflags, &rlen, &freeIt);

	    /*
	     * If we have not parsed up to st.endc or ':',
	     * we are not interested.
	     */
	    if (rval != NULL && *rval &&
		(c = p[rlen]) != '\0' &&
		c != ':' &&
		c != st.endc) {
		free(freeIt);
		goto apply_mods;
	    }

	    if (DEBUG(VAR)) {
		fprintf(debug_file, "Got '%s' from '%.*s'%.*s\n",
		       rval, rlen, p, rlen, p + rlen);
	    }

	    p += rlen;

	    if (rval != NULL && *rval) {
		int used;

		st.nstr = ApplyModifiers(st.nstr, rval, 0, 0, st.v,
				      st.ctxt, st.eflags, &used, freePtr);
		if (st.nstr == var_Error
		    || (st.nstr == varNoError && (st.eflags & VARE_UNDEFERR) == 0)
		    || strlen(rval) != (size_t) used) {
		    free(freeIt);
		    goto out;	/* error already reported */
		}
	    }
	    free(freeIt);
	    if (*p == ':')
		p++;
	    else if (*p == '\0' && endc != '\0') {
		Error("Unclosed variable specification after complex "
		    "modifier (expecting '%c') for %s", st.endc, st.v->name);
		goto out;
	    }
	    continue;
	}
    apply_mods:
	if (DEBUG(VAR)) {
	    fprintf(debug_file, "Applying[%s] :%c to \"%s\"\n", st.v->name,
		*p, st.nstr);
	}
	st.newStr = var_Error;
	char modifier = *p;
	switch (modifier) {
	case ':':
	    {
		int res = ApplyModifier_Assign(p, &st);
		if (res == 'b')
		    goto bad_modifier;
		if (res == 'c')
		    goto cleanup;
		if (res == 'd')
		    goto default_case;
		break;
	    }
	case '@':
	    if (!ApplyModifier_Loop(p, &st))
		goto cleanup;
	    break;
	case '_':
	    if (!ApplyModifier_Remember(p, &st))
		goto default_case;
	    break;
	case 'D':
	case 'U':
	    ApplyModifier_Defined(p, &st);
	    break;
	case 'L':
	    {
		if (st.v->flags & VAR_JUNK)
		    st.v->flags |= VAR_KEEP;
		st.newStr = bmake_strdup(st.v->name);
		st.cp = p + 1;
		st.termc = *st.cp;
		break;
	    }
	case 'P':
	    ApplyModifier_Path(p, &st);
	    break;
	case '!':
	    if (!ApplyModifier_Exclam(p, &st))
		goto cleanup;
	    break;
	case '[':
	    {
		int res = ApplyModifier_Words(p, &st);
		if (res == 'b')
		    goto bad_modifier;
		if (res == 'c')
		    goto cleanup;
		break;
	    }
	case 'g':
	    if (!ApplyModifier_Gmtime(p, &st))
		goto default_case;
	    break;
	case 'h':
	    if (!ApplyModifier_Hash(p, &st))
		goto default_case;
	    break;
	case 'l':
	    if (!ApplyModifier_Localtime(p, &st))
		goto default_case;
	    break;
	case 't':
	    if (!ApplyModifier_To(p, &st))
		goto bad_modifier;
	    break;
	case 'N':
	case 'M':
	    ApplyModifier_Match(p, &st);
	    break;
	case 'S':
	    if (!ApplyModifier_Subst(p, &st))
		goto cleanup;
	    break;
	case '?':
	    if (!ApplyModifier_IfElse(p, &st))
		goto cleanup;
	    break;
#ifndef NO_REGEX
	case 'C':
	    if (!ApplyModifier_Regex(p, &st))
		goto cleanup;
	    break;
#endif
	case 'q':
	case 'Q':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newStr = VarQuote(st.nstr, modifier == 'q');
		st.cp = p + 1;
		st.termc = *st.cp;
		break;
	    }
	    goto default_case;
	case 'T':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newStr = ModifyWords(st.ctxt, st.sep, st.oneBigWord,
					st.nstr, ModifyWord_Tail, NULL);
		st.cp = p + 1;
		st.termc = *st.cp;
		break;
	    }
	    goto default_case;
	case 'H':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newStr = ModifyWords(st.ctxt, st.sep, st.oneBigWord,
					st.nstr, ModifyWord_Head, NULL);
		st.cp = p + 1;
		st.termc = *st.cp;
		break;
	    }
	    goto default_case;
	case 'E':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newStr = ModifyWords(st.ctxt, st.sep, st.oneBigWord,
					st.nstr, ModifyWord_Suffix, NULL);
		st.cp = p + 1;
		st.termc = *st.cp;
		break;
	    }
	    goto default_case;
	case 'R':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newStr = ModifyWords(st.ctxt, st.sep, st.oneBigWord,
					st.nstr, ModifyWord_Root, NULL);
		st.cp = p + 1;
		st.termc = *st.cp;
		break;
	    }
	    goto default_case;
	case 'r':
	    if (!ApplyModifier_Range(p, &st))
		goto default_case;
	    break;
	case 'O':
	    if (!ApplyModifier_Order(p, &st))
		goto bad_modifier;
	    break;
	case 'u':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newStr = VarUniq(st.nstr);
		st.cp = p + 1;
		st.termc = *st.cp;
		break;
	    }
	    goto default_case;
#ifdef SUNSHCMD
	case 's':
	    if (p[1] == 'h' && (p[2] == st.endc || p[2] == ':')) {
		const char *emsg;
		if (st.eflags & VARE_WANTRES) {
		    st.newStr = Cmd_Exec(st.nstr, &emsg);
		    if (emsg)
			Error(emsg, st.nstr);
		} else
		    st.newStr = varNoError;
		st.cp = p + 2;
		st.termc = *st.cp;
		break;
	    }
	    goto default_case;
#endif
	default:
	default_case:
	    {
#ifdef SYSVVARSUB
		int res = ApplyModifier_SysV(p, &st);
		if (res == 'c')
		    goto cleanup;
		if (res != '=')
#endif
		{
		    Error("Unknown modifier '%c'", *p);
		    for (st.cp = p + 1;
			 *st.cp != ':' && *st.cp != st.endc && *st.cp != '\0';
			 st.cp++)
			continue;
		    st.termc = *st.cp;
		    st.newStr = var_Error;
		}
	    }
	}
	if (DEBUG(VAR)) {
	    fprintf(debug_file, "Result[%s] of :%c is \"%s\"\n",
		st.v->name, modifier, st.newStr);
	}

	if (st.newStr != st.nstr) {
	    if (*freePtr) {
		free(st.nstr);
		*freePtr = NULL;
	    }
	    st.nstr = st.newStr;
	    if (st.nstr != var_Error && st.nstr != varNoError) {
		*freePtr = st.nstr;
	    }
	}
	if (st.termc == '\0' && st.endc != '\0') {
	    Error("Unclosed variable specification (expecting '%c') "
		"for \"%s\" (value \"%s\") modifier %c",
		st.endc, st.v->name, st.nstr, modifier);
	} else if (st.termc == ':') {
	    st.cp++;
	}
	p = st.cp;
    }
out:
    *lengthPtr = p - tstr;
    return st.nstr;

bad_modifier:
    Error("Bad modifier `:%.*s' for %s",
	  (int)strcspn(p, ":)}"), p, st.v->name);

cleanup:
    *lengthPtr = st.cp - tstr;
    if (st.missing_delim != '\0')
	Error("Unclosed substitution for %s (%c missing)",
	      st.v->name, st.missing_delim);
    free(*freePtr);
    *freePtr = NULL;
    return var_Error;
}

static Boolean
VarIsDynamic(GNode *ctxt, const char *varname, size_t namelen)
{
    if ((namelen == 1 ||
	 (namelen == 2 && (varname[1] == 'F' || varname[1] == 'D'))) &&
	(ctxt == VAR_CMD || ctxt == VAR_GLOBAL))
    {
	/*
	 * If substituting a local variable in a non-local context,
	 * assume it's for dynamic source stuff. We have to handle
	 * this specially and return the longhand for the variable
	 * with the dollar sign escaped so it makes it back to the
	 * caller. Only four of the local variables are treated
	 * specially as they are the only four that will be set
	 * when dynamic sources are expanded.
	 */
	switch (varname[0]) {
	case '@':
	case '%':
	case '*':
	case '!':
	    return TRUE;
	}
	return FALSE;
    }

    if ((namelen == 7 || namelen == 8) && varname[0] == '.' &&
	isupper((unsigned char) varname[1]) &&
	(ctxt == VAR_CMD || ctxt == VAR_GLOBAL))
    {
	return strcmp(varname, ".TARGET") == 0 ||
	    strcmp(varname, ".ARCHIVE") == 0 ||
	    strcmp(varname, ".PREFIX") == 0 ||
	    strcmp(varname, ".MEMBER") == 0;
    }

    return FALSE;
}

/*-
 *-----------------------------------------------------------------------
 * Var_Parse --
 *	Given the start of a variable invocation (such as $v, $(VAR),
 *	${VAR:Mpattern}), extract the variable name, possibly some
 *	modifiers and find its value by applying the modifiers to the
 *	original value.
 *
 * Input:
 *	str		The string to parse
 *	ctxt		The context for the variable
 *	flags		VARE_UNDEFERR	if undefineds are an error
 *			VARE_WANTRES	if we actually want the result
 *			VARE_ASSIGN	if we are in a := assignment
 *	lengthPtr	OUT: The length of the specification
 *	freePtr		OUT: Non-NULL if caller should free *freePtr
 *
 * Results:
 *	The (possibly-modified) value of the variable or var_Error if the
 *	specification is invalid. The length of the specification is
 *	placed in *lengthPtr (for invalid specifications, this is just
 *	2...?).
 *	If *freePtr is non-NULL then it's a pointer that the caller
 *	should pass to free() to free memory used by the result.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
/* coverity[+alloc : arg-*4] */
const char *
Var_Parse(const char * const str, GNode *ctxt, VarEvalFlags eflags,
	  int *lengthPtr, void **freePtr)
{
    const char	*tstr;		/* Pointer into str */
    Var		*v;		/* Variable in invocation */
    Boolean 	 haveModifier;	/* TRUE if have modifiers for the variable */
    char	 endc;		/* Ending character when variable in parens
				 * or braces */
    char	 startc;	/* Starting character when variable in parens
				 * or braces */
    char	*nstr;		/* New string, used during expansion */
    Boolean	 dynamic;	/* TRUE if the variable is local and we're
				 * expanding it in a non-local context. This
				 * is done to support dynamic sources. The
				 * result is just the invocation, unaltered */
    const char	*extramodifiers; /* extra modifiers to apply first */

    *freePtr = NULL;
    extramodifiers = NULL;
    dynamic = FALSE;

    startc = str[1];
    if (startc != PROPEN && startc != BROPEN) {
	/*
	 * If it's not bounded by braces of some sort, life is much simpler.
	 * We just need to check for the first character and return the
	 * value if it exists.
	 */

	/* Error out some really stupid names */
	if (startc == '\0' || strchr(")}:$", startc)) {
	    *lengthPtr = 1;
	    return var_Error;
	}
	char name[] = { startc, '\0' };

	v = VarFind(name, ctxt, FIND_ENV | FIND_GLOBAL | FIND_CMD);
	if (v == NULL) {
	    *lengthPtr = 2;

	    if ((ctxt == VAR_CMD) || (ctxt == VAR_GLOBAL)) {
		/*
		 * If substituting a local variable in a non-local context,
		 * assume it's for dynamic source stuff. We have to handle
		 * this specially and return the longhand for the variable
		 * with the dollar sign escaped so it makes it back to the
		 * caller. Only four of the local variables are treated
		 * specially as they are the only four that will be set
		 * when dynamic sources are expanded.
		 */
		switch (str[1]) {
		case '@':
		    return "$(.TARGET)";
		case '%':
		    return "$(.MEMBER)";
		case '*':
		    return "$(.PREFIX)";
		case '!':
		    return "$(.ARCHIVE)";
		}
	    }
	    return (eflags & VARE_UNDEFERR) ? var_Error : varNoError;
	} else {
	    haveModifier = FALSE;
	    tstr = str + 1;
	    endc = str[1];
	}
    } else {
	Buffer namebuf;		/* Holds the variable name */
	int depth = 1;

	endc = startc == PROPEN ? PRCLOSE : BRCLOSE;
	Buf_Init(&namebuf, 0);

	/*
	 * Skip to the end character or a colon, whichever comes first.
	 */
	for (tstr = str + 2; *tstr != '\0'; tstr++) {
	    /* Track depth so we can spot parse errors. */
	    if (*tstr == startc)
		depth++;
	    if (*tstr == endc) {
		if (--depth == 0)
		    break;
	    }
	    if (depth == 1 && *tstr == ':')
		break;
	    /* A variable inside a variable, expand. */
	    if (*tstr == '$') {
		int rlen;
		void *freeIt;
		const char *rval = Var_Parse(tstr, ctxt, eflags, &rlen, &freeIt);
		if (rval != NULL)
		    Buf_AddStr(&namebuf, rval);
		free(freeIt);
		tstr += rlen - 1;
	    } else
		Buf_AddByte(&namebuf, *tstr);
	}
	if (*tstr == ':') {
	    haveModifier = TRUE;
	} else if (*tstr == endc) {
	    haveModifier = FALSE;
	} else {
	    /*
	     * If we never did find the end character, return NULL
	     * right now, setting the length to be the distance to
	     * the end of the string, since that's what make does.
	     */
	    *lengthPtr = tstr - str;
	    Buf_Destroy(&namebuf, TRUE);
	    return var_Error;
	}

	int namelen;
	char *varname = Buf_GetAll(&namebuf, &namelen);

	/*
	 * At this point, varname points into newly allocated memory from
	 * namebuf, containing only the name of the variable.
	 *
	 * start and tstr point into the const string that was pointed
	 * to by the original value of the str parameter.  start points
	 * to the '$' at the beginning of the string, while tstr points
	 * to the char just after the end of the variable name -- this
	 * will be '\0', ':', PRCLOSE, or BRCLOSE.
	 */

	v = VarFind(varname, ctxt, FIND_ENV | FIND_GLOBAL | FIND_CMD);
	/*
	 * Check also for bogus D and F forms of local variables since we're
	 * in a local context and the name is the right length.
	 */
	if (v == NULL && ctxt != VAR_CMD && ctxt != VAR_GLOBAL &&
		namelen == 2 && (varname[1] == 'F' || varname[1] == 'D') &&
		strchr("@%?*!<>", varname[0]) != NULL) {
	    /*
	     * Well, it's local -- go look for it.
	     */
	    char name[] = {varname[0], '\0' };
	    v = VarFind(name, ctxt, 0);

	    if (v != NULL) {
		if (varname[1] == 'D') {
		    extramodifiers = "H:";
		} else { /* F */
		    extramodifiers = "T:";
		}
	    }
	}

	if (v == NULL) {
	    dynamic = VarIsDynamic(ctxt, varname, namelen);

	    if (!haveModifier) {
		/*
		 * No modifiers -- have specification length so we can return
		 * now.
		 */
		*lengthPtr = tstr - str + 1;
		if (dynamic) {
		    char *pstr = bmake_strndup(str, *lengthPtr);
		    *freePtr = pstr;
		    Buf_Destroy(&namebuf, TRUE);
		    return pstr;
		} else {
		    Buf_Destroy(&namebuf, TRUE);
		    return (eflags & VARE_UNDEFERR) ? var_Error : varNoError;
		}
	    } else {
		/*
		 * Still need to get to the end of the variable specification,
		 * so kludge up a Var structure for the modifications
		 */
		v = bmake_malloc(sizeof(Var));
		v->name = varname;
		Buf_Init(&v->val, 1);
		v->flags = VAR_JUNK;
		Buf_Destroy(&namebuf, FALSE);
	    }
	} else
	    Buf_Destroy(&namebuf, TRUE);
    }

    if (v->flags & VAR_IN_USE) {
	Fatal("Variable %s is recursive.", v->name);
	/*NOTREACHED*/
    } else {
	v->flags |= VAR_IN_USE;
    }
    /*
     * Before doing any modification, we have to make sure the value
     * has been fully expanded. If it looks like recursion might be
     * necessary (there's a dollar sign somewhere in the variable's value)
     * we just call Var_Subst to do any other substitutions that are
     * necessary. Note that the value returned by Var_Subst will have
     * been dynamically-allocated, so it will need freeing when we
     * return.
     */
    nstr = Buf_GetAll(&v->val, NULL);
    if (strchr(nstr, '$') != NULL && (eflags & VARE_WANTRES) != 0) {
	nstr = Var_Subst(NULL, nstr, ctxt, eflags);
	*freePtr = nstr;
    }

    v->flags &= ~VAR_IN_USE;

    if (nstr != NULL && (haveModifier || extramodifiers != NULL)) {
	void *extraFree;
	int used;

	extraFree = NULL;
	if (extramodifiers != NULL) {
	    nstr = ApplyModifiers(nstr, extramodifiers, '(', ')',
				  v, ctxt, eflags, &used, &extraFree);
	}

	if (haveModifier) {
	    /* Skip initial colon. */
	    tstr++;

	    nstr = ApplyModifiers(nstr, tstr, startc, endc,
				  v, ctxt, eflags, &used, freePtr);
	    tstr += used;
	    free(extraFree);
	} else {
	    *freePtr = extraFree;
	}
    }
    *lengthPtr = tstr - str + (*tstr ? 1 : 0);

    if (v->flags & VAR_FROM_ENV) {
	Boolean destroy = FALSE;

	if (nstr != Buf_GetAll(&v->val, NULL)) {
	    destroy = TRUE;
	} else {
	    /*
	     * Returning the value unmodified, so tell the caller to free
	     * the thing.
	     */
	    *freePtr = nstr;
	}
	VarFreeEnv(v, destroy);
    } else if (v->flags & VAR_JUNK) {
	/*
	 * Perform any free'ing needed and set *freePtr to NULL so the caller
	 * doesn't try to free a static pointer.
	 * If VAR_KEEP is also set then we want to keep str(?) as is.
	 */
	if (!(v->flags & VAR_KEEP)) {
	    if (*freePtr) {
		free(nstr);
		*freePtr = NULL;
	    }
	    if (dynamic) {
		nstr = bmake_strndup(str, *lengthPtr);
		*freePtr = nstr;
	    } else {
		nstr = (eflags & VARE_UNDEFERR) ? var_Error : varNoError;
	    }
	}
	if (nstr != Buf_GetAll(&v->val, NULL))
	    Buf_Destroy(&v->val, TRUE);
	free(v->name);
	free(v);
    }
    return nstr;
}

/*-
 *-----------------------------------------------------------------------
 * Var_Subst  --
 *	Substitute for all variables in the given string in the given context.
 *	If eflags & VARE_UNDEFERR, Parse_Error will be called when an undefined
 *	variable is encountered.
 *
 * Input:
 *	var		Named variable || NULL for all
 *	str		the string which to substitute
 *	ctxt		the context wherein to find variables
 *	eflags		VARE_UNDEFERR	if undefineds are an error
 *			VARE_WANTRES	if we actually want the result
 *			VARE_ASSIGN	if we are in a := assignment
 *
 * Results:
 *	The resulting string.
 *
 * Side Effects:
 *	None.
 *-----------------------------------------------------------------------
 */
char *
Var_Subst(const char *var, const char *str, GNode *ctxt, VarEvalFlags eflags)
{
    Buffer	buf;		/* Buffer for forming things */
    const char	*val;		/* Value to substitute for a variable */
    int		length;		/* Length of the variable invocation */
    Boolean	trailingBslash;	/* variable ends in \ */
    void	*freeIt = NULL;	/* Set if it should be freed */
    static Boolean errorReported; /* Set true if an error has already
				 * been reported to prevent a plethora
				 * of messages when recursing */

    Buf_Init(&buf, 0);
    errorReported = FALSE;
    trailingBslash = FALSE;

    while (*str) {
	if (*str == '\n' && trailingBslash)
	    Buf_AddByte(&buf, ' ');
	if (var == NULL && (*str == '$') && (str[1] == '$')) {
	    /*
	     * A dollar sign may be escaped either with another dollar sign.
	     * In such a case, we skip over the escape character and store the
	     * dollar sign into the buffer directly.
	     */
	    if (save_dollars && (eflags & VARE_ASSIGN))
		Buf_AddByte(&buf, *str);
	    str++;
	    Buf_AddByte(&buf, *str);
	    str++;
	} else if (*str != '$') {
	    /*
	     * Skip as many characters as possible -- either to the end of
	     * the string or to the next dollar sign (variable invocation).
	     */
	    const char *cp;

	    for (cp = str++; *str != '$' && *str != '\0'; str++)
		continue;
	    Buf_AddBytesBetween(&buf, cp, str);
	} else {
	    if (var != NULL) {
		int expand;
		for (;;) {
		    if (str[1] == '\0') {
			/* A trailing $ is kind of a special case */
			Buf_AddByte(&buf, str[0]);
			str++;
			expand = FALSE;
		    } else if (str[1] != PROPEN && str[1] != BROPEN) {
			if (str[1] != *var || strlen(var) > 1) {
			    Buf_AddBytes(&buf, 2, str);
			    str += 2;
			    expand = FALSE;
			} else
			    expand = TRUE;
			break;
		    } else {
			const char *p;

			/* Scan up to the end of the variable name. */
			for (p = &str[2]; *p &&
			     *p != ':' && *p != PRCLOSE && *p != BRCLOSE; p++)
			    if (*p == '$')
				break;
			/*
			 * A variable inside the variable. We cannot expand
			 * the external variable yet, so we try again with
			 * the nested one
			 */
			if (*p == '$') {
			    Buf_AddBytesBetween(&buf, str, p);
			    str = p;
			    continue;
			}

			if (strncmp(var, str + 2, p - str - 2) != 0 ||
			    var[p - str - 2] != '\0') {
			    /*
			     * Not the variable we want to expand, scan
			     * until the next variable
			     */
			    for (; *p != '$' && *p != '\0'; p++)
				continue;
			    Buf_AddBytesBetween(&buf, str, p);
			    str = p;
			    expand = FALSE;
			} else
			    expand = TRUE;
			break;
		    }
		}
		if (!expand)
		    continue;
	    }

	    val = Var_Parse(str, ctxt, eflags, &length, &freeIt);

	    /*
	     * When we come down here, val should either point to the
	     * value of this variable, suitably modified, or be NULL.
	     * Length should be the total length of the potential
	     * variable invocation (from $ to end character...)
	     */
	    if (val == var_Error || val == varNoError) {
		/*
		 * If performing old-time variable substitution, skip over
		 * the variable and continue with the substitution. Otherwise,
		 * store the dollar sign and advance str so we continue with
		 * the string...
		 */
		if (oldVars) {
		    str += length;
		} else if ((eflags & VARE_UNDEFERR) || val == var_Error) {
		    /*
		     * If variable is undefined, complain and skip the
		     * variable. The complaint will stop us from doing anything
		     * when the file is parsed.
		     */
		    if (!errorReported) {
			Parse_Error(PARSE_FATAL, "Undefined variable \"%.*s\"",
				    length, str);
		    }
		    str += length;
		    errorReported = TRUE;
		} else {
		    Buf_AddByte(&buf, *str);
		    str += 1;
		}
	    } else {
		/*
		 * We've now got a variable structure to store in. But first,
		 * advance the string pointer.
		 */
		str += length;

		/*
		 * Copy all the characters from the variable value straight
		 * into the new string.
		 */
		length = strlen(val);
		Buf_AddBytes(&buf, length, val);
		trailingBslash = length > 0 && val[length - 1] == '\\';
	    }
	    free(freeIt);
	    freeIt = NULL;
	}
    }

    return Buf_DestroyCompact(&buf);
}

/* Initialize the module. */
void
Var_Init(void)
{
    VAR_INTERNAL = Targ_NewGN("Internal");
    VAR_GLOBAL = Targ_NewGN("Global");
    VAR_CMD = Targ_NewGN("Command");
}


void
Var_End(void)
{
    Var_Stats();
}

void
Var_Stats(void)
{
    Hash_DebugStats(&VAR_GLOBAL->context, "VAR_GLOBAL");
}


/****************** PRINT DEBUGGING INFO *****************/
static void
VarPrintVar(void *vp, void *data MAKE_ATTR_UNUSED)
{
    Var *v = (Var *)vp;
    fprintf(debug_file, "%-16s = %s\n", v->name, Buf_GetAll(&v->val, NULL));
}

/*-
 *-----------------------------------------------------------------------
 * Var_Dump --
 *	print all variables in a context
 *-----------------------------------------------------------------------
 */
void
Var_Dump(GNode *ctxt)
{
    Hash_ForEach(&ctxt->context, VarPrintVar, NULL);
}
