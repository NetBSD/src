#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="preprocess_ex";
#endif

int entryPreProcessCB (EObjectType cdkType, void *object, void *clientData, chtype input);

/*
 * This demonstrates the Cdk entry field widget.
 */
int main (void)
{
   /* Declare local variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKENTRY *widget	= 0;
   WINDOW *cursesWin	= 0;
   char *title		= "<C>Type in anything you want\n<C>but the dreaded letter </B>G<!B>!";
   char *info, *mesg[10], temp[256];

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK colors. */
   initCDKColor();

   /* Create the entry field widget. */
   widget = newCDKEntry (cdkscreen, CENTER, CENTER,
				title, 0, A_NORMAL, '.', vMIXED,
				40, 0, 256, TRUE, TRUE);

   /* Is the widget null? */
   if (widget == 0)
   {
      /* Clean up. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a little message. */
      printf ("Oops. Can't seem to create the entry box. Is the window too small?\n");
      exit (1);
   }

   setCDKEntryPreProcess (widget, entryPreProcessCB, 0);

   /* Activate the entry field. */
   info = activateCDKEntry (widget, 0);

   /* Tell them what they typed. */
   if (widget->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No information passed back.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (widget->exitType == vNORMAL)
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
   destroyCDKEntry (widget);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}

int entryPreProcessCB (EObjectType cdkType GCC_UNUSED, void *object, void *clientData GCC_UNUSED, chtype input)
{
   CDKENTRY *entry	= (CDKENTRY *)object;
   CDKDIALOG *widget	= 0;
   char *buttons[]	= {"OK"};
   int buttonCount	= 1;
   int lines		= 0;
   char *mesg[5];

   /* Check the input. */
   if ((input == 'g') || (input == 'G'))
   {
      mesg[lines++] = "<C><#HL(30)>";
      mesg[lines++] = "<C>I told you </B>NOT<!B> to type G";
      mesg[lines++] = "<C><#HL(30)>";

      widget = newCDKDialog (ScreenOf(entry), CENTER, CENTER,
				mesg, lines, buttons, buttonCount,
				A_REVERSE, FALSE, FALSE, FALSE);
      activateCDKDialog (widget, 0);
      destroyCDKDialog (widget);
      drawCDKEntry (entry, ObjOf(entry)->box);
      return 0;
   }
   return 1;
}
