/*	$NetBSD: ntpq-opts.h,v 1.1.1.1.6.1 2012/04/17 00:03:49 yamt Exp $	*/

/*  
 *  EDIT THIS FILE WITH CAUTION  (ntpq-opts.h)
 *  
 *  It has been AutoGen-ed  December 24, 2011 at 06:34:27 PM by AutoGen 5.12
 *  From the definitions    ntpq-opts.def
 *  and the template file   options
 *
 * Generated from AutoOpts 35:0:10 templates.
 *
 *  AutoOpts is a copyrighted work.  This header file is not encumbered
 *  by AutoOpts licensing, but is provided under the licensing terms chosen
 *  by the ntpq author or copyright holder.  AutoOpts is
 *  licensed under the terms of the LGPL.  The redistributable library
 *  (``libopts'') is licensed under the terms of either the LGPL or, at the
 *  users discretion, the BSD license.  See the AutoOpts and/or libopts sources
 *  for details.
 *
 * This source file is copyrighted and licensed under the following terms:
 *
 *  see html/copyright.html
 *  
 */
/*
 *  This file contains the programmatic interface to the Automated
 *  Options generated for the ntpq program.
 *  These macros are documented in the AutoGen info file in the
 *  "AutoOpts" chapter.  Please refer to that doc for usage help.
 */
#ifndef AUTOOPTS_NTPQ_OPTS_H_GUARD
#define AUTOOPTS_NTPQ_OPTS_H_GUARD 1
#include "config.h"
#include <autoopts/options.h>

/*
 *  Ensure that the library used for compiling this generated header is at
 *  least as new as the version current when the header template was released
 *  (not counting patch version increments).  Also ensure that the oldest
 *  tolerable version is at least as old as what was current when the header
 *  template was released.
 */
#define AO_TEMPLATE_VERSION 143360
#if (AO_TEMPLATE_VERSION < OPTIONS_MINIMUM_VERSION) \
 || (AO_TEMPLATE_VERSION > OPTIONS_STRUCT_VERSION)
# error option template version mismatches autoopts/options.h header
  Choke Me.
#endif

/*
 *  Enumeration of each option:
 */
typedef enum {
    INDEX_OPT_IPV4             =  0,
    INDEX_OPT_IPV6             =  1,
    INDEX_OPT_COMMAND          =  2,
    INDEX_OPT_DEBUG_LEVEL      =  3,
    INDEX_OPT_SET_DEBUG_LEVEL  =  4,
    INDEX_OPT_PEERS            =  5,
    INDEX_OPT_INTERACTIVE      =  6,
    INDEX_OPT_NUMERIC          =  7,
    INDEX_OPT_OLD_RV           =  8,
    INDEX_OPT_VERSION          =  9,
    INDEX_OPT_HELP             = 10,
    INDEX_OPT_MORE_HELP        = 11,
    INDEX_OPT_SAVE_OPTS        = 12,
    INDEX_OPT_LOAD_OPTS        = 13
} teOptIndex;

#define OPTION_CT    14
#define NTPQ_VERSION       "4.2.6p5"
#define NTPQ_FULL_VERSION  "ntpq 4.2.6p5"

/*
 *  Interface defines for all options.  Replace "n" with the UPPER_CASED
 *  option name (as in the teOptIndex enumeration above).
 *  e.g. HAVE_OPT(IPV4)
 */
#define         DESC(n) (ntpqOptions.pOptDesc[INDEX_OPT_## n])
#define     HAVE_OPT(n) (! UNUSED_OPT(& DESC(n)))
#define      OPT_ARG(n) (DESC(n).optArg.argString)
#define    STATE_OPT(n) (DESC(n).fOptState & OPTST_SET_MASK)
#define    COUNT_OPT(n) (DESC(n).optOccCt)
#define    ISSEL_OPT(n) (SELECTED_OPT(&DESC(n)))
#define ISUNUSED_OPT(n) (UNUSED_OPT(& DESC(n)))
#define  ENABLED_OPT(n) (! DISABLED_OPT(& DESC(n)))
#define  STACKCT_OPT(n) (((tArgList*)(DESC(n).optCookie))->useCt)
#define STACKLST_OPT(n) (((tArgList*)(DESC(n).optCookie))->apzArgs)
#define    CLEAR_OPT(n) STMTS( \
                DESC(n).fOptState &= OPTST_PERSISTENT_MASK;   \
                if ((DESC(n).fOptState & OPTST_INITENABLED) == 0) \
                    DESC(n).fOptState |= OPTST_DISABLED; \
                DESC(n).optCookie = NULL )

/* * * * * *
 *
 *  Enumeration of ntpq exit codes
 */
typedef enum {
    NTPQ_EXIT_SUCCESS = 0,
    NTPQ_EXIT_FAILURE = 1
} ntpq_exit_code_t;
/*
 *  Make sure there are no #define name conflicts with the option names
 */
#ifndef     NO_OPTION_NAME_WARNINGS
# ifdef    IPV4
#  warning undefining IPV4 due to option name conflict
#  undef   IPV4
# endif
# ifdef    IPV6
#  warning undefining IPV6 due to option name conflict
#  undef   IPV6
# endif
# ifdef    COMMAND
#  warning undefining COMMAND due to option name conflict
#  undef   COMMAND
# endif
# ifdef    DEBUG_LEVEL
#  warning undefining DEBUG_LEVEL due to option name conflict
#  undef   DEBUG_LEVEL
# endif
# ifdef    SET_DEBUG_LEVEL
#  warning undefining SET_DEBUG_LEVEL due to option name conflict
#  undef   SET_DEBUG_LEVEL
# endif
# ifdef    PEERS
#  warning undefining PEERS due to option name conflict
#  undef   PEERS
# endif
# ifdef    INTERACTIVE
#  warning undefining INTERACTIVE due to option name conflict
#  undef   INTERACTIVE
# endif
# ifdef    NUMERIC
#  warning undefining NUMERIC due to option name conflict
#  undef   NUMERIC
# endif
# ifdef    OLD_RV
#  warning undefining OLD_RV due to option name conflict
#  undef   OLD_RV
# endif
#else  /* NO_OPTION_NAME_WARNINGS */
# undef IPV4
# undef IPV6
# undef COMMAND
# undef DEBUG_LEVEL
# undef SET_DEBUG_LEVEL
# undef PEERS
# undef INTERACTIVE
# undef NUMERIC
# undef OLD_RV
#endif  /*  NO_OPTION_NAME_WARNINGS */

/* * * * * *
 *
 *  Interface defines for specific options.
 */
#define VALUE_OPT_IPV4           '4'
#define VALUE_OPT_IPV6           '6'
#define VALUE_OPT_COMMAND        'c'
#define VALUE_OPT_DEBUG_LEVEL    'd'
#define VALUE_OPT_SET_DEBUG_LEVEL 'D'
#define VALUE_OPT_PEERS          'p'
#define VALUE_OPT_INTERACTIVE    'i'
#define VALUE_OPT_NUMERIC        'n'
#define VALUE_OPT_OLD_RV         8
#define VALUE_OPT_HELP          '?'
#define VALUE_OPT_MORE_HELP     '!'
#define VALUE_OPT_VERSION       INDEX_OPT_VERSION
#define VALUE_OPT_SAVE_OPTS     '>'
#define VALUE_OPT_LOAD_OPTS     '<'
#define SET_OPT_SAVE_OPTS(a)   STMTS( \
        DESC(SAVE_OPTS).fOptState &= OPTST_PERSISTENT_MASK; \
        DESC(SAVE_OPTS).fOptState |= OPTST_SET; \
        DESC(SAVE_OPTS).optArg.argString = (char const*)(a) )
/*
 *  Interface defines not associated with particular options
 */
#define ERRSKIP_OPTERR  STMTS(ntpqOptions.fOptSet &= ~OPTPROC_ERRSTOP)
#define ERRSTOP_OPTERR  STMTS(ntpqOptions.fOptSet |= OPTPROC_ERRSTOP)
#define RESTART_OPT(n)  STMTS( \
                ntpqOptions.curOptIdx = (n); \
                ntpqOptions.pzCurOpt  = NULL)
#define START_OPT       RESTART_OPT(1)
#define USAGE(c)        (*ntpqOptions.pUsageProc)(&ntpqOptions, c)
/* extracted from opthead.tlib near line 451 */

#ifdef  __cplusplus
extern "C" {
#endif

/* * * * * *
 *
 *  Declare the ntpq option descriptor.
 */
extern tOptions ntpqOptions;

#if defined(ENABLE_NLS)
# ifndef _
#   include <stdio.h>
static inline char* aoGetsText(char const* pz) {
    if (pz == NULL) return NULL;
    return (char*)gettext(pz);
}
#   define _(s)  aoGetsText(s)
# endif /* _() */

# define OPT_NO_XLAT_CFG_NAMES  STMTS(ntpqOptions.fOptSet |= \
                                    OPTPROC_NXLAT_OPT_CFG;)
# define OPT_NO_XLAT_OPT_NAMES  STMTS(ntpqOptions.fOptSet |= \
                                    OPTPROC_NXLAT_OPT|OPTPROC_NXLAT_OPT_CFG;)

# define OPT_XLAT_CFG_NAMES     STMTS(ntpqOptions.fOptSet &= \
                                  ~(OPTPROC_NXLAT_OPT|OPTPROC_NXLAT_OPT_CFG);)
# define OPT_XLAT_OPT_NAMES     STMTS(ntpqOptions.fOptSet &= \
                                  ~OPTPROC_NXLAT_OPT;)

#else   /* ENABLE_NLS */
# define OPT_NO_XLAT_CFG_NAMES
# define OPT_NO_XLAT_OPT_NAMES

# define OPT_XLAT_CFG_NAMES
# define OPT_XLAT_OPT_NAMES

# ifndef _
#   define _(_s)  _s
# endif
#endif  /* ENABLE_NLS */

#ifdef  __cplusplus
}
#endif
#endif /* AUTOOPTS_NTPQ_OPTS_H_GUARD */
/* ntpq-opts.h ends here */
