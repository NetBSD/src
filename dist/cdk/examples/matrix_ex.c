#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="matrix_ex";
#endif

/*
 * This program demonstrates the Cdk matrix widget.
 */
int main (void)
{
   /* Declare local variables. */
   CDKSCREEN *cdkscreen		= 0;
   CDKMATRIX *courseList	= 0;
   WINDOW *cursesWin		= 0;
   char *title			= 0;
   int rows			= 8;
   int cols			= 5;
   int vrows			= 3;
   int vcols			= 5;
   char *coltitle[10], *rowtitle[10], *mesg[100];
   int colwidth[10], colvalue[10];

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors. */
   initCDKColor();

   /* Create the horizontal and vertical matrix labels. */
   coltitle[1] = "</B/5>Course";   colwidth[1] = 7 ; colvalue[1] = vUMIXED;
   coltitle[2] = "</B/33>Lec 1";   colwidth[2] = 7 ; colvalue[2] = vUMIXED;
   coltitle[3] = "</B/33>Lec 2";   colwidth[3] = 7 ; colvalue[3] = vUMIXED;
   coltitle[4] = "</B/33>Lec 3";   colwidth[4] = 7 ; colvalue[4] = vUMIXED;
   coltitle[5] = "</B/7>Flag";	   colwidth[5] = 1 ; colvalue[5] = vUMIXED;
   rowtitle[1] = "</B/6>Course 1"; rowtitle[2] = "<C></B/6>Course 2";
   rowtitle[3] = "</B/6>Course 3"; rowtitle[4] = "<L></B/6>Course 4";
   rowtitle[5] = "</B/6>Course 5"; rowtitle[6] = "<R></B/6>Course 6";
   rowtitle[7] = "</B/6>Course 7"; rowtitle[8] = "<R></B/6>Course 8";

   /* Create the title. */
   title = "<C>This is the CDK\n<C>matrix widget.\n<C><#LT><#HL(30)><#RT>";

   /* Create the matrix object. */
   courseList = newCDKMatrix (cdkscreen,
				CENTER, CENTER,
				rows, cols, vrows, vcols,
				title, rowtitle, coltitle,
				colwidth, colvalue,
				-1, -1, '.',
				COL, TRUE, TRUE, TRUE);

   /* Check to see if the matrix is null. */
   if (courseList == 0)
   {
      /* Clean up. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a little message. */
      printf ("Oops. Can't seem to create the matrix widget. Is the window too small ?\n");
      exit (1);
   }

   /* Activate the matrix. */
   activateCDKMatrix (courseList, 0);

   /* Check if the user hit escape or not. */
   if (courseList->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No information passed back.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (courseList->exitType == vNORMAL)
   {
      mesg[0] = "<C>You exited the matrix normally.";
      mesg[1] = "<C>To get the contents of the matrix cell, you can";
      mesg[2] = "<C>dereference the info array off of the matrix widget.";
      mesg[3] = "";
      mesg[4] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 5);
   }

   /* Clean up. */
   destroyCDKMatrix (courseList);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
