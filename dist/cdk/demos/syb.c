#include <cdk.h>
#include <sybfront.h>
#include <sybdb.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="syb";
#endif

#define MAXWIDTH	5000
#define MAXHISTORY	1000

/*
 * This structure is used for keeping command history.
 */
struct history_st {
   int count;
   int current;
   char *command[MAXHISTORY];
};

/*
 * Define some global variables.
 */
char *GPUsage		= "[-p Command Prompt] [-U User] [-P Password] [-S Server] [-h help]";
char *GPCurrentDatabase = 0;
extern char *dberrstr;

/*
 * Because the error/message callback do not allow you to pass in
 * data of your own, we have to make the screen pointer global. :(
 */
CDKSCREEN *GPCdkScreen = 0;

/*
 * Define function prototypes.
 */
DBPROCESS *loginToSybase (CDKSCREEN *screen, char *login, char *password);
DBPROCESS *sybaseLogin (CDKSCREEN *screen, char *login, char *password, int attempts);
char *assembleTitle (DBPROCESS *dbProcess);
char *assembleRow (DBPROCESS *dbProcess);
char *uc (char *word);
int getColWidth (DBPROCESS *dbProcess, int col);
void runIsqlCommand (CDKSCREEN *cdkscreen, CDKSWINDOW *swindow, DBPROCESS *process);
void useDatabase (CDKSWINDOW *swindow, DBPROCESS *dbProc, char *command);
void intro (CDKSCREEN *screen);
void help (CDKENTRY *entry);
void loadHistory (struct history_st *history);
void saveHistory (struct history_st *history, int count);

/*
 * Define callback prototypes.
 */
void viewHistoryCB (EObjectType cdktype, void *object, void *clientData, chtype key);
void swindowHelpCB (EObjectType cdktype, void *object, void *clientData, chtype key);
void historyUpCB   (EObjectType cdktype, void *object, void *clientData, chtype key);
void historyDownCB (EObjectType cdktype, void *object, void *clientData, chtype key);
void listHistoryCB (EObjectType cdktype, void *object, void *clientData, chtype key);

/*
 * Define Sybase error/message callbacks. This is required by DBLib.
 */
int err_handler (DBPROCESS *dbProcess, DBINT mesgNumber, int mesgState,
			int severity, char *mesgText);
int msg_handler (DBPROCESS *dbProcess, DBINT mesgNumber, int mesgState,
			int severity, char *mesgText);

/*
 * Written by:	Mike Glover
 * Purpose:
 *		This creates a very simple interface to Sybase.
 */
int main (int argc, char **argv)
{
   /* Declare variables. */
   CDKSWINDOW *commandOutput	= 0;
   CDKENTRY *commandEntry	= 0;
   DBPROCESS *dbProcess		= 0;
   WINDOW *cursesWin		= 0;
   char *dsquery		= 0;
   char *command		= 0;
   char *prompt			= 0;
   char *upper			= 0;
   char *login			= 0;
   char *password		= 0;
   char *server			= 0;
   int count			= 0;
   int width			= 0;
   int ret			= 0;
   struct history_st history;
   char *mesg[5], temp[1000];

   /* Set up the history. */
   GPCurrentDatabase	= strdup ("master");
   history.current	= 0;
   history.count	= 0;

   /* Check the command line for options. */
   while (1)
   {
      /* Are there any more command line options to parse. */
      if ((ret = getopt (argc, argv, "p:U:P:S:h")) == -1)
      {
	 break;
      }

      switch (ret)
      {
	 case 'p':
	      prompt = copyChar (optarg);
	      break;

	 case 'U':
	      login = copyChar (optarg);
	      break;

	 case 'P':
	      password = copyChar (optarg);
	      break;

	 case 'S':
	      server = copyChar (optarg);
	      break;

	 case 'h':
	      printf ("Usage: %s %s\n", argv[0], GPUsage);
	      exit (0);
	      break;
      }
   }

   /* Set up the command prompt. */
   if (prompt == 0)
   {
      if (server == 0)
      {
	 if ((dsquery = getenv ("DSQUERY")) != 0)
	 {
	    sprintf (temp, "</B/24>[%s] Command >", dsquery);
	    prompt = copyChar (temp);
	 }
	 else
	 {
	    prompt = copyChar ("</B/24>Command >");
	 }
      }
      else
      {
	 sprintf (temp, "</B/24>[%s] Command >", server);
	 prompt = copyChar (temp);
      }
   }

   /* Set up CDK. */
   cursesWin = initscr();
   GPCdkScreen = initCDKScreen (cursesWin);

   /* Start color. */
   initCDKColor();

   /* Initialize DB-Library. */
   if (dbinit() == FAIL)
   {
      mesg[0] = "<C></U>Fatal Error";
      mesg[1] = "<C>Could not connect to the Sybase database.";
      popupLabel (GPCdkScreen, mesg, 2);
      exit (-1);
   }

   /* Load the history. */
   loadHistory (&history);

   /* Create the scrolling window. */
   commandOutput = newCDKSwindow (GPCdkScreen, CENTER, TOP, -8, -2,
					"<C></B/5>Command Output Window",
					MAXWIDTH, TRUE, FALSE);

   /* Create the entry field. */
   width	= COLS - strlen (prompt) - 1;
   commandEntry = newCDKEntry (GPCdkScreen, CENTER, BOTTOM,
				0, prompt, A_BOLD|COLOR_PAIR(8),
				COLOR_PAIR(24)|'_', vMIXED,
				width, 1, 512, FALSE, FALSE);

   /* Create the key bindings. */
   bindCDKObject (vENTRY, commandEntry, KEY_UP, &historyUpCB, &history);
   bindCDKObject (vENTRY, commandEntry, KEY_DOWN, &historyDownCB, &history);
   bindCDKObject (vENTRY, commandEntry, CONTROL('^'), &listHistoryCB, &history);
   bindCDKObject (vENTRY, commandEntry, TAB, &viewHistoryCB, commandOutput);
   bindCDKObject (vSWINDOW, commandOutput, '?', swindowHelpCB, commandEntry);

   /* Draw the screen. */
   refreshCDKScreen (GPCdkScreen);

   /* Display the introduction window. */
   intro (GPCdkScreen);

   /* Make them login first. */
   dbProcess = sybaseLogin (GPCdkScreen, login, password, 3);
   if (dbProcess == 0)
   {
      destroyCDKScreen (GPCdkScreen);
      endCDK ();
      exit (-1);
   }

   /* Do this forever. */
   for (;;)
   {
      /* Get the command. */
      command = activateCDKEntry (commandEntry, 0);

      /* Strip off leading and trailing white space. */
      stripWhiteSpace (vBOTH, command);

      /* Upper case the command. */
      upper = uc (command);

      /* Check the output of the command. */
      if (strcmp (upper, "QUIT") == 0 ||
		strcmp (upper, "EXIT") == 0 ||
		strcmp (upper, "Q") == 0 ||
		strcmp (upper, "E") == 0 ||
		commandEntry->exitType == vESCAPE_HIT)
      {
	 /* Save the history. */
	 saveHistory (&history, 100);

	 /* Exit. */
	 dbclose (dbProcess);
	 dbexit();

	 /* All done. */
	 destroyCDKEntry (commandEntry);
	 destroyCDKSwindow (commandOutput);
	 delwin (cursesWin);
	 freeChar (upper);
	 endCDK();
	 exit (0);
      }
      else if (strcmp (command, "login") == 0)
      {
	 DBPROCESS *newLogin = sybaseLogin (GPCdkScreen, 0, 0, 3);
	 if (newLogin == 0)
	 {
	    addCDKSwindow (commandOutput, "Login Error: Could not switch to new user.", BOTTOM);
	 }
	 else
	 {
	    /* Close the old connection. */
	    dbclose (dbProcess);
	    dbProcess = newLogin;

	    /* Add a message to the scrolling window. */
	    addCDKSwindow (commandOutput,
				"Logged into database as new user.",
				BOTTOM);
	    count = 0;
	 }
      }
      else if (strcmp (command, "logout") == 0)
      {
	 /* Close the old connection. */
	 dbclose (dbProcess);
	 dbProcess = 0;

	 /* Add a message to the scrolling window. */
	 addCDKSwindow (commandOutput, "Logged out.", BOTTOM);
	 count = 0;
      }
      else if (strcmp (command, "clear") == 0)
      {
	 /* Clear the scrolling window. */
	 cleanCDKSwindow (commandOutput);
      }
      else if (strcmp (command, "history") == 0)
      {
	 listHistoryCB (vENTRY, (void *)commandEntry, (void *)&history, 0);
      }
      else if (strcmp (command, "tables") == 0)
      {
	 /* Check if we are logged in. */
	 if (dbProcess == 0)
	 {
	    addCDKSwindow (commandOutput, "You must login first.", BOTTOM);
	 }
	 else
	 {
	    sprintf (command, "select * from sysobjects where type = 'U'");

	    /* Put the command into the ISQL buffer. */
	    dbcmd (dbProcess, command);

	    /* Put the command into the scrolling window. */
	    sprintf (temp, "</R>%d><!R> %s", count+1, command);
	    addCDKSwindow (commandOutput, temp, BOTTOM);

	    /* Increment the counter. */
	    count++;
	 }
      }
      else if (strcmp (command, "help") == 0)
      {
	 /* Display the help. */
	 help(commandEntry);
      }
      else if (command[0] == 'u' && command[1] == 's' &&
		command[2] == 'e' && command[3] == ' ')
      {
	 /* They want to use a database. */
	 useDatabase (commandOutput, dbProcess, command);
	 count = 0;
      }
      else if (strcmp (command, "go") == 0)
      {
	 /* Check if we are logged in. */
	 if (dbProcess == 0)
	 {
	    addCDKSwindow (commandOutput, "You must login first.", BOTTOM);
	 }
	 else
	 {
	    /* Put the command into the scrolling window. */
	    sprintf (temp, "</R>%d><!R> %s", count+1, command);
	    addCDKSwindow (commandOutput, temp, BOTTOM);
	    count = 0;

	    /* Run the command. */
	    runIsqlCommand (GPCdkScreen, commandOutput, dbProcess);
	 }
      }
      else
      {
	 /* Check if we are logged in. */
	 if (dbProcess == 0)
	 {
	    addCDKSwindow (commandOutput, "You must login first.", BOTTOM);
	 }
	 else
	 {
	    /* Put the command into the ISQL buffer. */
	    dbcmd (dbProcess, command);

	    /* Put the command into the scrolling window. */
	    sprintf (temp, "</R>%d><!R> %s", count+1, command);
	    addCDKSwindow (commandOutput, temp, BOTTOM);

	    /* Increment the counter. */
	    count++;
	 }
      }

      /* Keep the history. */
      history.command[history.count] = copyChar (command);
      history.count++;
      history.current = history.count;

      /* Clear the entry field. */
      cleanCDKEntry (commandEntry);

      /* Free up the memory used by the upper pointer. */
      freeChar (upper);
   }
}

/*
 * This lets a person 'use' a database.
 */
void useDatabase (CDKSWINDOW *swindow, DBPROCESS *dbProc, char *command)
{
   char *database = 0;
   char temp[256];
   char **words;
   int wordCount, x;

   /* Split the command line up and get the database name. */
   words = CDKsplitString (command, ' ');
   wordCount = CDKcountStrings (words);

   /* Look for the name. */
   for (x=1; x < wordCount; x++)
   {
      if (strlen (words[x]) != 0)
      {
	 database = copyChar (words[x]);
      }
   }
   CDKfreeStrings(words);

   /* Try to actually use the database. */
   if (dbuse(dbProc, database) == FAIL)
   {
      /* We aren't allowed to use that database. */
      sprintf (temp, "Command: %s", command);
      addCDKSwindow (swindow, temp, BOTTOM);
      addCDKSwindow (swindow, "</B/16>Error<!B!16> You are not allowed to use that database.", BOTTOM);
      return;
   }

   /* Set the global database name. */
   if (database == 0)
   {
      /* Put a syntax error in the scrolling window. */
      sprintf (temp, "Command: %s", command);
      addCDKSwindow (swindow, temp, BOTTOM);
      addCDKSwindow (swindow, "</B/16>Error<!B!16> Syntax Error", BOTTOM);
      return;
   }

   /* Clear out the old database name and set the new one. */
   freeChar (GPCurrentDatabase);
   GPCurrentDatabase = database;

   /* Add a message into the scrolling window. */
   sprintf (temp, "Command: %s", command);
   addCDKSwindow (swindow, temp, BOTTOM);
   sprintf (temp, "Default Database set to %s", GPCurrentDatabase);
   addCDKSwindow (swindow, temp, BOTTOM);
}

/*
 * This does the requisite checking for failed login attempts.
 */
DBPROCESS *sybaseLogin (CDKSCREEN *screen, char *accountName, char *accountPassword, int attemptCount)
{
   DBPROCESS *dbProcess = 0;
   char *login		= accountName;
   char *password	= accountPassword;
   int count		= 0;
   int lines		= 0;
   char *mesg[5];

   /* Give them X attempts, then kick them out. */
   while (count < attemptCount)
   {
      /* Try to login. */
      dbProcess = loginToSybase (GPCdkScreen, login, password);

      /*
       * If the dbprocess is null the account/password
       * pair does not exist.
       */
       if (dbProcess == 0)
       {
	  /*
	   * If the login and account names were provided,
	   * set them to null and allow the user to enter
	   * the name and password by hand.
	   */
	   login = 0;
	   password = 0;

	  /* Spit out the login error message. */
	  lines = 0;
	  mesg[lines++] = "<C></B/5>Login Error";
	  mesg[lines++] = " ";
	  mesg[lines++] = "<C>The login/password pair does not exist.";
	  mesg[lines++] = " ";
	  mesg[lines++] = "<C>Please try again.";

	  popupLabel (GPCdkScreen, mesg, lines);

	  eraseCDKScreen (GPCdkScreen);
	  refreshCDKScreen (GPCdkScreen);
	  count++;
       }
       else
       {
	  break;
       }
   }

   /* Did we expire the login attempts? */
   if (count > attemptCount-1)
   {
      lines = 0;
      mesg[lines++] = "<C>Login Error";
      mesg[lines++] = " ";
      mesg[lines++] = "<C>Too many attempyts to login.";
      mesg[lines++] = "<C>Exiting.";

      popupLabel (GPCdkScreen, mesg, lines);

      return 0;
   }
   return dbProcess;
}

/*
 * Let the user login.
 */
DBPROCESS *loginToSybase (CDKSCREEN *screen, char *accountName, char *accountPassword)
{
   CDKENTRY *loginEntry		= 0;
   CDKENTRY *passwordEntry	= 0;
   LOGINREC *dbLogin		= 0;
   char *hostAccount		= 0;
   char *login			= accountName;
   char *password		= accountPassword;
   char *mesg[10], temp[256];

   /* Draw the screen. */
   refreshCDKScreen (screen);

   /* Define the login entry field. */
   if (login == 0)
   {
      loginEntry = newCDKEntry (screen, CENTER, CENTER,
				"\n<C></B/5>Sybase Login\n",
				"Account Name: ",
				A_BOLD|COLOR_PAIR(8),
				COLOR_PAIR(24)|'_', vMIXED,
				20, 1, 20, TRUE, FALSE);

      /* Use the current account name as the default answer. */
      hostAccount = getlogin();
      setCDKEntryValue (loginEntry, hostAccount);

      /* Get the login. */
      while (1)
      {
	 /* Redraw the screen. */
	 eraseCDKScreen (loginEntry->screen);
	 refreshCDKScreen (loginEntry->screen);

	 /* Get the login to the sybase account. */
	 login = copyChar (activateCDKEntry (loginEntry, 0));

	 /* Check if they hit escape. */
	 if (loginEntry->exitType == vESCAPE_HIT)
	 {
	    mesg[0] = "<C></U>Error";
	    mesg[1] = "A user name must be provided.";
	    popupLabel (screen, mesg, 2);
	 }
	 else
	 {
	    break;
	 }
      }

      /* Destroy the widget. */
      destroyCDKEntry (loginEntry);
   }

   /* Get the password if we need too. */
   if (password == 0)
   {
      sprintf (temp, "\n<C></B/5>%s's Password\n", login);
      passwordEntry = newCDKEntry (screen, CENTER, CENTER,
				temp, "Account Password: ",
				A_BOLD|COLOR_PAIR(8),
				COLOR_PAIR(24)|'_', vHMIXED,
				20, 0, 20, TRUE, FALSE);
      setCDKEntryHiddenChar (passwordEntry, '*');

      /* Get the password. (the account may not have a password.) */
      password = copyChar (activateCDKEntry (passwordEntry, 0));
      if ((passwordEntry->exitType == vESCAPE_HIT) ||
	((int)strlen(password) == 0))
      {
	 password = "";
      }

      /* Destroy the widget. */
      destroyCDKEntry (passwordEntry);
   }

   /*
    * Try to connect to the database and get a LOGINREC structre.
    */
   if ((dbLogin = dblogin()) == 0)
   {
      mesg[0] = "<C></U>Fatal Error";
      mesg[1] = "<C>Could not connect to the Sybase database.";
      popupLabel (screen, mesg, 2);
      refreshCDKScreen (screen);
      exit (-1);
   }

  /*
   * Set the login and password and try to login to the database.
   */
   DBSETLUSER (dbLogin, login);
   DBSETLPWD (dbLogin, password);
   DBSETLAPP (dbLogin, "cdk_syb");

   /* Create a dbprocess structure to communicate with the database. */
   return dbopen (dbLogin, 0);
}

/*
 * This actually runs the command.
 */
void runIsqlCommand (CDKSCREEN *screen, CDKSWINDOW *swindow, DBPROCESS *dbProcess)
{
   /* Declare local variables. */
   RETCODE returnCode;
   int rowCount;

   /* Add in a output seperation line. */
   addCDKSwindow (swindow, "<C><#HL(5)> Start of Output <#HL(5)>", BOTTOM);

   /* Run the command. */
   dbsqlexec (dbProcess);

   /* Check the return code of the commands. */
   while ((returnCode = dbresults (dbProcess)) != NO_MORE_RESULTS)
   {
      if (returnCode == FAIL)
      {
	 /* Oops, the command bombed. */
	 addCDKSwindow (swindow, "</5/16>Command failed.", BOTTOM);
      }
      else
      {
	 if (!(DBCMDROW (dbProcess)))
	 {
	    /* The command could not return any rows. */
	    addCDKSwindow (swindow, "</5/16>Command could not return rows.", BOTTOM);
	 }
	 else
	 {
	    /*
	     * The command returned some rows, print out the title.
	     */
	    char *row = assembleTitle (dbProcess);
	    addCDKSwindow (swindow, row, BOTTOM);
	    freeChar (row);

	    /* For each row returned, assemble the info. */
	    rowCount = 0;
	    while (dbnextrow (dbProcess) != NO_MORE_ROWS)
	    {
	       row = assembleRow (dbProcess);
	       addCDKSwindow (swindow, row, BOTTOM);
	       freeChar (row);
	    }
	 }
      }
   }

   /* Add in a output seperation line. */
   addCDKSwindow (swindow, "<C><#HL(5)>  End of Output  <#HL(5)>", BOTTOM);
   addCDKSwindow (swindow, "", BOTTOM);

   /* Can the query... */
   dbcanquery (dbProcess);
}

/*
 * This creates a single line from the column widths and values.
 */
char *assembleTitle (DBPROCESS *dbProc)
{
   char *colName	= 0;
   int colWidth		= 0;
   int colNameLen	= 0;
   int colCount		= dbnumcols (dbProc);
   int x		= 0;
   char temp[MAXWIDTH];
   char row[MAXWIDTH];

   /* Clean the row out. */
   memset (row, '\0', MAXWIDTH);

   /* Start assembling the row. */
   for (x=1; x <= colCount; x++)
   {
      colName		= dbcolname (dbProc, x);
      colWidth		= getColWidth (dbProc, x);
      colNameLen	= (int)strlen (colName);

      /* If we need to pad, then pad. */
      if (colNameLen < colWidth)
      {
	 /* Create a string the same length as the col width. */
	 memset (temp, '\0', MAXWIDTH);
	 memset (temp, ' ', (colWidth-colNameLen));

	 /* Copy the name. */
	 sprintf (row, "%s %s%s", row, colName, temp);
      }
      else
      {
	 /* Copy the name. */
	 sprintf (row, "%s %s", row, colName);
      }
   }
   return (char *)strdup (row);
}

/*
 * This assembles a single row.
 */
char *assembleRow (DBPROCESS *dbProcess)
{
   char *dataValue	= 0;
   int colCount		= dbnumcols (dbProcess);
   int columnType	= 0;
   int colWidth		= 0;
   int valueLen		= 0;
   char value[MAXWIDTH];
   char temp[MAXWIDTH];
   char row[MAXWIDTH];
   char format[20];
   int x;

   /* Clean out the row. */
   memset (row, '\0', MAXWIDTH);

   /* Start assembling the row. */
   for (x=1; x <= colCount; x++)
   {
      columnType	= (int)dbcoltype (dbProcess, x);
      colWidth		= (int)getColWidth (dbProcess, x);
      valueLen		= (int)dbdatlen (dbProcess, x);

      /* Check the column type. */
      if (columnType == SYBINT1)
      {
	 DBINT object_id = *((DBINT *)dbdata(dbProcess, x));
	 sprintf (format, "%%-%dd", colWidth);
	 sprintf (value, format, (int)object_id);
      }
      else if (columnType == SYBINT2)
      {
	 DBINT object_id = *((DBINT *)dbdata(dbProcess, x));
	 sprintf (format, "%%-%dd", colWidth);
	 sprintf (value, format, (int)object_id);
      }
      else if (columnType == SYBINT4)
      {
	 DBINT object_id = *((DBINT *)dbdata(dbProcess, x));
	 sprintf (format, "%%-%dd", colWidth);
	 sprintf (value, format, (int)object_id);
      }
      else if (columnType == SYBREAL)
      {
	 DBREAL object_id = *((DBREAL *)dbdata(dbProcess, x));
	 sprintf (format, "%%-%d.2f", colWidth);
	 sprintf (value, format, object_id);
      }
      else if (columnType == SYBFLT8)
      {
	 DBFLT8 object_id = *((DBFLT8 *)dbdata(dbProcess, x));
	 sprintf (format, "%%-%d.2f", colWidth);
	 sprintf (value, format, object_id);
      }
      else
      {
	 if (valueLen <= 0)
	 {
	    strcpy (value, " ");
	 }
	 else
	 {
	    memset (value, '\0', MAXWIDTH);
	    dataValue = (DBCHAR *)dbdata (dbProcess, x);
	    strncpy (value, dataValue, valueLen);
	 }
      }

      /* If we need to pad, then pad. */
      if (valueLen < colWidth)
      {
	 /* Copy the value into the string. */
	 memset (temp, '\0', MAXWIDTH);
	 memset (temp, ' ', (colWidth-valueLen));
	 sprintf  (row, "%s %s%s", row, value, temp);
      }
      else
      {
	 sprintf  (row, "%s %s", row, value);
      }
   }
   return copyChar (row);
}

/*
 * This function returns the correct width of a column, taking
 * into account the width of the title, the width of the data
 * element.
 */
int getColWidth (DBPROCESS *dbProcess, int col)
{
   char *colName	= dbcolname (dbProcess, col);
   int colNameLen	= (int)strlen(colName);
   int colWidth		= dbcollen (dbProcess, col);
   int columnType	= (int)dbcoltype (dbProcess, col);

   /* If the colType is int/real/float adjust accordingly. */
   if (columnType == SYBINT1 || columnType == SYBINT2 ||
	columnType == SYBINT4)
   {
      colWidth = 5;
   }
   else if (columnType == SYBREAL || columnType == SYBFLT8)
   {
      colWidth = 8;
   }

   /* Is the name of the column wider than the col width? */
   if (colNameLen >= colWidth)
   {
      return (colNameLen+1);
   }
   return colWidth;
}

/*
 * This callback allows the user to play with the scrolling window.
 */
void viewHistoryCB (EObjectType cdktype, void *object, void *clientData, chtype key)
{
   CDKSWINDOW *swindow	= (CDKSWINDOW *)clientData;
   CDKENTRY *entry	= (CDKENTRY *)object;

   /* Let them play... */
   activateCDKSwindow (swindow, 0);

   /* Redraw the entry field. */
   drawCDKEntry (entry, ObjOf(entry)->box);
}

/*
 * This displays a little introduction screen.
 */
void intro (CDKSCREEN *screen)
{
   int lines	= 0;
   char *mesg[10];

   /* Create the message. */
   mesg[lines++] = "";
   mesg[lines++] = "<C></B/16>Sybase Command Interface";
   mesg[lines++] = "<C>Written by Mike Glover";
   mesg[lines++] = "";
   mesg[lines++] = "<C>Type </B>help<!B> to get help.";

   /* Display the message. */
   popupLabel (screen, mesg, lines);
}

/*
 * This function displays help.
 */
void help (CDKENTRY *entry)
{
   int lines	= 0;
   char *mesg[25];

   /* Create the help message. */
   mesg[lines++] = "<C></B/29>Help";
   mesg[lines++] = "";
   mesg[lines++] = "</B/24>When in the command line.";
   mesg[lines++] = "<B=Up Arrow  > Scrolls back one command.";
   mesg[lines++] = "<B=Down Arrow> Scrolls forward one command.";
   mesg[lines++] = "<B=Tab       > Activates the scrolling window.";
   mesg[lines++] = "<B=help      > Displays this help window.";
   mesg[lines++] = "";
   mesg[lines++] = "</B/24>When in the scrolling window.";
   mesg[lines++] = "<B=l or L    > Loads a file into the window.";
   mesg[lines++] = "<B=s or S    > Saves the contents of the window to a file.";
   mesg[lines++] = "<B=Up Arrow  > Scrolls up one line.";
   mesg[lines++] = "<B=Down Arrow> Scrolls down one line.";
   mesg[lines++] = "<B=Page Up   > Scrolls back one page.";
   mesg[lines++] = "<B=Page Down > Scrolls forward one page.";
   mesg[lines++] = "<B=Tab or Esc> Returns to the command line.";
   mesg[lines++] = "<B=?         > Displays this help window.";
   mesg[lines++] = "";
   mesg[lines++] = "<C> (</B/24>Refer to the scrolling window online manual for more help<!B!24>.)";

   /* Pop up the help message. */
   popupLabel (entry->screen, mesg, lines);
}

/*
 * This converts a word to upper case.
 */
char *uc (char *word)
{
   int length	= strlen (word);
   char *upper	= (char *)malloc (sizeof (char *) * (length+2));
   int x;

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

/*
 * The following two functions are the error and message handler callbacks.
 * which will be called by Sybase when and error occurs.
 */
int err_handler (DBPROCESS *dbProcess, DBINT mesgNumber, int mesgState, int severity, char *mesgText)
{
   /* Declare local variables. */
   char *mesg[10], temp[256];
   int errorCount = 0;

   /* Check if the process is dead. */
   if ((dbProcess == 0) || (DBDEAD(dbProcess)))
   {
      mesg[0] = "</B/32>Database Process Error";
      mesg[1] = "<C>The database process seems to have died.";
      mesg[2] = "<C>Try logging in again with the </R>login<!R> command";
      popupLabel (GPCdkScreen, mesg, 3);
      return INT_EXIT;
   }
   else
   {
      mesg[0] = "</B/32>DB-Library Error";
      sprintf (temp, "<C>%s", dberrstr);
      mesg[1] = copyChar (temp);
      errorCount = 2;
   }

   /* Display the message if we have an error. */
   if (errorCount > 0)
   {
      popupLabel (GPCdkScreen, mesg, --errorCount);
   }

   /* Clean up. */
   if (errorCount == 2)
   {
      freeChar (mesg[1]);
   }
   return (1);
}
int mesg_handler (DBPROCESS *dbProcess, DBINT mesgNumber, int mesgState, int severity, char *mesgText)
{
   /* Declare local variables. */
   char *mesg[10], temp[256];

   /* Create the message. */
   mesg[0] = "<C></B/16>SQL Server Message";
   sprintf (temp, "</R>Message Number<!R> %ld", mesgNumber);
   mesg[1] = copyChar (temp);
   sprintf (temp, "</R>State         <!R> %d", mesgState);
   mesg[2] = copyChar (temp);
   sprintf (temp, "</R>Severity      <!R> %d", severity);
   mesg[3] = copyChar (temp);
   sprintf (temp, "</R>Message       <!R> %s", mesgText);
   mesg[4] = copyChar (temp);
   popupLabel (GPCdkScreen, mesg, 5);

   /* Clean up. */
   freeChar (mesg[1]); freeChar (mesg[2]);
   freeChar (mesg[3]); freeChar (mesg[4]);
   return (1);
}

/*
 * This is for the scrolling window help callback.
 */
void swindowHelpCB (EObjectType cdktype, void *object, void *clientData, chtype key)
{
   CDKENTRY *entry	= (CDKENTRY *)clientData;
   help(entry);
}

/*
 * This is the callback for the down arrow.
 */
void historyUpCB (EObjectType cdktype, void *object, void *clientData, chtype key)
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
void historyDownCB (EObjectType cdktype, void *object, void *clientData, chtype key)
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

   /* If we are at the end, clear the entry field. */
   if (history->current == history->count)
   {
      cleanCDKEntry (entry);
      drawCDKEntry (entry, ObjOf(entry)->box);
      return;
   }

   /* Display the command. */
   setCDKEntryValue (entry, history->command[history->current]);
   drawCDKEntry (entry, ObjOf(entry)->box);
}

/*
 * This callback allows the user to pick from the history list from a
 * scrolling list.
 */
void listHistoryCB (EObjectType cdktype, void *object, void *clientData, chtype key)
{
   CDKSCROLL *scrollList	= 0;
   CDKENTRY *entry		= (CDKENTRY *)object;
   struct history_st *history	= (struct history_st *) clientData;
   int height			= (history->count < 10 ? history->count+3 : 13);
   int selection;

   /* No history, no list. */
   if (history->count == 0)
   {
      /* Popup a little window telling the user there are no commands. */
      char *mesg[] = {"<C></B/16>No Commands Entered", "<C>No History"};
      popupLabel (entry->screen, mesg, 2);

      /* Redraw the screen. */
      eraseCDKEntry (ObjOf(entry));
      drawCDKScreen (entry->screen);

      /* And leave... */
      return;
   }

   /* Create the scrolling list of previous commands. */
   scrollList = newCDKScroll (entry->screen, CENTER, CENTER, RIGHT,
				height, -10, "<C></B/29>Command History",
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

   /* Redraw the screen. */
   eraseCDKEntry (ObjOf(entry));
   drawCDKScreen (ScreenOf(entry));
}

/*
 * This loads the history into the editor from the RC file.
 */
void loadHistory (struct history_st *history)
{
   char *home	= 0;
   char filename[1000];

   /* Create the RC filename. */
   if ((home = getenv ("HOME")) == 0)
   {
      home = ".";
   }
   sprintf (filename, "%s/.sybrc", home);

   /* Set some variables. */
   history->current	= 0;
   history->count	= 0;

   /* Read the file. */
   if ((history->count = readFile (filename, history->command, MAXHISTORY)) != -1)
   {
      history->current = history->count;
   }
   return;
}

/*
 * This saves the history into RC file.
 */
void saveHistory (struct history_st *history, int count)
{
   FILE *fd	= 0;
   char *home	= 0;
   char filename[1000];
   int x;

   /* Create the RC filename. */
   if ((home = getenv ("HOME")) == 0)
   {
      home = ".";
   }
   sprintf (filename, "%s/.sybrc", home);

   /* Open the file for writing. */
   if ((fd = fopen (filename, "w")) == 0)
   {
      return;
   }

   /* Start saving the history. */
   for (x=0; x < history->count; x++)
   {
      fprintf (fd, "%s\n", history->command[x]);
   }
   fclose (fd);
   return;
}
