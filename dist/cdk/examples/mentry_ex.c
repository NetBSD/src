#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="mentry_ex";
#endif

int main (int argc GCC_UNUSED, char **argv)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKMENTRY *filename	= 0;
   WINDOW *cursesWin	= 0;
   char *info		= 0;
   char *label		= "</R>Message";
   char *title		= "<C></5>Enter a message.<!5>";
   int exitType;

   /* Set up CDK. */ 
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors.*/
   initCDKColor();

   /* Set up the multi-line entry field. */
   filename	= newCDKMentry (cdkscreen, CENTER, CENTER,
			title, label, A_BOLD, '.', vMIXED,
			20, 5, 20, 0, TRUE, TRUE);

   /* Is the object null? */
   if (filename == 0)
   {
      /* Shut down CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message. */
      printf ("Oops. Can not create CDK object. Is the window too small?\n");
      exit (1);
   }

   /* Draw the CDK screen. */
   refreshCDKScreen (cdkscreen);

   /* Set what ever was given from the command line. */
   setCDKMentry (filename, argv[1], 0, TRUE);

   /* Activate this thing. */
   activateCDKMentry (filename, 0);
   exitType = filename->exitType;
   if (exitType == vNORMAL)
   {
      info = copyChar (filename->info);
   }

   /* Clean up. */
   destroyCDKMentry (filename);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();

   /* Spit out the results. */
   if (exitType == vESCAPE_HIT)
   {
      printf ("You pressed escape.\n");
   }
   else if (exitType == vNORMAL)
   {
      printf ("Your message was: <%s>\n", info);
   }
   free(info);
   exit (0);
}
