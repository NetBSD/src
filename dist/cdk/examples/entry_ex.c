#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="entry_ex";
#endif

static BINDFN_PROTO(XXXCB);

/*
 * This demonstrates the Cdk entry field widget.
 */
int main (int argc GCC_UNUSED, char **argv)
{
   /* Declare local variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKENTRY *directory	= 0;
   WINDOW *cursesWin	= 0;
   char *title		= "<C>Enter a\n<C>directory name.";
   char *label		= "</U/5>Directory:<!U!5>";
   char *info, *mesg[10], temp[256+6];

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK colors. */
   initCDKColor();

   /* Create the entry field widget. */
   directory = newCDKEntry (cdkscreen, CENTER, CENTER,
				title, label, A_NORMAL, '.', vMIXED,
				40, 0, 256, TRUE, TRUE);
   bindCDKObject (vENTRY, directory, '?', XXXCB, 0);

   /* Is the widget null? */
   if (directory == 0)
   {
      /* Clean up. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a little message. */
      printf ("Oops. Can't seem to create the entry box. Is the window too small?\n");
      exit (1);
   }

   /* Draw the screen. */
   refreshCDKScreen (cdkscreen);

  /*
   * Pass in whatever was given off of the command line. Notice we
   * don't check if argv[1] is null or not. The function setCDKEntry
   * already performs any needed checks.
   */
   setCDKEntry (directory, argv[1], 0, 256, TRUE);

   /* Activate the entry field. */
   info = activateCDKEntry (directory, 0);

   /* Tell them what they typed. */
   if (directory->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No information passed back.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (directory->exitType == vNORMAL)
   {
      mesg[0] = "<C>You typed in the following";
      sprintf (temp, "<C>(%s)", info);
      mesg[1] = copyChar (temp);
      mesg[2] = "";
      mesg[3] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 4);
      freeChar (mesg[1]);
   }

   /* Clean up and exit. */
   destroyCDKEntry (directory);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}

static void XXXCB (EObjectType cdktype GCC_UNUSED, void *object GCC_UNUSED, void *clientData GCC_UNUSED, chtype key GCC_UNUSED)
{
}
