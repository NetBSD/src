#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="alphalist_ex";
#endif

/*
 * This program demonstrates the Cdk alphalist widget.
 */
#define MAXINFOLINES	10000

int getUserList (char **list, int maxItems);

int main(void)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen		= 0;
   CDKALPHALIST *alphaList	= 0;
   WINDOW *cursesWin		= 0;
   char *title			= "<C></B/24>Alpha List\n<C>Title";
   char *label			= "</B>Account: ";
   char *word			= 0;
   char *info[MAXINFOLINES], *mesg[5], temp[256];
   int count;

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start color. */
   initCDKColor();

   /* Get the user list. */
   count = getUserList (info, MAXINFOLINES);

   /* Create the alpha list widget. */
   alphaList = newCDKAlphalist (cdkscreen, CENTER, CENTER,
				-10, -2, title, label,
				info, count,
				'_', A_REVERSE, TRUE, TRUE);

   /* Let them play with the alpha list. */
   word = activateCDKAlphalist (alphaList, 0);

   /* Determine what the user did. */
   if (alphaList->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No word was selected.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (alphaList->exitType == vNORMAL)
   {
      mesg[0] = "<C>You selected the following";
      sprintf (temp, "<C>(%s)", word);
      mesg[1] = copyChar (temp);
      mesg[2] = "";
      mesg[3] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 4);
   }

   /* Clean up. */
   destroyCDKAlphalist (alphaList);
   delwin (cursesWin);
   endCDK();
   exit (0);
}

/*
 * This reads the passwd file and retrieves user information.
 */
int getUserList (char **list, int maxItems)
{
   struct passwd *ent;
   int x = 0;

   while ( (ent = getpwent ()) != 0)
   {
      if (x+1 >= maxItems)
	 break;
      list[x++] = copyChar (ent->pw_name);
   }
   return x;
}
