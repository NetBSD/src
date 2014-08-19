/*	$NetBSD: boolean.c,v 1.1.1.2.6.1 2014/08/19 23:51:47 tls Exp $	*/


/**
 * \file boolean.c
 *
 * Handle options with true/false values for arguments.
 *
 * @addtogroup autoopts
 * @{
 */
/*
 *  This routine will run run-on options through a pager so the
 *  user may examine, print or edit them at their leisure.
 *
 *  This file is part of AutoOpts, a companion to AutoGen.
 *  AutoOpts is free software.
 *  AutoOpts is Copyright (C) 1992-2013 by Bruce Korb - all rights reserved
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
 *  These files have the following sha256 sums:
 *
 *  8584710e9b04216a394078dc156b781d0b47e1729104d666658aecef8ee32e95  COPYING.gplv3
 *  4379e7444a0e2ce2b12dd6f5a52a27a4d02d39d247901d3285c88cf0d37f477b  COPYING.lgplv3
 *  13aa749a5b0a454917a944ed8fffc530b784f5ead522b1aacaf4ec8aa55a6239  COPYING.mbsd
 */

/*=export_func  optionBooleanVal
 * private:
 *
 * what:  Decipher a boolean value
 * arg:   + tOptions* + pOpts    + program options descriptor +
 * arg:   + tOptDesc* + pOptDesc + the descriptor for this arg +
 *
 * doc:
 *  Decipher a true or false value for a boolean valued option argument.
 *  The value is true, unless it starts with 'n' or 'f' or "#f" or
 *  it is an empty string or it is a number that evaluates to zero.
=*/
void
optionBooleanVal(tOptions * pOpts, tOptDesc * pOD )
{
    char* pz;
    bool  res = true;

    (void)pOpts;

    if (pOpts <= OPTPROC_EMIT_LIMIT)
        return;

    if ((pOD->fOptState & OPTST_RESET) != 0)
        return;

    if (pOD->optArg.argString == NULL) {
        pOD->optArg.argBool = false;
        return;
    }

    switch (*(pOD->optArg.argString)) {
    case '0':
    {
        long  val = strtol( pOD->optArg.argString, &pz, 0 );
        if ((val != 0) || (*pz != NUL))
            break;
        /* FALLTHROUGH */
    }
    case 'N':
    case 'n':
    case 'F':
    case 'f':
    case NUL:
        res = false;
        break;
    case '#':
        if (pOD->optArg.argString[1] != 'f')
            break;
        res = false;
    }

    if (pOD->fOptState & OPTST_ALLOC_ARG) {
        AGFREE(pOD->optArg.argString);
        pOD->fOptState &= ~OPTST_ALLOC_ARG;
    }
    pOD->optArg.argBool = res;
}
/** @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of autoopts/boolean.c */
