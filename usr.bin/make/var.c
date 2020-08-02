/*	$NetBSD: var.c,v 1.399 2020/08/02 16:06:49 rillig Exp $	*/

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
static char rcsid[] = "$NetBSD: var.c,v 1.399 2020/08/02 16:06:49 rillig Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)var.c	8.3 (Berkeley) 3/19/94";
#else
__RCSID("$NetBSD: var.c,v 1.399 2020/08/02 16:06:49 rillig Exp $");
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
#include    <assert.h>
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

    if (var == NULL && (flags & FIND_CMD) && ctxt != VAR_CMD)
	var = Hash_FindEntry(&VAR_CMD->context, name);

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

	    size_t len = strlen(env);
	    Buf_InitZ(&v->val, len + 1);
	    Buf_AddBytesZ(&v->val, env, len);

	    v->flags = VAR_FROM_ENV;
	    return v;
	}

	if (checkEnvFirst && (flags & FIND_GLOBAL) && ctxt != VAR_GLOBAL) {
	    var = Hash_FindEntry(&VAR_GLOBAL->context, name);
	    if (var == NULL && ctxt != VAR_INTERNAL)
		var = Hash_FindEntry(&VAR_INTERNAL->context, name);
	    if (var == NULL)
		return NULL;
	    else
		return (Var *)Hash_GetValue(var);
	}

	return NULL;
    }

    if (var == NULL)
	return NULL;
    else
	return (Var *)Hash_GetValue(var);
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

    size_t len = val != NULL ? strlen(val) : 0;
    Buf_InitZ(&v->val, len + 1);
    Buf_AddBytesZ(&v->val, val, len);

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
    char *name_freeIt = NULL;
    if (strchr(name, '$') != NULL)
	name = name_freeIt = Var_Subst(name, VAR_GLOBAL, VARE_WANTRES);
    Hash_Entry *ln = Hash_FindEntry(&ctxt->context, name);
    if (DEBUG(VAR)) {
	fprintf(debug_file, "%s:delete %s%s\n",
	    ctxt->name, name, ln != NULL ? "" : " (not found)");
    }
    free(name_freeIt);

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
    if (!parent && (v->flags & VAR_EXPORTED) && !(v->flags & VAR_REEXPORT))
	return 0;		/* nothing to do */
    val = Buf_GetAllZ(&v->val, NULL);
    if (!(flags & VAR_EXPORT_LITERAL) && strchr(val, '$')) {
	if (parent) {
	    /*
	     * Flag this as something we need to re-export.
	     * No point actually exporting it now though,
	     * the child can do it at the last minute.
	     */
	    v->flags |= VAR_EXPORTED | VAR_REEXPORT;
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
	    val = Var_Subst(tmp, VAR_GLOBAL, VARE_WANTRES);
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

    val = Var_Subst("${" MAKE_EXPORTED ":O:u}", VAR_GLOBAL, VARE_WANTRES);
    if (*val) {
	char **av;
	char *as;
	int ac;
	int i;

	av = brk_string(val, &ac, FALSE, &as);
	for (i = 0; i < ac; i++)
	    Var_Export1(av[i], 0);
	free(as);
	free(av);
    }
    free(val);
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

    char *val = Var_Subst(str, VAR_GLOBAL, VARE_WANTRES);
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
    unexport_env = strncmp(str, "-env", 4) == 0;
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
	vlist = Var_Subst("${" MAKE_EXPORTED ":O:u}", VAR_GLOBAL,
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
		    cp = Var_Subst(tmp, VAR_GLOBAL, VARE_WANTRES);
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
    char *name_freeIt = NULL;

    /*
     * We only look for a variable in the given context since anything set
     * here will override anything in a lower context, so there's not much
     * point in searching them all just to save a bit of memory...
     */
    if (strchr(name, '$') != NULL) {
	const char *unexpanded_name = name;
	name = name_freeIt = Var_Subst(name, ctxt, VARE_WANTRES);
	if (name[0] == '\0') {
	    if (DEBUG(VAR)) {
		fprintf(debug_file, "Var_Set(\"%s\", \"%s\", ...) "
			"name expands to empty string - ignored\n",
			unexpanded_name, val);
	    }
	    free(name_freeIt);
	    return;
	}
    }
    if (ctxt == VAR_GLOBAL) {
	v = VarFind(name, VAR_CMD, 0);
	if (v != NULL) {
	    if (v->flags & VAR_FROM_CMD) {
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
	if (ctxt == VAR_CMD && !(flags & VAR_NO_EXPORT)) {
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
	if (v->flags & VAR_EXPORTED) {
	    Var_Export1(name, VAR_EXPORT_PARENT);
	}
    }
    /*
     * Any variables given on the command line are automatically exported
     * to the environment (as per POSIX standard)
     */
    if (ctxt == VAR_CMD && !(flags & VAR_NO_EXPORT)) {
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
    free(name_freeIt);
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
	expanded_name = Var_Subst(name, ctxt, VARE_WANTRES);
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
		    Buf_GetAllZ(&v->val, NULL));
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
    char *name_freeIt = NULL;
    if (strchr(name, '$') != NULL)
	name = name_freeIt = Var_Subst(name, ctxt, VARE_WANTRES);

    Var *v = VarFind(name, ctxt, FIND_CMD | FIND_GLOBAL | FIND_ENV);
    free(name_freeIt);
    if (v == NULL)
	return FALSE;

    (void)VarFreeEnv(v, TRUE);
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * Var_Value --
 *	Return the unexpanded value of the given variable in the given
 *	context, or the usual contexts.
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
const char *
Var_Value(const char *name, GNode *ctxt, char **freeIt)
{
    Var *v = VarFind(name, ctxt, FIND_ENV | FIND_GLOBAL | FIND_CMD);
    *freeIt = NULL;
    if (v == NULL)
	return NULL;

    char *p = Buf_GetAllZ(&v->val, NULL);
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
    Buf_InitZ(&buf->buf, 32 /* bytes */);
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
    Buf_AddBytesZ(&buf->buf, mem, mem_size);
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
 *
 * Results:
 *	Returns the start of the match, or NULL.
 *	*match_len returns the length of the match, if any.
 *	*hasPercent returns whether the pattern contains a percent.
 *-----------------------------------------------------------------------
 */
static const char *
Str_SYSVMatch(const char *word, const char *pattern, size_t *match_len,
    Boolean *hasPercent)
{
    const char *p = pattern;
    const char *w = word;

    *hasPercent = FALSE;
    if (*p == '\0') {		/* ${VAR:=suffix} */
	*match_len = strlen(w);	/* Null pattern is the whole string */
	return w;
    }

    const char *percent = strchr(p, '%');
    if (percent != NULL) {	/* ${VAR:...%...=...} */
	*hasPercent = TRUE;
	if (*w == '\0')
	    return NULL;	/* empty word does not match pattern */

	/* check that the prefix matches */
	for (; p != percent && *w != '\0' && *w == *p; w++, p++)
	     continue;
	if (p != percent)
	    return NULL;	/* No match */

	p++;			/* Skip the percent */
	if (*p == '\0') {
	    /* No more pattern, return the rest of the string */
	    *match_len = strlen(w);
	    return w;
	}
    }

    /* Test whether the tail matches */
    size_t w_len = strlen(w);
    size_t p_len = strlen(p);
    if (w_len < p_len)
	return NULL;

    const char *w_tail = w + w_len - p_len;
    if (memcmp(p, w_tail, p_len) != 0)
	return NULL;

    *match_len = w_tail - w;
    return w;
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

    size_t match_len;
    Boolean lhsPercent;
    const char *match = Str_SYSVMatch(word, args->lhs, &match_len, &lhsPercent);
    if (match == NULL) {
	SepBuf_AddStr(buf, word);
	return;
    }

    /* Append rhs to the buffer, substituting the first '%' with the
     * match, but only if the lhs had a '%' as well. */

    char *rhs_expanded = Var_Subst(args->rhs, args->ctx, VARE_WANTRES);

    const char *rhs = rhs_expanded;
    const char *percent = strchr(rhs, '%');

    if (percent != NULL && lhsPercent) {
	/* Copy the prefix of the replacement pattern */
	SepBuf_AddBytesBetween(buf, rhs, percent);
	rhs = percent + 1;
    }
    if (percent != NULL || !lhsPercent)
	SepBuf_AddBytes(buf, match, match_len);

    /* Append the suffix of the replacement pattern */
    SepBuf_AddStr(buf, rhs);

    free(rhs_expanded);
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
    char *s = Var_Subst(args->str, args->ctx, args->eflags);
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
	modifyWord(av[i], &result, data);
	if (result.buf.count > 0)
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
 *	otype		How to order: s - sort, r - reverse, x - random.
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

    Buf_InitZ(&buf, 0);

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

    Buf_InitZ(&buf, 0);
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
 * Parse a text part of a modifier such as the "from" and "to" in :S/from/to/
 * or the :@ modifier, until the next unescaped delimiter.  The delimiter, as
 * well as the backslash or the dollar, can be escaped with a backslash.
 *
 * Return the parsed (and possibly expanded) string, or NULL if no delimiter
 * was found.
 */
static char *
ParseModifierPart(
    const char **pp,		/* The parsing position, updated upon return */
    int delim,			/* Parsing stops at this delimiter */
    VarEvalFlags eflags,	/* Flags for evaluating nested variables;
				 * if VARE_WANTRES is not set, the text is
				 * only parsed */
    GNode *ctxt,		/* For looking up nested variables */
    size_t *out_length,		/* Optionally stores the length of the returned
				 * string, just to save another strlen call. */
    VarPatternFlags *out_pflags,/* For the first part of the :S modifier,
				 * sets the VARP_ANCHOR_END flag if the last
				 * character of the pattern is a $. */
    ModifyWord_SubstArgs *subst	/* For the second part of the :S modifier,
				 * allow ampersands to be escaped and replace
				 * unescaped ampersands with subst->lhs. */
) {
    Buffer buf;

    Buf_InitZ(&buf, 0);

    /*
     * Skim through until the matching delimiter is found;
     * pick up variable substitutions on the way. Also allow
     * backslashes to quote the delimiter, $, and \, but don't
     * touch other backslashes.
     */
    const char *p = *pp;
    while (*p != '\0' && *p != delim) {
	Boolean is_escaped = p[0] == '\\' && (
	    p[1] == delim || p[1] == '\\' || p[1] == '$' ||
	    (p[1] == '&' && subst != NULL));
	if (is_escaped) {
	    Buf_AddByte(&buf, p[1]);
	    p += 2;
	    continue;
	}

	if (*p != '$') {	/* Unescaped, simple text */
	    if (subst != NULL && *p == '&')
		Buf_AddBytesZ(&buf, subst->lhs, subst->lhsLen);
	    else
		Buf_AddByte(&buf, *p);
	    p++;
	    continue;
	}

	if (p[1] == delim) {	/* Unescaped $ at end of pattern */
	    if (out_pflags != NULL)
		*out_pflags |= VARP_ANCHOR_END;
	    else
		Buf_AddByte(&buf, *p);
	    p++;
	    continue;
	}

	if (eflags & VARE_WANTRES) {	/* Nested variable, evaluated */
	    const char *cp2;
	    int     len;
	    void   *freeIt;

	    cp2 = Var_Parse(p, ctxt, eflags & ~VARE_ASSIGN, &len, &freeIt);
	    Buf_AddStr(&buf, cp2);
	    free(freeIt);
	    p += len;
	    continue;
	}

	/* XXX: This whole block is very similar to Var_Parse without
	 * VARE_WANTRES.  There may be subtle edge cases though that are
	 * not yet covered in the unit tests and that are parsed differently,
	 * depending on whether they are evaluated or not.
	 *
	 * This subtle difference is not documented in the manual page,
	 * neither is the difference between parsing :D and :M documented.
	 * No code should ever depend on these details, but who knows. */

	const char *varstart = p;	/* Nested variable, only parsed */
	if (p[1] == PROPEN || p[1] == BROPEN) {
	    /*
	     * Find the end of this variable reference
	     * and suck it in without further ado.
	     * It will be interpreted later.
	     */
	    int have = p[1];
	    int want = have == PROPEN ? PRCLOSE : BRCLOSE;
	    int depth = 1;

	    for (p += 2; *p != '\0' && depth > 0; ++p) {
		if (p[-1] != '\\') {
		    if (*p == have)
			++depth;
		    if (*p == want)
			--depth;
		}
	    }
	    Buf_AddBytesBetween(&buf, varstart, p);
	} else {
	    Buf_AddByte(&buf, *varstart);
	    p++;
	}
    }

    if (*p != delim) {
	*pp = p;
	return NULL;
    }

    *pp = ++p;
    if (out_length != NULL)
	*out_length = Buf_Size(&buf);

    char *rstr = Buf_Destroy(&buf, FALSE);
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
    Buf_InitZ(&buf, 0);

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

    Buf_InitZ(&buf, 0);
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

/* The ApplyModifier functions all work in the same way.
 * They parse the modifier (often until the next colon) and store the
 * updated position for the parser into st->next
 * (except when returning AMR_UNKNOWN).
 * They take the st->val and generate st->newVal from it.
 * On failure, many of them update st->missing_delim.
 */
typedef struct {
    const int startc;		/* '\0' or '{' or '(' */
    const int endc;
    Var * const v;
    GNode * const ctxt;
    const VarEvalFlags eflags;

    char *val;			/* The value of the expression before the
				 * modifier is applied */
    char *newVal;		/* The new value after applying the modifier
				 * to the expression */
    const char *next;		/* The position where parsing continues
				 * after the current modifier. */
    char missing_delim;		/* For error reporting */

    Byte	sep;		/* Word separator in expansions */
    Boolean	oneBigWord;	/* TRUE if we will treat the variable as a
				 * single big word, even if it contains
				 * embedded spaces (as opposed to the
				 * usual behaviour of treating it as
				 * several space-separated words). */

} ApplyModifiersState;

typedef enum {
    AMR_OK,			/* Continue parsing */
    AMR_UNKNOWN,		/* Not a match, try others as well */
    AMR_BAD,			/* Error out with message */
    AMR_CLEANUP			/* Error out with "missing delimiter",
				 * if st->missing_delim is set. */
} ApplyModifierResult;

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
static ApplyModifierResult
ApplyModifier_Loop(const char *mod, ApplyModifiersState *st) {
    ModifyWord_LoopArgs args;

    args.ctx = st->ctxt;
    st->next = mod + 1;
    char delim = '@';
    args.tvar = ParseModifierPart(&st->next, delim, st->eflags & ~VARE_WANTRES,
				  st->ctxt, NULL, NULL, NULL);
    if (args.tvar == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    args.str = ParseModifierPart(&st->next, delim, st->eflags & ~VARE_WANTRES,
				 st->ctxt, NULL, NULL, NULL);
    if (args.str == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    args.eflags = st->eflags & (VARE_UNDEFERR | VARE_WANTRES);
    int prev_sep = st->sep;
    st->sep = ' ';		/* XXX: this is inconsistent */
    st->newVal = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->val,
			     ModifyWord_Loop, &args);
    st->sep = prev_sep;
    Var_Delete(args.tvar, st->ctxt);
    free(args.tvar);
    free(args.str);
    return AMR_OK;
}

/* :Ddefined or :Uundefined */
static ApplyModifierResult
ApplyModifier_Defined(const char *mod, ApplyModifiersState *st)
{
    Buffer buf;			/* Buffer for patterns */
    VarEvalFlags neflags;

    if (st->eflags & VARE_WANTRES) {
	Boolean wantres;
	if (*mod == 'U')
	    wantres = (st->v->flags & VAR_JUNK) != 0;
	else
	    wantres = (st->v->flags & VAR_JUNK) == 0;
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
    Buf_InitZ(&buf, 0);
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

    st->next = p;

    if (st->v->flags & VAR_JUNK)
	st->v->flags |= VAR_KEEP;
    if (neflags & VARE_WANTRES) {
	st->newVal = Buf_Destroy(&buf, FALSE);
    } else {
	st->newVal = st->val;
	Buf_Destroy(&buf, TRUE);
    }
    return AMR_OK;
}

/* :gmtime */
static ApplyModifierResult
ApplyModifier_Gmtime(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatchEq(mod, "gmtime", st->endc))
	return AMR_UNKNOWN;

    time_t utc;
    if (mod[6] == '=') {
	char *ep;
	utc = strtoul(mod + 7, &ep, 10);
	st->next = ep;
    } else {
	utc = 0;
	st->next = mod + 6;
    }
    st->newVal = VarStrftime(st->val, 1, utc);
    return AMR_OK;
}

/* :localtime */
static Boolean
ApplyModifier_Localtime(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatchEq(mod, "localtime", st->endc))
	return AMR_UNKNOWN;

    time_t utc;
    if (mod[9] == '=') {
	char *ep;
	utc = strtoul(mod + 10, &ep, 10);
	st->next = ep;
    } else {
	utc = 0;
	st->next = mod + 9;
    }
    st->newVal = VarStrftime(st->val, 0, utc);
    return AMR_OK;
}

/* :hash */
static ApplyModifierResult
ApplyModifier_Hash(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatch(mod, "hash", st->endc))
	return AMR_UNKNOWN;

    st->newVal = VarHash(st->val);
    st->next = mod + 4;
    return AMR_OK;
}

/* :P */
static ApplyModifierResult
ApplyModifier_Path(const char *mod, ApplyModifiersState *st)
{
    if (st->v->flags & VAR_JUNK)
	st->v->flags |= VAR_KEEP;
    GNode *gn = Targ_FindNode(st->v->name, TARG_NOCREATE);
    if (gn == NULL || gn->type & OP_NOPATH) {
	st->newVal = NULL;
    } else if (gn->path) {
	st->newVal = bmake_strdup(gn->path);
    } else {
	st->newVal = Dir_FindFile(st->v->name, Suff_FindPath(gn));
    }
    if (!st->newVal)
	st->newVal = bmake_strdup(st->v->name);
    st->next = mod + 1;
    return AMR_OK;
}

/* :!cmd! */
static ApplyModifierResult
ApplyModifier_Exclam(const char *mod, ApplyModifiersState *st)
{
    st->next = mod + 1;
    char delim = '!';
    char *cmd = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				  NULL, NULL, NULL);
    if (cmd == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    const char *emsg = NULL;
    if (st->eflags & VARE_WANTRES)
	st->newVal = Cmd_Exec(cmd, &emsg);
    else
	st->newVal = varNoError;
    free(cmd);

    if (emsg)
	Error(emsg, st->val);	/* XXX: why still return AMR_OK? */

    if (st->v->flags & VAR_JUNK)
	st->v->flags |= VAR_KEEP;
    return AMR_OK;
}

/* The :range modifier generates an integer sequence as long as the words.
 * The :range=7 modifier generates an integer sequence from 1 to 7. */
static ApplyModifierResult
ApplyModifier_Range(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatchEq(mod, "range", st->endc))
	return AMR_UNKNOWN;

    int n;
    if (mod[5] == '=') {
	char *ep;
	n = strtoul(mod + 6, &ep, 10);
	st->next = ep;
    } else {
	n = 0;
	st->next = mod + 5;
    }

    if (n == 0) {
	char *as;
	char **av = brk_string(st->val, &n, FALSE, &as);
	free(as);
	free(av);
    }

    Buffer buf;
    Buf_InitZ(&buf, 0);

    int i;
    for (i = 0; i < n; i++) {
	if (i != 0)
	    Buf_AddByte(&buf, ' ');
	Buf_AddInt(&buf, 1 + i);
    }

    st->newVal = Buf_Destroy(&buf, FALSE);
    return AMR_OK;
}

/* :Mpattern or :Npattern */
static ApplyModifierResult
ApplyModifier_Match(const char *mod, ApplyModifiersState *st)
{
    Boolean copy = FALSE;	/* pattern should be, or has been, copied */
    Boolean needSubst = FALSE;
    /*
     * In the loop below, ignore ':' unless we are at (or back to) the
     * original brace level.
     * XXX This will likely not work right if $() and ${} are intermixed.
     */
    int nest = 0;
    const char *p;
    for (p = mod + 1; *p != '\0' && !(*p == ':' && nest == 0); p++) {
	if (*p == '\\' &&
	    (p[1] == ':' || p[1] == st->endc || p[1] == st->startc)) {
	    if (!needSubst)
		copy = TRUE;
	    p++;
	    continue;
	}
	if (*p == '$')
	    needSubst = TRUE;
	if (*p == '(' || *p == '{')
	    ++nest;
	if (*p == ')' || *p == '}') {
	    --nest;
	    if (nest < 0)
		break;
	}
    }
    st->next = p;
    const char *endpat = st->next;

    char *pattern;
    if (copy) {
	/* Compress the \:'s out of the pattern. */
	pattern = bmake_malloc(endpat - (mod + 1) + 1);
	char *dst = pattern;
	const char *src = mod + 1;
	for (; src < endpat; src++, dst++) {
	    if (src[0] == '\\' && src + 1 < endpat &&
		/* XXX: st->startc is missing here; see above */
		(src[1] == ':' || src[1] == st->endc))
		src++;
	    *dst = *src;
	}
	*dst = '\0';
	endpat = dst;
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
	pattern = Var_Subst(pattern, st->ctxt, st->eflags);
	free(old_pattern);
    }

    if (DEBUG(VAR))
	fprintf(debug_file, "Pattern[%s] for [%s] is [%s]\n",
	    st->v->name, st->val, pattern);

    ModifyWordsCallback callback = mod[0] == 'M'
	? ModifyWord_Match : ModifyWord_NoMatch;
    st->newVal = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->val,
			     callback, pattern);
    free(pattern);
    return AMR_OK;
}

/* :S,from,to, */
static ApplyModifierResult
ApplyModifier_Subst(const char * const mod, ApplyModifiersState *st)
{
    char delim = mod[1];
    if (delim == '\0') {
	Error("Missing delimiter for :S modifier");
	st->next = mod + 1;
	return AMR_CLEANUP;
    }

    st->next = mod + 2;

    ModifyWord_SubstArgs args;
    args.pflags = 0;

    /*
     * If pattern begins with '^', it is anchored to the
     * start of the word -- skip over it and flag pattern.
     */
    if (*st->next == '^') {
	args.pflags |= VARP_ANCHOR_START;
	st->next++;
    }

    char *lhs = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				  &args.lhsLen, &args.pflags, NULL);
    if (lhs == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }
    args.lhs = lhs;

    char *rhs = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				  &args.rhsLen, NULL, &args);
    if (rhs == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }
    args.rhs = rhs;

    Boolean oneBigWord = st->oneBigWord;
    for (;; st->next++) {
	switch (*st->next) {
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

    st->newVal = ModifyWords(st->ctxt, st->sep, oneBigWord, st->val,
			     ModifyWord_Subst, &args);

    free(lhs);
    free(rhs);
    return AMR_OK;
}

#ifndef NO_REGEX

/* :C,from,to, */
static ApplyModifierResult
ApplyModifier_Regex(const char *mod, ApplyModifiersState *st)
{
    char delim = mod[1];
    if (delim == '\0') {
	Error("Missing delimiter for :C modifier");
	st->next = mod + 1;
	return AMR_CLEANUP;
    }

    st->next = mod + 2;

    char *re = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				 NULL, NULL, NULL);
    if (re == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    ModifyWord_SubstRegexArgs args;
    args.replace = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				     NULL, NULL, NULL);
    if (args.replace == NULL) {
	free(re);
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    args.pflags = 0;
    Boolean oneBigWord = st->oneBigWord;
    for (;; st->next++) {
	switch (*st->next) {
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

    int error = regcomp(&args.re, re, REG_EXTENDED);
    free(re);
    if (error) {
	VarREError(error, &args.re, "Regex compilation error");
	free(args.replace);
	return AMR_CLEANUP;
    }

    args.nsub = args.re.re_nsub + 1;
    if (args.nsub < 1)
	args.nsub = 1;
    if (args.nsub > 10)
	args.nsub = 10;
    st->newVal = ModifyWords(st->ctxt, st->sep, oneBigWord, st->val,
			     ModifyWord_SubstRegex, &args);
    regfree(&args.re);
    free(args.replace);
    return AMR_OK;
}
#endif

static void
ModifyWord_Copy(const char *word, SepBuf *buf, void *data MAKE_ATTR_UNUSED)
{
    SepBuf_AddStr(buf, word);
}

/* :ts<separator> */
static ApplyModifierResult
ApplyModifier_ToSep(const char *sep, ApplyModifiersState *st)
{
    if (sep[0] != st->endc && (sep[1] == st->endc || sep[1] == ':')) {
	/* ":ts<any><endc>" or ":ts<any>:" */
	st->sep = sep[0];
	st->next = sep + 1;
    } else if (sep[0] == st->endc || sep[0] == ':') {
	/* ":ts<endc>" or ":ts:" */
	st->sep = '\0';		/* no separator */
	st->next = sep;
    } else if (sep[0] == '\\') {
	const char *xp = sep + 1;
	int base = 8;		/* assume octal */

	switch (sep[1]) {
	case 'n':
	    st->sep = '\n';
	    st->next = sep + 2;
	    break;
	case 't':
	    st->sep = '\t';
	    st->next = sep + 2;
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
		return AMR_BAD;	/* ":ts<backslash><unrecognised>". */

	    char *end;
	get_numeric:
	    st->sep = strtoul(sep + 1 + (sep[1] == 'x'), &end, base);
	    if (*end != ':' && *end != st->endc)
		return AMR_BAD;
	    st->next = end;
	    break;
	}
    } else {
	return AMR_BAD;		/* Found ":ts<unrecognised><unrecognised>". */
    }

    st->newVal = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->val,
			     ModifyWord_Copy, NULL);
    return AMR_OK;
}

/* :tA, :tu, :tl, :ts<separator>, etc. */
static ApplyModifierResult
ApplyModifier_To(const char *mod, ApplyModifiersState *st)
{
    assert(mod[0] == 't');

    st->next = mod + 1;		/* make sure it is set */
    if (mod[1] == st->endc || mod[1] == ':' || mod[1] == '\0')
	return AMR_BAD;		/* Found ":t<endc>" or ":t:". */

    if (mod[1] == 's')
	return ApplyModifier_ToSep(mod + 2, st);

    if (mod[2] != st->endc && mod[2] != ':')
	return AMR_BAD;		/* Found ":t<unrecognised><unrecognised>". */

    /* Check for two-character options: ":tu", ":tl" */
    if (mod[1] == 'A') {	/* absolute path */
	st->newVal = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->val,
				 ModifyWord_Realpath, NULL);
	st->next = mod + 2;
    } else if (mod[1] == 'u') {
	size_t len = strlen(st->val);
	st->newVal = bmake_malloc(len + 1);
	size_t i;
	for (i = 0; i < len + 1; i++)
	    st->newVal[i] = toupper((unsigned char)st->val[i]);
	st->next = mod + 2;
    } else if (mod[1] == 'l') {
	size_t len = strlen(st->val);
	st->newVal = bmake_malloc(len + 1);
	size_t i;
	for (i = 0; i < len + 1; i++)
	    st->newVal[i] = tolower((unsigned char)st->val[i]);
	st->next = mod + 2;
    } else if (mod[1] == 'W' || mod[1] == 'w') {
	st->oneBigWord = mod[1] == 'W';
	st->newVal = st->val;
	st->next = mod + 2;
    } else {
	/* Found ":t<unrecognised>:" or ":t<unrecognised><endc>". */
	return AMR_BAD;
    }
    return AMR_OK;
}

/* :[#], :[1], etc. */
static ApplyModifierResult
ApplyModifier_Words(const char *mod, ApplyModifiersState *st)
{
    st->next = mod + 1;		/* point to char after '[' */
    char delim = ']';		/* look for closing ']' */
    char *estr = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				   NULL, NULL, NULL);
    if (estr == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    /* now st->next points just after the closing ']' */
    if (st->next[0] != ':' && st->next[0] != st->endc)
	goto bad_modifier;	/* Found junk after ']' */

    if (estr[0] == '\0')
	goto bad_modifier;	/* empty square brackets in ":[]". */

    if (estr[0] == '#' && estr[1] == '\0') { /* Found ":[#]" */
	if (st->oneBigWord) {
	    st->newVal = bmake_strdup("1");
	} else {
	    /* XXX: brk_string() is a rather expensive
	     * way of counting words. */
	    char *as;
	    int ac;
	    char **av = brk_string(st->val, &ac, FALSE, &as);
	    free(as);
	    free(av);

	    Buffer buf;
	    Buf_InitZ(&buf, 4);	/* 3 digits + '\0' */
	    Buf_AddInt(&buf, ac);
	    st->newVal = Buf_Destroy(&buf, FALSE);
	}
	goto ok;
    }

    if (estr[0] == '*' && estr[1] == '\0') {
	/* Found ":[*]" */
	st->oneBigWord = TRUE;
	st->newVal = st->val;
	goto ok;
    }

    if (estr[0] == '@' && estr[1] == '\0') {
	/* Found ":[@]" */
	st->oneBigWord = FALSE;
	st->newVal = st->val;
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
	st->newVal = st->val;
	goto ok;
    }

    /* ":[0..N]" or ":[N..0]" */
    if (first == 0 || last == 0)
	goto bad_modifier;

    /* Normal case: select the words described by seldata. */
    st->newVal = VarSelectWords(st->sep, st->oneBigWord, st->val, first, last);

ok:
    free(estr);
    return AMR_OK;

bad_modifier:
    free(estr);
    return AMR_BAD;
}

/* :O or :Or or :Ox */
static ApplyModifierResult
ApplyModifier_Order(const char *mod, ApplyModifiersState *st)
{
    char otype;

    st->next = mod + 1;	/* skip to the rest in any case */
    if (mod[1] == st->endc || mod[1] == ':') {
	otype = 's';
    } else if ((mod[1] == 'r' || mod[1] == 'x') &&
	       (mod[2] == st->endc || mod[2] == ':')) {
	otype = mod[1];
	st->next = mod + 2;
    } else {
	return AMR_BAD;
    }
    st->newVal = VarOrder(st->val, otype);
    return AMR_OK;
}

/* :? then : else */
static ApplyModifierResult
ApplyModifier_IfElse(const char *mod, ApplyModifiersState *st)
{
    Boolean value = FALSE;
    VarEvalFlags then_eflags = st->eflags & ~VARE_WANTRES;
    VarEvalFlags else_eflags = st->eflags & ~VARE_WANTRES;

    int cond_rc = COND_PARSE;	/* anything other than COND_INVALID */
    if (st->eflags & VARE_WANTRES) {
	cond_rc = Cond_EvalExpression(NULL, st->v->name, &value, 0, FALSE);
	if (cond_rc != COND_INVALID && value)
	    then_eflags |= VARE_WANTRES;
	if (cond_rc != COND_INVALID && !value)
	    else_eflags |= VARE_WANTRES;
    }

    st->next = mod + 1;
    char delim = ':';
    char *then_expr = ParseModifierPart(&st->next, delim, then_eflags, st->ctxt,
					NULL, NULL, NULL);
    if (then_expr == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    delim = st->endc;		/* BRCLOSE or PRCLOSE */
    char *else_expr = ParseModifierPart(&st->next, delim, else_eflags, st->ctxt,
					NULL, NULL, NULL);
    if (else_expr == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    st->next--;
    if (cond_rc == COND_INVALID) {
	Error("Bad conditional expression `%s' in %s?%s:%s",
	    st->v->name, st->v->name, then_expr, else_expr);
	return AMR_CLEANUP;
    }

    if (value) {
	st->newVal = then_expr;
	free(else_expr);
    } else {
	st->newVal = else_expr;
	free(then_expr);
    }
    if (st->v->flags & VAR_JUNK)
	st->v->flags |= VAR_KEEP;
    return AMR_OK;
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
static ApplyModifierResult
ApplyModifier_Assign(const char *mod, ApplyModifiersState *st)
{
    const char *op = mod + 1;
    if (!(op[0] == '=' ||
	(op[1] == '=' &&
	 (op[0] == '!' || op[0] == '+' || op[0] == '?'))))
	return AMR_UNKNOWN;	/* "::<unrecognised>" */

    GNode *v_ctxt;		/* context where v belongs */

    if (st->v->name[0] == 0) {
	st->next = mod + 1;
	return AMR_BAD;
    }

    v_ctxt = st->ctxt;
    char *sv_name = NULL;
    if (st->v->flags & VAR_JUNK) {
	/*
	 * We need to bmake_strdup() it in case ParseModifierPart() recurses.
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
	st->next = mod + 3;
	break;
    default:
	st->next = mod + 2;
	break;
    }

    char delim = st->startc == PROPEN ? PRCLOSE : BRCLOSE;
    char *val = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				  NULL, NULL, NULL);
    if (st->v->flags & VAR_JUNK) {
	/* restore original name */
	free(st->v->name);
	st->v->name = sv_name;
    }
    if (val == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    st->next--;

    if (st->eflags & VARE_WANTRES) {
	switch (op[0]) {
	case '+':
	    Var_Append(st->v->name, val, v_ctxt);
	    break;
	case '!': {
	    const char *emsg;
	    st->newVal = Cmd_Exec(val, &emsg);
	    if (emsg)
		Error(emsg, st->val);
	    else
		Var_Set(st->v->name, st->newVal, v_ctxt);
	    free(st->newVal);
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
    st->newVal = varNoError;
    return AMR_OK;
}

/* remember current value */
static ApplyModifierResult
ApplyModifier_Remember(const char *mod, ApplyModifiersState *st)
{
    if (!ModMatchEq(mod, "_", st->endc))
	return AMR_UNKNOWN;

    if (mod[1] == '=') {
	size_t n = strcspn(mod + 2, ":)}");
	char *name = bmake_strndup(mod + 2, n);
	Var_Set(name, st->val, st->ctxt);
	free(name);
	st->next = mod + 2 + n;
    } else {
	Var_Set("_", st->val, st->ctxt);
	st->next = mod + 1;
    }
    st->newVal = st->val;
    return AMR_OK;
}

#ifdef SYSVVARSUB
/* :from=to */
static ApplyModifierResult
ApplyModifier_SysV(const char *mod, ApplyModifiersState *st)
{
    Boolean eqFound = FALSE;

    /*
     * First we make a pass through the string trying
     * to verify it is a SYSV-make-style translation:
     * it must be: <string1>=<string2>)
     */
    st->next = mod;
    int nest = 1;
    while (*st->next != '\0' && nest > 0) {
	if (*st->next == '=') {
	    eqFound = TRUE;
	    /* continue looking for st->endc */
	} else if (*st->next == st->endc)
	    nest--;
	else if (*st->next == st->startc)
	    nest++;
	if (nest > 0)
	    st->next++;
    }
    if (*st->next != st->endc || !eqFound)
	return AMR_UNKNOWN;

    char delim = '=';
    st->next = mod;
    char *lhs = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				  NULL, NULL, NULL);
    if (lhs == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    delim = st->endc;
    char *rhs = ParseModifierPart(&st->next, delim, st->eflags, st->ctxt,
				  NULL, NULL, NULL);
    if (rhs == NULL) {
	st->missing_delim = delim;
	return AMR_CLEANUP;
    }

    /*
     * SYSV modifications happen through the whole
     * string. Note the pattern is anchored at the end.
     */
    st->next--;
    if (lhs[0] == '\0' && *st->val == '\0') {
	st->newVal = st->val;	/* special case */
    } else {
	ModifyWord_SYSVSubstArgs args = { st->ctxt, lhs, rhs };
	st->newVal = ModifyWords(st->ctxt, st->sep, st->oneBigWord, st->val,
				 ModifyWord_SYSVSubst, &args);
    }
    free(lhs);
    free(rhs);
    return AMR_OK;
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
 *			true-value, else return false-value.
 *    	  :lhs=rhs  	Similar to :S, but the rhs goes to the end of
 *    			the invocation, including any ':'.
 *	  :sh		Treat the current value as a command
 *			to be run, new value is its output.
 * The following added so we can handle ODE makefiles.
 *	  :@<tmpvar>@<newval>@
 *			Assign a temporary global variable <tmpvar>
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
 *	  :!<cmd>!	Run cmd much the same as :sh runs the
 *			current value of the variable.
 * Assignment operators (see ApplyModifier_Assign).
 */
static char *
ApplyModifiers(
    const char **pp,		/* the parsing position, updated upon return */
    char *val,			/* the current value of the variable */
    int const startc,		/* '(' or '{' or '\0' */
    int const endc,		/* ')' or '}' or '\0' */
    Var * const v,		/* the variable may have its flags changed */
    GNode * const ctxt,		/* for looking up and modifying variables */
    VarEvalFlags const eflags,
    void ** const freePtr	/* free this after using the return value */
) {
    assert(startc == '(' || startc == '{' || startc == '\0');
    assert(endc == ')' || endc == '}' || startc == '\0');

    ApplyModifiersState st = {
	startc, endc, v, ctxt, eflags,
	val, NULL, NULL, '\0', ' ', FALSE
    };

    const char *p = *pp;
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
		(c = p[rlen]) != '\0' && c != ':' && c != st.endc) {
		free(freeIt);
		goto apply_mods;
	    }

	    if (DEBUG(VAR)) {
		fprintf(debug_file, "Got '%s' from '%.*s'%.*s\n",
		       rval, rlen, p, rlen, p + rlen);
	    }

	    p += rlen;

	    if (rval != NULL && *rval) {
		const char *rval_pp = rval;
		st.val = ApplyModifiers(&rval_pp, st.val, 0, 0, v,
					ctxt, eflags, freePtr);
		if (st.val == var_Error
		    || (st.val == varNoError && !(st.eflags & VARE_UNDEFERR))
		    || *rval_pp != '\0') {
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
		*p, st.val);
	}
	st.newVal = var_Error;		/* default value, in case of errors */
	st.next = NULL; /* fail fast if an ApplyModifier forgets to set this */
	ApplyModifierResult res = AMR_BAD;	/* just a safe fallback */
	char modifier = *p;
	switch (modifier) {
	case ':':
	    res = ApplyModifier_Assign(p, &st);
	    break;
	case '@':
	    res = ApplyModifier_Loop(p, &st);
	    break;
	case '_':
	    res = ApplyModifier_Remember(p, &st);
	    break;
	case 'D':
	case 'U':
	    res = ApplyModifier_Defined(p, &st);
	    break;
	case 'L':
	    if (st.v->flags & VAR_JUNK)
		st.v->flags |= VAR_KEEP;
	    st.newVal = bmake_strdup(st.v->name);
	    st.next = p + 1;
	    res = AMR_OK;
	    break;
	case 'P':
	    res = ApplyModifier_Path(p, &st);
	    break;
	case '!':
	    res = ApplyModifier_Exclam(p, &st);
	    break;
	case '[':
	    res = ApplyModifier_Words(p, &st);
	    break;
	case 'g':
	    res = ApplyModifier_Gmtime(p, &st);
	    break;
	case 'h':
	    res = ApplyModifier_Hash(p, &st);
	    break;
	case 'l':
	    res = ApplyModifier_Localtime(p, &st);
	    break;
	case 't':
	    res = ApplyModifier_To(p, &st);
	    break;
	case 'N':
	case 'M':
	    res = ApplyModifier_Match(p, &st);
	    break;
	case 'S':
	    res = ApplyModifier_Subst(p, &st);
	    break;
	case '?':
	    res = ApplyModifier_IfElse(p, &st);
	    break;
#ifndef NO_REGEX
	case 'C':
	    res = ApplyModifier_Regex(p, &st);
	    break;
#endif
	case 'q':
	case 'Q':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newVal = VarQuote(st.val, modifier == 'q');
		st.next = p + 1;
		res = AMR_OK;
	    } else
		res = AMR_UNKNOWN;
	    break;
	case 'T':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newVal = ModifyWords(st.ctxt, st.sep, st.oneBigWord,
					st.val, ModifyWord_Tail, NULL);
		st.next = p + 1;
		res = AMR_OK;
	    } else
		res = AMR_UNKNOWN;
	    break;
	case 'H':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newVal = ModifyWords(st.ctxt, st.sep, st.oneBigWord,
					st.val, ModifyWord_Head, NULL);
		st.next = p + 1;
		res = AMR_OK;
	    } else
		res = AMR_UNKNOWN;
	    break;
	case 'E':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newVal = ModifyWords(st.ctxt, st.sep, st.oneBigWord,
					st.val, ModifyWord_Suffix, NULL);
		st.next = p + 1;
		res = AMR_OK;
	    } else
		res = AMR_UNKNOWN;
	    break;
	case 'R':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newVal = ModifyWords(st.ctxt, st.sep, st.oneBigWord,
					st.val, ModifyWord_Root, NULL);
		st.next = p + 1;
		res = AMR_OK;
	    } else
		res = AMR_UNKNOWN;
	    break;
	case 'r':
	    res = ApplyModifier_Range(p, &st);
	    break;
	case 'O':
	    res = ApplyModifier_Order(p, &st);
	    break;
	case 'u':
	    if (p[1] == st.endc || p[1] == ':') {
		st.newVal = VarUniq(st.val);
		st.next = p + 1;
		res = AMR_OK;
	    } else
		res = AMR_UNKNOWN;
	    break;
#ifdef SUNSHCMD
	case 's':
	    if (p[1] == 'h' && (p[2] == st.endc || p[2] == ':')) {
		const char *emsg;
		if (st.eflags & VARE_WANTRES) {
		    st.newVal = Cmd_Exec(st.val, &emsg);
		    if (emsg)
			Error(emsg, st.val);
		} else
		    st.newVal = varNoError;
		st.next = p + 2;
		res = AMR_OK;
	    } else
		res = AMR_UNKNOWN;
	    break;
#endif
	default:
	    res = AMR_UNKNOWN;
	}

#ifdef SYSVVARSUB
	if (res == AMR_UNKNOWN)
	    res = ApplyModifier_SysV(p, &st);
#endif

	if (res == AMR_UNKNOWN) {
	    Error("Unknown modifier '%c'", *p);
	    st.next = p + 1;
	    while (*st.next != ':' && *st.next != st.endc && *st.next != '\0')
		st.next++;
	    st.newVal = var_Error;
	}
	if (res == AMR_CLEANUP)
	    goto cleanup;
	if (res == AMR_BAD)
	    goto bad_modifier;

	if (DEBUG(VAR)) {
	    fprintf(debug_file, "Result[%s] of :%c is \"%s\"\n",
		st.v->name, modifier, st.newVal);
	}

	if (st.newVal != st.val) {
	    if (*freePtr) {
		free(st.val);
		*freePtr = NULL;
	    }
	    st.val = st.newVal;
	    if (st.val != var_Error && st.val != varNoError) {
		*freePtr = st.val;
	    }
	}
	if (*st.next == '\0' && st.endc != '\0') {
	    Error("Unclosed variable specification (expecting '%c') "
		"for \"%s\" (value \"%s\") modifier %c",
		st.endc, st.v->name, st.val, modifier);
	} else if (*st.next == ':') {
	    st.next++;
	}
	p = st.next;
    }
out:
    *pp = p;
    return st.val;

bad_modifier:
    Error("Bad modifier `:%.*s' for %s",
	  (int)strcspn(p, ":)}"), p, st.v->name);

cleanup:
    *pp = st.next;
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

	    if (ctxt == VAR_CMD || ctxt == VAR_GLOBAL) {
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
	}
    } else {
	endc = startc == PROPEN ? PRCLOSE : BRCLOSE;

	Buffer namebuf;		/* Holds the variable name */
	Buf_InitZ(&namebuf, 0);

	/*
	 * Skip to the end character or a colon, whichever comes first.
	 */
	int depth = 1;
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
	    Parse_Error(PARSE_FATAL, "Unclosed variable \"%s\"",
			Buf_GetAllZ(&namebuf, NULL));
	    /*
	     * If we never did find the end character, return NULL
	     * right now, setting the length to be the distance to
	     * the end of the string, since that's what make does.
	     */
	    *lengthPtr = tstr - str;
	    Buf_Destroy(&namebuf, TRUE);
	    return var_Error;
	}

	size_t namelen;
	char *varname = Buf_GetAllZ(&namebuf, &namelen);

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
		Buf_InitZ(&v->val, 1);
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
    nstr = Buf_GetAllZ(&v->val, NULL);
    if (strchr(nstr, '$') != NULL && (eflags & VARE_WANTRES) != 0) {
	nstr = Var_Subst(nstr, ctxt, eflags);
	*freePtr = nstr;
    }

    v->flags &= ~VAR_IN_USE;

    if (nstr != NULL && (haveModifier || extramodifiers != NULL)) {
	void *extraFree;

	extraFree = NULL;
	if (extramodifiers != NULL) {
	    const char *em = extramodifiers;
	    nstr = ApplyModifiers(&em, nstr, '(', ')',
				  v, ctxt, eflags, &extraFree);
	}

	if (haveModifier) {
	    /* Skip initial colon. */
	    tstr++;

	    nstr = ApplyModifiers(&tstr, nstr, startc, endc,
				  v, ctxt, eflags, freePtr);
	    free(extraFree);
	} else {
	    *freePtr = extraFree;
	}
    }
    *lengthPtr = tstr - str + (*tstr ? 1 : 0);

    if (v->flags & VAR_FROM_ENV) {
	Boolean destroy = nstr != Buf_GetAllZ(&v->val, NULL);
	if (!destroy) {
	    /*
	     * Returning the value unmodified, so tell the caller to free
	     * the thing.
	     */
	    *freePtr = nstr;
	}
	(void)VarFreeEnv(v, destroy);
    } else if (v->flags & VAR_JUNK) {
	/*
	 * Perform any free'ing needed and set *freePtr to NULL so the caller
	 * doesn't try to free a static pointer.
	 * If VAR_KEEP is also set then we want to keep str(?) as is.
	 */
	if (!(v->flags & VAR_KEEP)) {
	    if (*freePtr != NULL) {
		free(*freePtr);
		*freePtr = NULL;
	    }
	    if (dynamic) {
		nstr = bmake_strndup(str, *lengthPtr);
		*freePtr = nstr;
	    } else {
		nstr = (eflags & VARE_UNDEFERR) ? var_Error : varNoError;
	    }
	}
	if (nstr != Buf_GetAllZ(&v->val, NULL))
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
Var_Subst(const char *str, GNode *ctxt, VarEvalFlags eflags)
{
    Buffer	buf;		/* Buffer for forming things */
    const char	*val;		/* Value to substitute for a variable */
    int		length;		/* Length of the variable invocation */
    Boolean	trailingBslash;	/* variable ends in \ */
    void	*freeIt = NULL;	/* Set if it should be freed */
    static Boolean errorReported; /* Set true if an error has already
				 * been reported to prevent a plethora
				 * of messages when recursing */

    Buf_InitZ(&buf, 0);
    errorReported = FALSE;
    trailingBslash = FALSE;

    while (*str) {
	if (*str == '\n' && trailingBslash)
	    Buf_AddByte(&buf, ' ');
	if (*str == '$' && str[1] == '$') {
	    /*
	     * A dollar sign may be escaped with another dollar sign.
	     * In such a case, we skip over the escape character and store the
	     * dollar sign into the buffer directly.
	     */
	    if (save_dollars && (eflags & VARE_ASSIGN))
		Buf_AddByte(&buf, '$');
	    Buf_AddByte(&buf, '$');
	    str += 2;
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
		size_t val_len = strlen(val);
		Buf_AddBytesZ(&buf, val, val_len);
		trailingBslash = val_len > 0 && val[val_len - 1] == '\\';
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
    fprintf(debug_file, "%-16s = %s\n", v->name, Buf_GetAllZ(&v->val, NULL));
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
