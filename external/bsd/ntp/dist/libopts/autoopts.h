/*	$NetBSD: autoopts.h,v 1.2.6.1 2012/04/17 00:03:46 yamt Exp $	*/


/*
 *  Time-stamp:      "2008-11-01 20:08:06 bkorb"
 *
 *  autoopts.h  Id: d5e30331d37ca10ec88c592d24d8615dd6c1f0ee
 *
 *  This file defines all the global structures and special values
 *  used in the automated option processing library.
 *
 *  This file is part of AutoOpts, a companion to AutoGen.
 *  AutoOpts is free software.
 *  AutoOpts is copyright (c) 1992-2009 by Bruce Korb - all rights reserved
 *
 *  AutoOpts is available under any one of two licenses.  The license
 *  in use must be one of these two and the choice is under the control
 *  of the user of the license.
 *
 *   The GNU Lesser General Public License, version 3 or later
 *      See the files "COPYING.lgplv3" and "COPYING.gplv3"
 *
 *   The Modified Berkeley Software Distribution License
 *      See the file "COPYING.mbsd"
 *
 *  These files have the following md5sums:
 *
 *  43b91e8ca915626ed3818ffb1b71248b pkg/libopts/COPYING.gplv3
 *  06a1a2e4760c90ea5e1dad8dfaac4d39 pkg/libopts/COPYING.lgplv3
 *  66a5cedaf62c4b2637025f049f9b826f pkg/libopts/COPYING.mbsd
 */

#ifndef AUTOGEN_AUTOOPTS_H
#define AUTOGEN_AUTOOPTS_H

#include "compat/compat.h"
#include "ag-char-map.h"

#define AO_NAME_LIMIT           127
#define AO_NAME_SIZE            ((size_t)(AO_NAME_LIMIT + 1))

#ifndef AG_PATH_MAX
#  ifdef PATH_MAX
#    define AG_PATH_MAX         ((size_t)PATH_MAX)
#  else
#    define AG_PATH_MAX         ((size_t)4096)
#  endif
#else
#  if defined(PATH_MAX) && (PATH_MAX > MAXPATHLEN)
#     undef  AG_PATH_MAX
#     define AG_PATH_MAX        ((size_t)PATH_MAX)
#  endif
#endif

#undef  EXPORT
#define EXPORT

#if defined(_WIN32) && !defined(__CYGWIN__)
# define DIRCH                  '\\'
#else
# define DIRCH                  '/'
#endif

#ifndef EX_NOINPUT
#  define EX_NOINPUT            66
#endif
#ifndef EX_SOFTWARE
#  define EX_SOFTWARE           70
#endif
#ifndef EX_CONFIG
#  define EX_CONFIG             78
#endif

/*
 *  Convert the number to a list usable in a printf call
 */
#define NUM_TO_VER(n)           ((n) >> 12), ((n) >> 7) & 0x001F, (n) & 0x007F

#define NAMED_OPTS(po) \
        (((po)->fOptSet & (OPTPROC_SHORTOPT | OPTPROC_LONGOPT)) == 0)

#define SKIP_OPT(p)  (((p)->fOptState & (OPTST_DOCUMENT|OPTST_OMITTED)) != 0)

typedef int tDirection;
#define DIRECTION_PRESET        -1
#define DIRECTION_PROCESS       1
#define DIRECTION_CALLED        0

#define PROCESSING(d)           ((d)>0)
#define PRESETTING(d)           ((d)<0)

/*
 *  Procedure success codes
 *
 *  USAGE:  define procedures to return "tSuccess".  Test their results
 *          with the SUCCEEDED, FAILED and HADGLITCH macros.
 *
 *  Microsoft sticks its nose into user space here, so for Windows' sake,
 *  make sure all of these are undefined.
 */
#undef  SUCCESS
#undef  FAILURE
#undef  PROBLEM
#undef  SUCCEEDED
#undef  SUCCESSFUL
#undef  FAILED
#undef  HADGLITCH

#define SUCCESS                 ((tSuccess) 0)
#define FAILURE                 ((tSuccess)-1)
#define PROBLEM                 ((tSuccess) 1)

typedef int tSuccess;

#define SUCCEEDED( p )          ((p) == SUCCESS)
#define SUCCESSFUL( p )         SUCCEEDED( p )
#define FAILED( p )             ((p) <  SUCCESS)
#define HADGLITCH( p )          ((p) >  SUCCESS)

/*
 *  When loading a line (or block) of text as an option, the value can
 *  be processed in any of several modes:
 *
 *  @table @samp
 *  @item keep
 *  Every part of the value between the delimiters is saved.
 *
 *  @item uncooked
 *  Even if the value begins with quote characters, do not do quote processing.
 *
 *  @item cooked
 *  If the value looks like a quoted string, then process it.
 *  Double quoted strings are processed the way strings are in "C" programs,
 *  except they are treated as regular characters if the following character
 *  is not a well-established escape sequence.
 *  Single quoted strings (quoted with apostrophies) are handled the way
 *  strings are handled in shell scripts, *except* that backslash escapes
 *  are honored before backslash escapes and apostrophies.
 *  @end table
 */
typedef enum {
    OPTION_LOAD_COOKED,
    OPTION_LOAD_UNCOOKED,
    OPTION_LOAD_KEEP
} tOptionLoadMode;

extern tOptionLoadMode option_load_mode;

/*
 *  The pager state is used by optionPagedUsage() procedure.
 *  When it runs, it sets itself up to be called again on exit.
 *  If, however, a routine needs a child process to do some work
 *  before it is done, then 'pagerState' must be set to
 *  'PAGER_STATE_CHILD' so that optionPagedUsage() will not try
 *  to run the pager program before its time.
 */
typedef enum {
    PAGER_STATE_INITIAL,
    PAGER_STATE_READY,
    PAGER_STATE_CHILD
} tePagerState;

extern tePagerState pagerState;

typedef enum {
    ENV_ALL,
    ENV_IMM,
    ENV_NON_IMM
} teEnvPresetType;

typedef enum {
    TOPT_UNDEFINED = 0,
    TOPT_SHORT,
    TOPT_LONG,
    TOPT_DEFAULT
} teOptType;

typedef struct {
    tOptDesc*  pOD;
    tCC*       pzOptArg;
    tAoUL      flags;
    teOptType  optType;
} tOptState;
#define OPTSTATE_INITIALIZER(st) \
    { NULL, NULL, OPTST_ ## st, TOPT_UNDEFINED }

#define TEXTTO_TABLE \
        _TT_( LONGUSAGE ) \
        _TT_( USAGE ) \
        _TT_( VERSION )
#define _TT_(n) \
        TT_ ## n ,

typedef enum { TEXTTO_TABLE COUNT_TT } teTextTo;

#undef _TT_

typedef struct {
    tCC*    pzStr;
    tCC*    pzReq;
    tCC*    pzNum;
    tCC*    pzFile;
    tCC*    pzKey;
    tCC*    pzKeyL;
    tCC*    pzBool;
    tCC*    pzNest;
    tCC*    pzOpt;
    tCC*    pzNo;
    tCC*    pzBrk;
    tCC*    pzNoF;
    tCC*    pzSpc;
    tCC*    pzOptFmt;
    tCC*    pzTime;
} arg_types_t;

#define AGALOC( c, w )          ao_malloc((size_t)c)
#define AGREALOC( p, c, w )     ao_realloc((void*)p, (size_t)c)
#define AGFREE(_p)              ao_free((void*)(intptr_t)(p))
#define AGDUPSTR( p, s, w )     (p = ao_strdup(s))

static void *
ao_malloc( size_t sz );

static void *
ao_realloc( void *p, size_t sz );

static void
ao_free( void *p );

static char *
ao_strdup( char const *str );

#define TAGMEM( m, t )

/*
 *  DO option handling?
 *
 *  Options are examined at two times:  at immediate handling time and at
 *  normal handling time.  If an option is disabled, the timing may be
 *  different from the handling of the undisabled option.  The OPTST_DIABLED
 *  bit indicates the state of the currently discovered option.
 *  So, here's how it works:
 *
 *  A) handling at "immediate" time, either 1 or 2:
 *
 *  1.  OPTST_DISABLED is not set:
 *      IMM           must be set
 *      DISABLE_IMM   don't care
 *      TWICE         don't care
 *      DISABLE_TWICE don't care
 *      0 -and-  1 x x x
 *
 *  2.  OPTST_DISABLED is set:
 *      IMM           don't care
 *      DISABLE_IMM   must be set
 *      TWICE         don't care
 *      DISABLE_TWICE don't care
 *      1 -and-  x 1 x x
 */
#define DO_IMMEDIATELY(_flg) \
    (  (((_flg) & (OPTST_DISABLED|OPTST_IMM)) == OPTST_IMM) \
    || (   ((_flg) & (OPTST_DISABLED|OPTST_DISABLE_IMM))    \
        == (OPTST_DISABLED|OPTST_DISABLE_IMM)  ))

/*  B) handling at "regular" time because it was not immediate
 *
 *  1.  OPTST_DISABLED is not set:
 *      IMM           must *NOT* be set
 *      DISABLE_IMM   don't care
 *      TWICE         don't care
 *      DISABLE_TWICE don't care
 *      0 -and-  0 x x x
 *
 *  2.  OPTST_DISABLED is set:
 *      IMM           don't care
 *      DISABLE_IMM   don't care
 *      TWICE         must be set
 *      DISABLE_TWICE don't care
 *      1 -and-  x x 1 x
 */
#define DO_NORMALLY(_flg) ( \
       (((_flg) & (OPTST_DISABLED|OPTST_IMM))            == 0)  \
    || (((_flg) & (OPTST_DISABLED|OPTST_DISABLE_IMM))    ==     \
                  OPTST_DISABLED)  )

/*  C)  handling at "regular" time because it is to be handled twice.
 *      The immediate bit was already tested and found to be set:
 *
 *  3.  OPTST_DISABLED is not set:
 *      IMM           is set (but don't care)
 *      DISABLE_IMM   don't care
 *      TWICE         must be set
 *      DISABLE_TWICE don't care
 *      0 -and-  ? x 1 x
 *
 *  4.  OPTST_DISABLED is set:
 *      IMM           don't care
 *      DISABLE_IMM   is set (but don't care)
 *      TWICE         don't care
 *      DISABLE_TWICE must be set
 *      1 -and-  x ? x 1
 */
#define DO_SECOND_TIME(_flg) ( \
       (((_flg) & (OPTST_DISABLED|OPTST_TWICE))          ==     \
                  OPTST_TWICE)                                  \
    || (((_flg) & (OPTST_DISABLED|OPTST_DISABLE_TWICE))  ==     \
                  (OPTST_DISABLED|OPTST_DISABLE_TWICE)  ))

/*
 *  text_mmap structure.  Only active on platforms with mmap(2).
 */
#ifdef HAVE_SYS_MMAN_H
#  include <sys/mman.h>
#else
#  ifndef  PROT_READ
#   define PROT_READ            0x01
#  endif
#  ifndef  PROT_WRITE
#   define PROT_WRITE           0x02
#  endif
#  ifndef  MAP_SHARED
#   define MAP_SHARED           0x01
#  endif
#  ifndef  MAP_PRIVATE
#   define MAP_PRIVATE          0x02
#  endif
#endif

#ifndef MAP_FAILED
#  define  MAP_FAILED           ((void*)-1)
#endif

#ifndef  _SC_PAGESIZE
# ifdef  _SC_PAGE_SIZE
#  define _SC_PAGESIZE          _SC_PAGE_SIZE
# endif
#endif

#ifndef HAVE_STRCHR
extern char* strchr( char const *s, int c);
extern char* strrchr( char const *s, int c);
#endif

/*
 *  Define and initialize all the user visible strings.
 *  We do not do translations.  If translations are to be done, then
 *  the client will provide a callback for that purpose.
 */
#undef DO_TRANSLATIONS
#include "autoopts/usage-txt.h"

/*
 *  File pointer for usage output
 */
extern FILE* option_usage_fp;

extern tOptProc optionPrintVersion, optionPagedUsage, optionLoadOpt;

#endif /* AUTOGEN_AUTOOPTS_H */
/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of autoopts/autoopts.h */
