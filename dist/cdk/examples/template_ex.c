#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="template_ex";
#endif

/*
 * This program demonstrates the Cdk template widget.
 */
int main (void)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen		= 0;
   CDKTEMPLATE *phoneNumber	= 0;
   WINDOW *cursesWin		= 0;
   char *title			= "<C>Title\n<R>Title\n<L>Title";
   char *label			= "</5>Phone Number:<!5> ";
   char *Overlay		= "</B/6>(___)<!6> </5>___-____";
   char *plate			= "(###) ###-####";
   char *info, *mixed, temp[256], *mesg[5];

   /* Set up CDK*/
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors. */
   initCDKColor();

   /* Declare the template. */
   phoneNumber = newCDKTemplate (cdkscreen, CENTER, CENTER,
					title, label,
					plate, Overlay,
					TRUE, TRUE);

   /* Is the template pointer null? */
   if (phoneNumber == 0)
   {
      /* Exit CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message and exit. */
      printf ("Oops. Can;'t seem to create template. Is the window too small?");
      exit (1);
   }

   /* Activate the template. */
   info = activateCDKTemplate (phoneNumber, 0);

   /* Tell them what they typed. */
   if (phoneNumber->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No information typed in.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (phoneNumber->exitType == vNORMAL)
   {
      /* Mix the plate and the number. */
      mixed = mixCDKTemplate (phoneNumber);

      /* Create the message to display.				*/
      sprintf (temp, "Phone Number with out plate mixing  : %s", info);
      mesg[0] = copyChar (temp);
      sprintf (temp, "Phone Number with the plate mixed in: %s", mixed);
      mesg[1] = copyChar (temp);
      mesg[2] = "";
      mesg[3] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 4);
      freeChar (mesg[1]);
   }

   /* Clean up. */
   destroyCDKTemplate (phoneNumber);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
