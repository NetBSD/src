#ifndef lint
static char rcsid[] = "$Id: version.c,v 1.2 1993/08/02 17:28:50 mycroft Exp $";
#endif /* not lint */

#if defined(__STDC__) || defined(const)
const
#endif
char version_string[] = "GNU assembler version 1.38\n";

/* DO NOT PUT COMMENTS ABOUT CHANGES IN THIS FILE.

   This file exists only to define `version_string'.

   Log changes in ChangeLog.  The easiest way to do this is with
   the Emacs command `add-change-log-entry'.  If you don't use Emacs,
   add entries of the form:

Thu Jan  1 00:00:00 1970  Dennis Ritchie  (dmr at alice)

	* universe.c (temporal_reality): Began Time.
*/

#ifdef VMS
dummy3()
{
}
#endif
