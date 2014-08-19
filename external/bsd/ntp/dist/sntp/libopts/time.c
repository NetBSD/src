/*	$NetBSD: time.c,v 1.1.1.2.6.1 2014/08/19 23:51:47 tls Exp $	*/


/**
 * \file time.c
 *
 * @addtogroup autoopts
 * @{
 */
/*
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

/*=export_func  optionTimeVal
 * private:
 *
 * what:  process an option with a time duration.
 * arg:   + tOptions* + pOpts    + program options descriptor +
 * arg:   + tOptDesc* + pOptDesc + the descriptor for this arg +
 *
 * doc:
 *  Decipher a time duration value.
=*/
void
optionTimeVal(tOptions * pOpts, tOptDesc * pOD)
{
    time_t val;

    if (pOpts <= OPTPROC_EMIT_LIMIT)
        return;

    if ((pOD->fOptState & OPTST_RESET) != 0)
        return;

    val = parse_duration(pOD->optArg.argString);
    if (val == BAD_TIME) {
        fprintf(stderr, zNotDuration, pOpts->pzProgName, pOD->optArg.argString);
        if ((pOpts->fOptSet & OPTPROC_ERRSTOP) != 0)
            (*(pOpts->pUsageProc))(pOpts, EXIT_FAILURE);
    }

    if (pOD->fOptState & OPTST_ALLOC_ARG) {
        AGFREE(pOD->optArg.argString);
        pOD->fOptState &= ~OPTST_ALLOC_ARG;
    }

    pOD->optArg.argInt = (long)val;
}

/*=export_func  optionTimeDate
 * private:
 *
 * what:  process an option with a time and date.
 * arg:   + tOptions* + pOpts    + program options descriptor +
 * arg:   + tOptDesc* + pOptDesc + the descriptor for this arg +
 *
 * doc:
 *  Decipher a time and date value.
=*/
void
optionTimeDate(tOptions * pOpts, tOptDesc * pOD)
{
#if defined(HAVE_GETDATE_R) && defined(HAVE_PUTENV)
    if (pOpts <= OPTPROC_EMIT_LIMIT)
        return;

    if ((! HAS_pzPkgDataDir(pOpts)) || (pOpts->pzPkgDataDir == NULL))
        goto default_action;

    /*
     *  Export the DATEMSK environment variable.  getdate_r() uses it to
     *  find the file with the strptime formats.  If we cannot find the file
     *  we need ($PKGDATADIR/datemsk), then fall back to just a time duration.
     */
    {
        static char * envptr = NULL;

        if (envptr == NULL) {
            static char const fmt[] = "DATEMSK=%s/datemsk";
            envptr = AGALOC(sizeof(fmt) + strlen(pOpts->pzPkgDataDir), fmt);
            sprintf(envptr, fmt, pOpts->pzPkgDataDir);

            putenv(envptr);
        }

        if (access(envptr+8, R_OK) != 0)
            goto default_action;
    }

    /*
     *  Convert the date to a time since the epoch and stash it in a long int.
     */
    {
        struct tm stm;
        time_t tm;

        if (getdate_r(pOD->optArg.argString, &stm) != 0) {
            fprintf(stderr, zNotDate, pOpts->pzProgName,
                    pOD->optArg.argString);
            if ((pOpts->fOptSet & OPTPROC_ERRSTOP) != 0)
                (*(pOpts->pUsageProc))(pOpts, EXIT_FAILURE);
            return;
        }

        tm = mktime(&stm);

        if (pOD->fOptState & OPTST_ALLOC_ARG) {
            AGFREE(pOD->optArg.argString);
            pOD->fOptState &= ~OPTST_ALLOC_ARG;
        }

        pOD->optArg.argInt = tm;
    }
    return;

 default_action:

#endif
    optionTimeVal(pOpts, pOD);
    if (pOD->optArg.argInt != BAD_TIME)
        pOD->optArg.argInt += (long)time(NULL);
}
/** @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of autoopts/time.c */
