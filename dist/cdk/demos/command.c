#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="command";
#endif

/* Define some global variables. */
#define MAXHISTORY	5000
char *introductionMessage[] = {"<C></B/16>Little Command Interface", "",
				"<C>Written by Mike Glover", "",
				"<C>Type </B>help<!B> to get help."};

/* This structure is used for keeping command history. */
struct history_st {
   int count;
   int current;
   char *command[MAXHISTORY];
};

/* Define some local prototypes. */
char *uc (char *word);
void help (CDKENTRY *entry);
static BINDFN_PROTO(historyUpCB);
static BINDFN_PROTO(historyDownCB);
static BINDFN_PROTO(viewHistoryCB);
static BINDFN_PROTO(listHistoryCB);
static BINDFN_PROTO(jumpWindowCB);

/*
 * Written by:	Mike Glover
 * Purpose:
 *		This creates a very simple command interface.
 */
int main(int argc, char **argv)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen		= 0;
   CDKSWINDOW *commandOutput	= 0;
   CDKENTRY *commandEntry	= 0;
   WINDOW *cursesWin		= 0;
   chtype *convert		= 0;
   char *command		= 0;
   char *upper			= 0;
   char *prompt			= "</B/24>Command >";
   char *title			= "<C></B/5>Command Output Window";
   int promptLen		= 0;
   int commandFieldWidth	= 0;
   struct history_st history;
   char temp[600];
   int ret, junk;

   /* Set up the history. */
   history.current = 0;
   history.count = 0;

   /* Check the command line for options. */
   while (1)
   {
      /* Are there any more command line options to parse. */
      if ((ret = getopt (argc, argv, "t:p:")) == -1)
      {
	 break;
      }
      switch (ret)
      {
	 case 'p':
	      prompt = copyChar (optarg);
	      break;

	 case 't':
	      title = copyChar (optarg);
	      break;

	 default:
	      break;
      }
   }

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start color. */
   initCDKColor();

   /* Create the scrolling window. */
   commandOutput = newCDKSwindow (cdkscreen, CENTER, TOP, -4, -2,
					title, 1000, TRUE, TRUE);

   /* Convert the prompt to a chtype and determine its length. */
   convert = char2Chtype (prompt, &promptLen, &junk);
   commandFieldWidth = COLS - promptLen - 4;
   freeChtype (convert);

   /* Create the entry field. */
   commandEntry = newCDKEntry (cdkscreen, CENTER, BOTTOM,
				0, prompt, A_BOLD|COLOR_PAIR(8), COLOR_PAIR(24)|'_',
				vMIXED, commandFieldWidth, 1, 512, FALSE, FALSE);

   /* Create the key bindings. */
   bindCDKObject (vENTRY, commandEntry, KEY_UP, historyUpCB, &history);
   bindCDKObject (vENTRY, commandEntry, KEY_DOWN, historyDownCB, &history);
   bindCDKObject (vENTRY, commandEntry, TAB, viewHistoryCB, commandOutput);
   bindCDKObject (vENTRY, commandEntry, CONTROL('^'), listHistoryCB, &history);
   bindCDKObject (vENTRY, commandEntry, CONTROL('G'), jumpWindowCB, commandOutput);
   bindCDKObject (vENTRY, commandEntry, KEY_PPAGE, jumpWindowCB, commandOutput);
   bindCDKObject (vENTRY, commandEntry, KEY_NPAGE, jumpWindowCB, commandOutput);

   /* Draw the screen. */
   refreshCDKScreen (cdkscreen);

   /* Show them who wrote this and how to get help. */
   popupLabel (cdkscreen, introductionMessage, 5);
   eraseCDKEntry (commandEntry);

   /* Do this forever. */
   for (;;)
   {
      /* Get the command. */
      drawCDKEntry (commandEntry, ObjOf(commandEntry)->box);
      command	= activateCDKEntry (commandEntry, 0);

      if (commandEntry->exitType == vESCAPE_HIT)
      {
	 break;
      }

      upper	= uc (command);

      /* Check the output of the command. */
      if (strcmp (upper, "QUIT") == 0 ||
		strcmp (upper, "EXIT") == 0 ||
		strcmp (upper, "Q") == 0 ||
		strcmp (upper, "E") == 0)
      {
	 break;
      }
      else if (strcmp (command, "clear") == 0)
      {
	 /* Keep the history. */
	 history.command[history.count] = copyChar (command);
	 history.count++;
	 history.current = history.count;

	 cleanCDKSwindow (commandOutput);
	 cleanCDKEntry (commandEntry);
      }
      else if (strcmp (command, "history") == 0)
      {
	 /* Display the history list. */
	 listHistoryCB (vENTRY, commandEntry, &history, 0);

	 /* Keep the history. */
	 history.command[history.count] = copyChar (command);
	 history.count++;
	 history.current = history.count;
      }
      else if (strcmp (command, "help") == 0)
      {
	 /* Keep the history. */
	 history.command[history.count] = copyChar (command);
	 history.count++;
	 history.current = history.count;

	 /* Display the help. */
	 help (commandEntry);

	 /* Clean the entry field. */
	 cleanCDKEntry (commandEntry);
	 eraseCDKEntry (commandEntry);
      }
      else
      {
	 /* Keep the history. */
	 history.command[history.count] = copyChar (command);
	 history.count++;
	 history.current = history.count;

	 /* Jump to the bottom of the scrolling window. */
	 jumpToLineCDKSwindow (commandOutput, BOTTOM);

	 /* Insert a line providing the command. */
	 sprintf (temp, "Command: </R>%s", command);
	 addCDKSwindow (commandOutput, temp, BOTTOM);

	 /* Run the command. */
	 execCDKSwindow (commandOutput, command, BOTTOM);

	 /* Clean out the entry field. */
	 cleanCDKEntry (commandEntry);
      }

      /* Clean up a little. */
      freeChar (upper);
   }

   /* All done. */
   destroyCDKEntry (commandEntry);
   destroyCDKSwindow (commandOutput);
   delwin (cursesWin);
   freeChar (upper);
   endCDK();
   exit (0);
}

/*
 * This is the callback for the down arrow.
 */
static void historyUpCB (EObjectType cdktype GCC_UNUSED, void *object, void *clientData, chtype key GCC_UNUSED)
{
   CDKENTRY *entry = (CDKENTRY *)object;
   struct history_st *history = (struct history_st *) clientData;

   /* Make sure we don't go out of bounds. */
   if (history->current == 0)
   {
      Beep();
      return;
   }

   /* Decrement the counter. */
   history->current--;

   /* Display the command. */
   setCDKEntryValue (entry, history->command[history->current]);
   drawCDKEntry (entry, ObjOf(entry)->box);
}

/*
 * This is the callback for the down arrow.
 */
static void historyDownCB (EObjectType cdktype GCC_UNUSED, void *object, void *clientData, chtype key GCC_UNUSED)
{
   CDKENTRY *entry = (CDKENTRY *)object;
   struct history_st *history = (struct history_st *) clientData;

   /* Make sure we don't go out of bounds. */
   if (history->current == history->count)
   {
      Beep();
      return;
   }

   /* Increment the counter... */
   history->current++;

   if (history->current == history->count)
   {
      /* If we are at the end, clear the entry field. */
      cleanCDKEntry (entry);
   }
   else
   {
      /* Display the command. */
      setCDKEntryValue (entry, history->command[history->current]);
   }
   drawCDKEntry (entry, ObjOf(entry)->box);
}

/*
 * This callback allows the user to play with the scrolling window.
 */
static void viewHistoryCB (EObjectType cdktype GCC_UNUSED, void *object, void *clientData, chtype key GCC_UNUSED)
{
   CDKSWINDOW *swindow	= (CDKSWINDOW *)clientData;
   CDKENTRY *entry	= (CDKENTRY *)object;

   /* Let them play... */
   activateCDKSwindow (swindow, 0);

   /* Redraw the entry field. */
   drawCDKEntry (entry, ObjOf(entry)->box);
}

/*
 * This callback jumps to a line in the scrolling window.
 */
static void jumpWindowCB (EObjectType cdktype GCC_UNUSED, void *object, void *clientData, chtype key)
{
   CDKENTRY *entry	= (CDKENTRY *)object;
   CDKSWINDOW *swindow	= (CDKSWINDOW *)clientData;
   CDKSCALE *scale	= 0;
   int line;

   if (key == KEY_PPAGE || key == KEY_NPAGE)
   {
      injectCDKSwindow (swindow, key);
   }
   else
   {
      /* Ask them which line they want to jump to. */
      scale = newCDKScale (ScreenOf(entry), CENTER, CENTER,
			"<C>Jump To Which Line",
			"Line",
			A_NORMAL, 5,
			0, 0, swindow->itemCount, 1, 2, TRUE, FALSE);

      /* Get the line. */
      line = activateCDKScale (scale, 0);

      /* Clean up. */
      destroyCDKScale (scale);

      /* Jump to the line. */
      jumpToLineCDKSwindow (swindow, line);
   }

   /* Redraw the widgets. */
   drawCDKEntry (entry, ObjOf(entry)->box);
}

/*
 * This callback allows the user to pick from the history list from a
 * scrolling list.
 */
static void listHistoryCB (EObjectType cdktype GCC_UNUSED, void *object, void *clientData, chtype key GCC_UNUSED)
{
   CDKENTRY *entry = (CDKENTRY *)object;
   struct history_st *history = (struct history_st *) clientData;
   CDKSCROLL *scrollList;
   int height = (history->count < 10 ? history->count+3 : 13);
   int selection;

   /* No history, no list. */
   if (history->count == 0)
   {
      /* Popup a little window telling the user there are no commands. */
      char *mesg[] = {"<C></B/16>No Commands Entered", "<C>No History"};
      popupLabel (ScreenOf(entry), mesg, 2);
   }
   else
   {
      /* Create the scrolling list of previous commands. */
      scrollList = newCDKScroll (ScreenOf(entry), CENTER, CENTER, RIGHT,
				height, 20, "<C></B/29>Command History",
				history->command, history->count,
				NUMBERS, A_REVERSE, TRUE, FALSE);

      /* Get the command to execute. */
      selection = activateCDKScroll (scrollList, 0);
      destroyCDKScroll (scrollList);

      /* Check the results of the selection. */
      if (selection >= 0)
      {
	 /* Get the command and stick it back in the entry field. */
	 setCDKEntryValue (entry, history->command[selection]);
      }
   }

   /* Redraw the screen. */
   eraseCDKEntry (entry);
   drawCDKScreen (ScreenOf(entry));
}

/*
 * This function displays help.
 */
void help (CDKENTRY *entry)
{
   char *mesg[25];

   /* Create the help message. */
   mesg[0] = "<C></B/29>Help";
   mesg[1] = "";
   mesg[2] = "</B/24>When in the command line.";
   mesg[3] = "<B=history   > Displays the command history.";
   mesg[4] = "<B=Ctrl-^    > Displays the command history.";
   mesg[5] = "<B=Up Arrow  > Scrolls back one command.";
   mesg[6] = "<B=Down Arrow> Scrolls forward one command.";
   mesg[7] = "<B=Tab       > Activates the scrolling window.";
   mesg[8] = "<B=help      > Displays this help window.";
   mesg[9] = "";
   mesg[10] = "</B/24>When in the scrolling window.";
   mesg[11] = "<B=l or L    > Loads a file into the window.";
   mesg[12] = "<B=s or S    > Saves the contents of the window to a file.";
   mesg[13] = "<B=Up Arrow  > Scrolls up one line.";
   mesg[14] = "<B=Down Arrow> Scrolls down one line.";
   mesg[15] = "<B=Page Up   > Scrolls back one page.";
   mesg[16] = "<B=Page Down > Scrolls forward one page.";
   mesg[17] = "<B=Tab/Escape> Returns to the command line.";
   mesg[18] = "";
   mesg[19] = "<C> (</B/24>Refer to the scrolling window online manual for more help<!B!24>.)";
   popupLabel (ScreenOf(entry), mesg, 20);
}

/*
 * This converts a word to upper case.
 */
char *uc (char *word)
{
   char *upper	= 0;
   int length	= 0;
   int x;

   /* Make sure the word is not null. */
   if (word == 0)
   {
      return 0;
   }
   length = strlen (word);

   /* Get the memory for the new word. */
   upper = (char *)malloc (sizeof (char *) * (length+2));
   if (upper == 0)
   {
      return (word);
   }

   /* Start converting the case. */
   for (x=0; x < length; x++)
   {
      if (isalpha ((int)word[x]))
      {
	 upper[x] = toupper(word[x]);
      }
      else
      {
	 upper[x] = word[x];
      }
   }
   upper[length] = '\0';
   return upper;
}
