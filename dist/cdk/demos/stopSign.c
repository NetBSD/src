#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="sillyness_ex";
#endif

int main(void)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKLABEL *stopSign	= 0;
   CDKLABEL *title	= 0;
   WINDOW *cursesWin	= 0;
   int currentLight	= 0;
   char *mesg[5], *sign[4];
   chtype key;

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors. */
   initCDKColor();

   /* Set the labels up. */
   mesg[0] = "<C><#HL(40)>";
   mesg[1] = "<C>Press </B/16>r<!B!16> for the </B/16>red light";
   mesg[2] = "<C>Press </B/32>y<!B!32> for the </B/32>yellow light";
   mesg[3] = "<C>Press </B/24>g<!B!24> for the </B/24>green light";
   mesg[4] = "<C><#HL(40)>";
   sign[0] = " <#DI> ";
   sign[1] = " <#DI> ";
   sign[2] = " <#DI> ";

   /* Declare the labels. */
   title = newCDKLabel (cdkscreen, CENTER, TOP, mesg, 5, FALSE, FALSE);
   stopSign = newCDKLabel (cdkscreen, CENTER, CENTER, sign, 3, TRUE, TRUE);

   /* Do this until they hit q or escape. */
   while (1)
   {
      /* Draw the labels. */
      drawCDKLabel (title, FALSE);
      drawCDKLabel (stopSign, TRUE);

      key = waitCDKLabel (stopSign, 0);
      if (key == KEY_ESC || key == 'q' || key == 'Q')
      {
	 break;
      }
      else if (key == 'r' || key == 'R')
      {
	 sign[0] = "</B/16> <#DI> ";
	 sign[1] = " <#DI> ";
	 sign[2] = " <#DI> ";
	 currentLight = 0;
      }
      else if (key == 'y' || key == 'Y')
      {
	 sign[0] = " <#DI> ";
	 sign[1] = "</B/32> <#DI> ";
	 sign[2] = " <#DI> ";
	 currentLight = 1;
      }
      else if (key == 'g' || key == 'G')
      {
	 sign[0] = " <#DI> ";
	 sign[1] = " <#DI> ";
	 sign[2] = "</B/24> <#DI> ";
	 currentLight = 2;
      }

      /* Set the contents of the label and re-draw it. */
      setCDKLabel (stopSign, sign, 3, TRUE);
   }

   /* Clean up. */
   destroyCDKLabel (title);
   destroyCDKLabel (stopSign);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
