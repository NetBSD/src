#include <cdk.h>

/*
 * This demonstrates how to create four different Cdk
 * screens and flip between them.
 */

#ifdef HAVE_XCURSES
char *XCursesProgramName="cdkscreen_ex";
#endif

int main (void)
{
   /* Declare vars. */
   WINDOW	*cursesWindow;
   CDKSCREEN	*cdkScreen1, *cdkScreen2;
   CDKSCREEN	*cdkScreen3, *cdkScreen4;
   CDKSCREEN	*cdkScreen5;
   CDKLABEL	*label1, *label2;
   CDKLABEL	*label3, *label4;
   CDKDIALOG	*dialog;
   char *title1Mesg[4], *title2Mesg[4];
   char *title3Mesg[4], *title4Mesg[4];
   char *dialogMesg[10];
   char *buttons[] = {"Continue", "Exit"};
   int answer;

   /* Create the curses window. */
   cursesWindow = initscr();

   /* Create the two screens. */
   cdkScreen1 = initCDKScreen (cursesWindow);
   cdkScreen2 = initCDKScreen (cursesWindow);
   cdkScreen3 = initCDKScreen (cursesWindow);
   cdkScreen4 = initCDKScreen (cursesWindow);
   cdkScreen5 = initCDKScreen (cursesWindow);

   /* Create the first screen. */
   title1Mesg[0] = "<C><#HL(30)>";
   title1Mesg[1] = "<C></R>This is the first screen.";
   title1Mesg[2] = "<C>Hit space to go to the next screen";
   title1Mesg[3] = "<C><#HL(30)>";
   label1 = newCDKLabel (cdkScreen1, CENTER, TOP, title1Mesg, 4, FALSE, FALSE);

   /* Create the second screen. */
   title2Mesg[0] = "<C><#HL(30)>";
   title2Mesg[1] = "<C></R>This is the second screen.";
   title2Mesg[2] = "<C>Hit space to go to the next screen";
   title2Mesg[3] = "<C><#HL(30)>";
   label2 = newCDKLabel (cdkScreen2, RIGHT, CENTER, title2Mesg, 4, FALSE, FALSE);

   /* Create the third screen. */
   title3Mesg[0] = "<C><#HL(30)>";
   title3Mesg[1] = "<C></R>This is the third screen.";
   title3Mesg[2] = "<C>Hit space to go to the next screen";
   title3Mesg[3] = "<C><#HL(30)>";
   label3 = newCDKLabel (cdkScreen3, CENTER, BOTTOM, title3Mesg, 4, FALSE, FALSE);

   /* Create the fourth screen. */
   title4Mesg[0] = "<C><#HL(30)>";
   title4Mesg[1] = "<C></R>This is the fourth screen.";
   title4Mesg[2] = "<C>Hit space to go to the next screen";
   title4Mesg[3] = "<C><#HL(30)>";
   label4 = newCDKLabel (cdkScreen4, LEFT, CENTER, title4Mesg, 4, FALSE, FALSE);

   /* Create the fifth screen. */
   dialogMesg[0] = "<C><#HL(30)>";
   dialogMesg[1] = "<C>Screen 5";
   dialogMesg[2] = "<C>This is the last of 5 screens. If you want";
   dialogMesg[3] = "<C>to continue press the 'Continue' button.";
   dialogMesg[4] = "<C>Otherwise press the 'Exit' button";
   dialogMesg[5] = "<C><#HL(30)>";
   dialog = newCDKDialog (cdkScreen5, CENTER, CENTER,
				dialogMesg, 6, buttons, 2,
				A_REVERSE, TRUE, TRUE, TRUE);

   /* Do this for ever... (almost) */
   for (;;)
   {
      /* Draw the first screen. */
      drawCDKScreen (cdkScreen1);
      waitCDKLabel (label1, ' ');
      eraseCDKScreen (cdkScreen1);

      /* Draw the second screen. */
      drawCDKScreen (cdkScreen2);
      waitCDKLabel (label2, ' ');
      eraseCDKScreen (cdkScreen2);

      /* Draw the third screen. */
      drawCDKScreen (cdkScreen3);
      waitCDKLabel (label3, ' ');
      eraseCDKScreen (cdkScreen3);

      /* Draw the fourth screen. */
      drawCDKScreen (cdkScreen4);
      waitCDKLabel (label4, ' ');
      eraseCDKScreen (cdkScreen4);

      /* Draw the fourth screen. */
      drawCDKScreen (cdkScreen5);
      answer = activateCDKDialog (dialog, 0);
      eraseCDKScreen (cdkScreen5);

      /* Check the users answer. */
      if (answer == 1)
      {
	 destroyCDKLabel (label1);
	 destroyCDKLabel (label2);
	 destroyCDKLabel (label3);
	 destroyCDKLabel (label4);
	 destroyCDKDialog (dialog);
	 destroyCDKScreen (cdkScreen1);
	 destroyCDKScreen (cdkScreen2);
	 destroyCDKScreen (cdkScreen3);
	 destroyCDKScreen (cdkScreen4);
	 destroyCDKScreen (cdkScreen5);
	 endCDK();
	 exit (0);
      }
   }
}
