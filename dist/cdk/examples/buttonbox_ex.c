#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="buttonbox_ex";
#endif

static BINDFN_PROTO(entryCB);

/*
 * This program demonstrates the Cdk buttonbox widget.
 */
int main (void)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen		= 0;
   CDKBUTTONBOX *buttonWidget	= 0;
   CDKENTRY *entry		= 0;
   WINDOW *cursesWin		= 0;
   char *buttons[]		= {" OK ", " Cancel "};
   char *info			= 0;
   int exitType;
   int selection;

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start color. */
   initCDKColor();

   /* Create the entry widget. */
   entry = newCDKEntry (cdkscreen, CENTER, CENTER,
			"<C>Enter a name\n<R>This is an entry box!\n<L>This is an entry box!", "Name:", A_NORMAL, '.', vMIXED,
			40, 0, 256, TRUE, FALSE);

   /* Create the button box widget. */
   buttonWidget = newCDKButtonbox (cdkscreen,
			getbegx(entry->win),
			getbegy(entry->win) + entry->boxHeight - 1,
			1, entry->boxWidth,
			"<R>This is a button box!\n<L>This is a button box!", 1, 2,
			buttons, 2, A_REVERSE,
			TRUE, TRUE);

   /* Set the lower left and right characters of the box. */
   setCDKEntryLLChar (entry, ACS_LTEE);
   setCDKEntryLRChar (entry, ACS_RTEE);
   setCDKButtonboxULChar (buttonWidget, ACS_LTEE);
   setCDKButtonboxURChar (buttonWidget, ACS_RTEE);

  /*
   * Bind the Tab key in the entry field to send a
   * Tab key to the button box widget.
   */
   bindCDKObject (vENTRY, entry, KEY_TAB, entryCB, buttonWidget);

   /* Activate the entry field. */
   drawCDKButtonbox (buttonWidget, TRUE);
   activateCDKEntry (entry, 0);
   exitType = entry->exitType;
   selection = buttonWidget->currentButton;
   if (exitType == vNORMAL)
   {
      info = copyChar (entry->info);
   }

   /* Clean up. */
   destroyCDKButtonbox (buttonWidget);
   destroyCDKEntry (entry);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();

   /* Spit out some info. */
   if (exitType == vESCAPE_HIT)
   {
      printf ("You pressed escape.\n");
   }
   else if (exitType == vNORMAL)
   {
      printf ("You typed in (%s) and selected button (%s)\n", info, buttons[selection]);
   }
   free (info);
   exit (0);
}

static void entryCB (EObjectType cdktype GCC_UNUSED, void *object GCC_UNUSED, void *clientData, chtype key)
{
   CDKBUTTONBOX *buttonbox = (CDKBUTTONBOX *)clientData;

   injectCDKButtonbox (buttonbox, key);
}
