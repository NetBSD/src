#include "rolodex.h"

#ifdef HAVE_XCURSES
char *XCursesProgramName="rolodex";
#endif

/*
 * This is the main part of the loop.
 */
int main(void)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen;
   CDKMENU *rolodexMenu;
   CDKLABEL *rolodexTitle;
   SRolodex groupList[MAXGROUPS];
   WINDOW *cursesWin;
   char *menulist[MAX_MENU_ITEMS][MAX_SUB_ITEMS], *title[5];
   char *home, temp[256], *mesg[15];
   int subMenuSize[10], menuLocations[10], selection;
   int group, ret, x;
   int groupCount = 0;

   /* Set up CDK  */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK color. */
   initCDKColor();

   /* Create the menu lists. */
   menulist[0][0] = "</U>File";menulist[1][0] = "</U>Groups";
   menulist[0][1] = "</B/5>Open   ";menulist[1][1] = "</B/5>New   ";
   menulist[0][2] = "</B/5>Save   ";menulist[1][2] = "</B/5>Open  ";
   menulist[0][3] = "</B/5>Save As";menulist[1][3] = "</B/5>Delete";
   menulist[0][4] = "</B/5>Quit   ";

   menulist[2][0] = "</U>Print";menulist[3][0] = "</U>Help";
   menulist[2][1] = "</B/5>Print Groups";menulist[3][1] = "</B/5>About Rolodex     ";
   menulist[3][2] = "</B/5>Rolodex Statistics";

   /* Set up the sub-menu sizes and their locations. */
   subMenuSize[0] = 5;menuLocations[0] = LEFT;
   subMenuSize[1] = 4;menuLocations[1] = LEFT;
   subMenuSize[2] = 2;menuLocations[2] = LEFT;
   subMenuSize[3] = 3;menuLocations[3] = RIGHT;

   /* Create the menu. */
   rolodexMenu = newCDKMenu (cdkscreen, menulist, 4,
				subMenuSize, menuLocations,
				TOP, A_BOLD|A_UNDERLINE, A_REVERSE);

   /* Create the title. */
   title[0]= "<C></U>Cdk Rolodex";
   title[1]= "<C></B/24>Written by Mike Glover";
   rolodexTitle = newCDKLabel (cdkscreen, CENTER, CENTER, title, 2, FALSE, FALSE);

   /* Define the help key binding. */
   bindCDKObject (vMENU, rolodexMenu, '?', helpCB, 0);

   /* Draw the CDK screen. */
   refreshCDKScreen (cdkscreen);

   /* Check the value of the HOME env var. */
   home = getenv ("HOME");
   if (home != 0)
   {
      /* Make sure the $HOME/.rolodex directory exists. */
      sprintf (temp, "%s/.rolodex", home);

      /* Set the value of the global rolodex DBM directory. */
      GDBMDir = copyChar (temp);

      /* Set the value of the global RC filename. */
      sprintf (temp, "%s/.rolorc", home);
      GRCFile = copyChar (temp);
   }
   else
   {
      /* Set the value of the global rolodex DBM directory. */
      GDBMDir = copyChar (".rolodex");

      /* Set the value of the global RC filename. */
      GRCFile = copyChar (".rolorc");
   }

   /* Make the rolodex directory. */
   mkdir (GDBMDir, 0755);

   /* Open the rolodex RC file. */
   groupCount = readRCFile (GRCFile, groupList);

   /* Check the value of groupCount. */
   if (groupCount < 0)
   {
      /* The RC file seems to be corrupt. */
      sprintf (temp, "<C></B/16>The RC file (%s) seems to be corrupt.", GRCFile);
      mesg[0] = copyChar (temp);
      mesg[1] = "<C></B/16>No rolodex groups were loaded.";
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
      freeChar (mesg[0]);
      groupCount = 0;
   }
   else if (groupCount == 0)
   {
      mesg[0] = "<C></B/24>Empty rolodex RC file. No groups loaded.";
      mesg[1] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 2);
   }
   else
   {
      if (groupCount == 1)
      {
	 sprintf (temp, "<C></24>There was 1 group loaded from the RC file.");
      }
      else
      {
	 sprintf (temp, "<C></24>There were %d groups loaded from the RC file.", groupCount);
      }
      mesg[0] = copyChar (temp);
      mesg[1] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 2);
      freeChar (mesg[0]);
   }

   /* Loop until we are done. */
   for (;;)
   {
      /* Activate the menu */
      selection= activateCDKMenu (rolodexMenu, 0);

      /* Check the return value of the selection. */
      if (selection == 0)
      {
	 /* Open the rolodex RC file. */
	 groupCount = openNewRCFile (cdkscreen, groupList, groupCount);
      }
      else if (selection == 1)
      {
	 /* Write out the RC file. */
	 ret = writeRCFile (cdkscreen, GRCFile, groupList, groupCount);

	 /* Reset the saved flag if the rc file saved ok. */
	 if (ret != 0)
	 {
	    GGroupModified = 0;
	 }
      }
      else if (selection == 2)
      {
	 /* Save as. */
	 ret = writeRCFileAs (cdkscreen, groupList, groupCount);

	 /* Reset the saved flag if the rc file saved ok. */
	 if (ret != 0)
	 {
	    GGroupModified = 0;
	 }
      }
      else if (selection == 3)
      {
	 /* Has anything changed??? */
	 if (GGroupModified != 0)
	 {
	    /* Write out the RC file. */
	    writeRCFile (cdkscreen, GRCFile, groupList, groupCount);
	 }

	 /* Remove the CDK widget pointers. */
	 destroyCDKMenu (rolodexMenu);
	 destroyCDKLabel (rolodexTitle);
	 destroyCDKScreen (cdkscreen);
	 delwin (cursesWin);

	 /* Free up other pointers. */
	 freeChar (GCurrentGroup);
	 freeChar (GRCFile);
	 freeChar (GDBMDir);

	 /* Clean up the group information. */
	 for (x=0; x < groupCount; x++)
	 {
	    freeChar (groupList[x].name);
	    freeChar (groupList[x].desc);
	    freeChar (groupList[x].dbm);
	 }

	 /* Shutdown CDK. */
	 endCDK();

	 /* Exit cleanly. */
	 exit (0);
      }
      else if (selection == 100)
      {
	 /* Add a new group to the list. */
	 groupCount = addRolodexGroup (cdkscreen, groupList, groupCount);
      }
      else if (selection == 101)
      {
	 /* If there are no groups, ask them if they want to create a new one. */
	 if (groupCount == 0)
	 {
	       char *buttons[] = {"<Yes>", "<No>"};
	       mesg[0] = "<C>There are no groups defined.";
	       mesg[1] = "<C>Do you want to define a new group?";

	       /* Add the group if they said yes. */
	       if (popupDialog (cdkscreen, mesg, 2, buttons, 2) == 0)
	       {
		  groupCount = addRolodexGroup (cdkscreen, groupList, groupCount);
	       }
	 }
	 else
	 {
	    /* Get the number of the group to open. */
	    group = pickRolodexGroup (cdkscreen, "<C></B/29>Open Rolodex Group",
					groupList, groupCount);

	    /* Make sure a group was picked. */
	    if (group >= 0)
	    {
	       /* Set the global variable GCurrentGroup */
	       freeChar (GCurrentGroup);
	       GCurrentGroup = copyChar (groupList[group].name);

	       /* Try to open the DBM file and read the contents. */
	       useRolodexGroup (cdkscreen,
				groupList[group].name,
				groupList[group].desc,
				groupList[group].dbm);
	    }
	 }
      }
      else if (selection == 102)
      {
	 /* Delete the group chosen. */
	 groupCount = deleteRolodexGroup (cdkscreen, groupList, groupCount);
      }
      else if (selection == 200)
      {
	 /* Print Phone Number Group. */
	 printGroupNumbers (cdkscreen, groupList, groupCount);
      }
      else if (selection == 300)
      {
	 /* About Rolodex. */
	 aboutCdkRolodex(cdkscreen);
      }
      else if (selection == 301)
      {
	 displayRolodexStats (ScreenOf(rolodexMenu), groupCount);
      }
   }
}

/*
 * This writes out the new RC file.
 */
int writeRCFile (CDKSCREEN *screen, char *filename, SRolodex *groupList, int groupCount)
{
   /* Declare local variables. */
   char *mesg[5], temp[256];
   time_t clck;
   FILE *fd;
   int x;

   /* Can we open the file?					*/
   if ((fd = fopen (filename, "w")) == 0)
   {
      /* Pop up a message. */
      sprintf (temp, "</B/16>The file <%s> could not be opened.", filename);
      mesg[0] = copyChar (temp);
#ifdef HAVE_STRERROR
      sprintf (temp, "<C></B/16>%s", strerror(errno));
#else
      sprintf (temp, "<C></B/16>Unknown reason.");
#endif
      mesg[1] = copyChar (temp);
      mesg[2] = "<C>Press any key to continue.";

      popupLabel (screen, mesg, 3);

      freeChar (mesg[0]);
      freeChar (mesg[1]);

      return (0);
   }

   /* Get the current time. */
   time(&clck);

   /* Put some comments at the top of the header. */
   fprintf (fd, "#\n");
   fprintf (fd, "# This file was automatically generated on %s", ctime(&clck));
   fprintf (fd, "#\n");

   /* Start writing the RC file. */
   for (x=0; x < groupCount; x++)
   {
      fprintf (fd, "%s%c%s%c%s\n",
	       groupList[x].name, CONTROL('V'),
	       groupList[x].desc, CONTROL('V'),
	       groupList[x].dbm);
   }
   fclose (fd);

   /* Pop up a message stating that it has been saved. */
   if (groupCount == 1)
   {
      sprintf (temp, "There was 1 group saved to file");
      mesg[0] = copyChar (temp);
   }
   else
   {
      sprintf (temp, "There were %d groups saved to file", groupCount);
      mesg[0] = copyChar (temp);
   }

   sprintf (temp, "<C>%s", filename);
   mesg[1] = copyChar (temp);
   mesg[2] = "<C>Press any key to continue.";

   popupLabel (screen, mesg, 3);

   /* Clean up. */
   freeChar (mesg[0]);
   freeChar (mesg[1]);
   return (TRUE);
}

/*
 * This allows the user to pick a DBM file to open.
 */
int pickRolodexGroup (CDKSCREEN *screen, char *title, SRolodex *groupList, int groupCount)
{
   /* Declare local variables. */
   CDKSCROLL *roloList	= 0;
   int height		= groupCount;
   char *mesg[MAXGROUPS], temp[256];
   int selection, x;

   /* Determine the height of the scrolling list. */
   if (groupCount > 5)
   {
      height = 5;
   }
   height += 3;

   /* Copy the names of the scrolling list into an array. */
   for (x=0; x < groupCount; x++)
   {
      sprintf (temp, "<C></B/29>%s", groupList[x].name);
      mesg[x] = copyChar (temp);
   }

   /* Create the scrolling list. */
   roloList = newCDKScroll (screen, CENTER, CENTER, NONE,
				height, 50, title, mesg, groupCount,
				NONUMBERS, A_REVERSE, TRUE, FALSE);

   /* Create a callback to the scrolling list. */
   bindCDKObject (vSCROLL, roloList, '?', groupInfoCB, groupList);

   /* Activate the scrolling list. */
   selection = activateCDKScroll (roloList, 0);

   /* Destroy the scrolling list. */
   destroyCDKScroll (roloList);
   freeCharList (mesg, groupCount);

   /* Return the item selected. */
   return selection;
}

/*
 * This allows the user to add a rolo group to the list.
 */
int addRolodexGroup (CDKSCREEN *screen, SRolodex *groupList, int groupCount)
{
   /* Declare local variables. */
   CDKENTRY *newName= 0;
   CDKENTRY *newDesc= 0;
   char *mesg[4], *desc, *newGroupName, temp[256];
   int x;

   /* Create the name widget. */
   newName = newCDKEntry (screen, CENTER, 8,
				"<C></B/29>New Group Name",
				"</B/29>   Name: ",
				A_NORMAL, '_', vMIXED,
				20, 2, 256, TRUE, FALSE);


   /* Get the name. */
   newGroupName = activateCDKEntry (newName, 0);

   /* Make sure they didn't hit escape. */
   if (newName->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C></B/16>Add Group Canceled.";
      destroyCDKEntry (newName);
      popupLabel (screen, mesg, 1);
      return groupCount;
   }

   /* Make sure that group name does not already exist. */
   for (x=0; x < groupCount; x++)
   {
      if (strcmp (newGroupName, groupList[x].name) == 0)
      {
	 sprintf (temp, "<C></B/16>Sorry the group (%s) already exists.", newGroupName);
	 mesg[0] =  copyChar (temp);
	 popupLabel (screen, mesg, 1);
	 freeChar (mesg[0]);
	 destroyCDKEntry (newName);
	 return groupCount;
      }
   }

   /* Keep the name. */
   groupList[groupCount].name = copyChar (newGroupName);

   /* Create the description widget. */
   newDesc = newCDKEntry (screen, CENTER, 13,
				"<C></B/29>Group Description",
				"</B/29>Descriprion: ",
				A_NORMAL, '_', vMIXED,
				20, 2, 256, TRUE, FALSE);

   /* Get the description. */
   desc = activateCDKEntry (newDesc, 0);

   /* Check if they hit escape or not. */
   if (newDesc->exitType == vESCAPE_HIT)
   {
      groupList[groupCount].desc = copyChar ("No Description Provided.");
   }
   else
   {
      groupList[groupCount].desc = copyChar (desc);
   }

   /* Create the DBM filename. */
   sprintf (temp, "%s/%s.phl", GDBMDir, groupList[groupCount].name);
   groupList[groupCount].dbm = copyChar (temp);

   /* Inrement the group count. */
   groupCount++;
   GGroupModified = 1;

   /* Destroy the widgets. */
   destroyCDKEntry (newName);
   destroyCDKEntry (newDesc);
   return groupCount;
}

/*
 * This displays rolodex information.
 */
void displayRolodexStats (CDKSCREEN *screen, int groupCount)
{
   /* Declare local variables. */
   char *mesg[6], temp[256];

   /* Create the information to display. */
   mesg[0] = "<C></U>Rolodex Statistics";
   sprintf (temp, "</B/5>Read Command Filename<!B!5> </U>%s<!U>", GRCFile);
   mesg[1] = copyChar (temp);
   sprintf (temp, "</B/5>Group Count          <!B!5> </U>%d<!U>", groupCount);
   mesg[2] = copyChar (temp);

   /* Display the message. */
   popupLabel (screen, mesg, 3);

   /* Clean up. */
   freeChar (mesg[1]);
   freeChar (mesg[2]);
}

/*
 * This function gets a new rc filename and saves the contents of the
 * groups under that name.
 */
int writeRCFileAs (CDKSCREEN *screen, SRolodex *groupList, int groupCount)
{
   /* Declare local variables. */
   CDKENTRY *newRCFile;
   char *newFilename;
   int ret;

   /* Create the entry field. */
   newRCFile = newCDKEntry (screen, CENTER, CENTER,
				"<C></R>Save As",
				"Filename: ",
				A_NORMAL, '_', vMIXED,
				20, 2, 256, TRUE, FALSE);

   /* Add a pre-process function so no spaces are introduced. */
   setCDKEntryPreProcess (newRCFile, entryPreProcessCB, 0);

   /* Get the filename. */
   newFilename = activateCDKEntry (newRCFile, 0);

   /* Check if they hit escape or not. */
   if (newRCFile->exitType == vESCAPE_HIT)
   {
      destroyCDKEntry (newRCFile);
      return 1;
   }

   /* Call the function to save the RC file. */
   ret = writeRCFile (screen, newFilename, groupList, groupCount);

   /* Reset the saved flag if the rc file saved ok. */
   if (ret != 0)
   {
      /* Change the default filename. */
      freeChar (GRCFile);
      GRCFile = copyChar (newFilename);
      GGroupModified = 0;
   }

   /* Clean up. */
   destroyCDKEntry (newRCFile);
   return 1;
}

/*
 * This opens a new RC file.
 */
int openNewRCFile (CDKSCREEN *screen, SRolodex *groupList, int groupCount)
{
   /* Declare local variables. */
   CDKFSELECT *fileSelector;
   char *filename, *mesg[10], temp[256];
   int x;

   /* Get the filename. */
   fileSelector = newCDKFselect (screen, CENTER, CENTER, 20, 55,
				"<C>Open RC File",
				"Filename: ",
				A_NORMAL, '.', A_REVERSE,
				"</5>", "</48>", "</N>", "</N>",
				TRUE, FALSE);

   /* Activate the file selector. */
   filename = activateCDKFselect (fileSelector, 0);

   /* Check if the file selector left early. */
   if (fileSelector->exitType == vESCAPE_HIT)
   {
      destroyCDKFselect (fileSelector);
      mesg[0] = "Open New RC File Aborted.";
      popupLabel (screen, mesg, 1);
      return groupCount;
   }

   /* Clean out the old information. */
   for (x=0; x < groupCount; x++)
   {
      freeChar (groupList[x].name);
      freeChar (groupList[x].desc);
      freeChar (groupList[x].dbm);
   }

   /* Open the RC file. */
   groupCount = readRCFile (filename, groupList);

   /* Check the return value. */
   if (groupCount < 0)
   {
      /* This file does not appear to be a rolodex file. */
      mesg[0] = "<C></B/16>The file<!B!16>";
      sprintf (temp, "<C></B/16>(%s)<!B!16>", filename);
      mesg[1] = copyChar (temp);
      mesg[2] = "<C>does not seem to be a rolodex RC file.";
      mesg[3] = "<C>Press any key to continue.";
      popupLabel (screen, mesg, 4);
      freeChar (mesg[1]);
      groupCount = 0;
   }

   /* Clean up. */
   destroyCDKFselect (fileSelector);
   return groupCount;
}

/*
 * This reads the users rc file.
 */
int readRCFile (char *filename, SRolodex *groupList)
{
   /* Declare variables. */
   int groupsFound= 0;
   int errorsFound= 0;
   char *lines[MAX_LINES];
   char **items;
   int linesRead, chunks, x;

   /* Open the file and start reading. */
   linesRead = readFile (filename, lines, MAX_LINES);

   /* Check the number of lines read. */
   if (linesRead == 0)
   {
      return (0);
   }

  /*
   * Cycle through what was given to us and save it.
   */
   for (x=0; x < linesRead; x++)
   {
      /* Strip white space from the line. */
      stripWhiteSpace (vBOTH, lines[x]);

      /* Only split lines which do not start with a # */
      if (strlen (lines[x]) != 0 && lines[x][0] != '#')
      {
	 items = CDKsplitString (lines[x], CONTROL('V'));
	 chunks = CDKcountStrings (items);

	 /* Only take the ones which fit the format. */
	 if (chunks == 3)
	 {
	    /* Clean off the name and DB name. */
	    stripWhiteSpace (vBOTH, items[0]);
	    stripWhiteSpace (vBOTH, items[1]);
	    stripWhiteSpace (vBOTH, items[2]);

	    /* Set the group name and DB name. */
	    groupList[groupsFound].name = items[0];
	    groupList[groupsFound].desc = items[1];
	    groupList[groupsFound].dbm = items[2];
	    groupsFound++;
	    free(items);
	 }
	 else
	 {
	    CDKfreeStrings(items);
	    errorsFound++;
	 }
      }
   }

   /* Clean up. */
   freeCharList (lines, linesRead);

   /* Check the number of groups to the number of errors. */
   if (errorsFound > 0 && groupsFound == 0)
   {
      /* This does NOT look like the rolodex RC file. */
      return -1;
   }
   return groupsFound;
}

/*
 * This function allows the user to add/delete/modify/save the
 * contents of a rolodex group.
 */
void useRolodexGroup (CDKSCREEN *screen, char *groupName, char *groupDesc GCC_UNUSED, char *groupDBM)
{
   /* Declare local variables. */
   CDKSCROLL *nameList= 0;
   CDKLABEL *helpWindow = 0;
   SPhoneData phoneData;
   SPhoneRecord *phoneRecord;
   char *Index[MAX_ITEMS], *title[3], *mesg[3], temp[256];
   int phoneCount, selection, height, x;

   /* Set up the help window at the bottom of the screen. */
   title[0]	= "<C><#HL(30)>";
   title[1]	= "<C>Press </B>?<!B> to get detailed help.";
   title[2]	= "<C><#HL(30)>";
   helpWindow= newCDKLabel (screen, CENTER, BOTTOM, title, 3, FALSE, FALSE);
   drawCDKLabel (helpWindow, FALSE);

   /* Open the DBM file and read in the contents of the file */
   phoneCount = readPhoneDataFile (groupDBM, &phoneData);
   phoneData.recordCount = phoneCount;

   /* Check the number of entries returned. */
   if (phoneCount == 0)
   {
      /*
       * They tried to open an empty group, maybe they want to
       * add a new entry to this number.
       */
      char *buttons[] = {"<Yes>", "<No>"};
      mesg[0] = "<C>There were no entries in this group.";
      mesg[1] = "<C>Do you want to add a new listng?";
      if (popupDialog (screen, mesg, 2, buttons, 2) == 1)
      {
	 destroyCDKLabel (helpWindow);
	 return;
      }

      /* Get the information for a new number. */
      if (addPhoneRecord (screen, &phoneData) != 0)
      {
	 return;
      }
   }
   else if (phoneCount < 0)
   {
      mesg[0] = "<C>Could not open the database for this group.";
      popupLabel (screen, mesg, 1);
      destroyCDKLabel (helpWindow);
      return;
   }

   /* Set up the data needed for the scrolling list. */
   for (x=0; x < phoneData.recordCount; x++)
   {
      phoneRecord = &phoneData.record[x];
      sprintf (temp, "</B/29>%s (%s)", phoneRecord->name, GLineType[phoneRecord->lineType]);
      Index[x] = copyChar (temp);
   }
   sprintf (temp, "<C>Listing of Group </U>%s", groupName);
   height = (phoneData.recordCount > 5 ? 8 : phoneData.recordCount+3);

   /* Create the scrolling list. */
   nameList = newCDKScroll (screen, CENTER, CENTER, RIGHT,
				height, 50, temp, Index,
				phoneData.recordCount,
				NUMBERS, A_REVERSE, TRUE, FALSE);

   /* Clean up. */
   freeCharList (Index, phoneData.recordCount);

   /* Create key bindings. */
   bindCDKObject (vSCROLL, nameList, 'i', insertPhoneEntryCB, &phoneData);
   bindCDKObject (vSCROLL, nameList, 'd', deletePhoneEntryCB, &phoneData);
   bindCDKObject (vSCROLL, nameList, DELETE, deletePhoneEntryCB, &phoneData);
   bindCDKObject (vSCROLL, nameList, '?', phoneEntryHelpCB, 0);

   /* Let them play. */
   selection = 0;
   while (selection >= 0)
   {
      /* Get the information they want to view. */
      selection = activateCDKScroll (nameList, 0);

      /* Display the information. */
      if (selection >= 0)
      {
	 /* Display the information. */
	 displayPhoneInfo (screen, phoneData.record[selection]);
      }
   }

   /* Save the rolodex information to file. */
   if (savePhoneDataFile (groupDBM, &phoneData) == 0)
   {
      /* Something happened. */
      mesg[0] = "<C>Could not save phone data to data file.";
      mesg[1] = "<C>All changes have been lost.";
      popupLabel (screen, mesg, 2);
   }

   /* Clean up. */
   for (x=0; x < phoneData.recordCount; x++)
   {
      freeChar (phoneData.record[x].name);
      freeChar (phoneData.record[x].phoneNumber);
      freeChar (phoneData.record[x].address);
      freeChar (phoneData.record[x].city);
      freeChar (phoneData.record[x].province);
      freeChar (phoneData.record[x].postalCode);
      freeChar (phoneData.record[x].desc);
   }
   destroyCDKScroll (nameList);
   destroyCDKLabel (helpWindow);
}

/*
 * This opens a phone data file and returns the number of elements read.
 */
int readPhoneDataFile (char *dataFile, SPhoneData *phoneData)
{
   /* Declare variables. */
   char *lines[MAX_LINES];
   char **items;
   int linesRead= 0;
   int chunks= 0;
   int linesFound= 0;
   int x;

   /* Open the file and start reading. */
   linesRead = readFile (dataFile, lines, MAX_LINES);

   /* Check the number of lines read. */
   if ((linesRead == 0) || (linesRead == -1))
   {
      return (0);
   }

  /*
   * Cycle through what was given to us and save it.
   */
   for (x=0; x < linesRead; x++)
   {
      if (lines[x][0] != '#')
      {
	 /* Split the string. */
	 items = CDKsplitString (lines[x], CONTROL('V'));
	 chunks = CDKcountStrings (items);

	 /* Copy the chunks. */
	 if (chunks == 8)
	 {
	    phoneData->record[linesFound].name		= items[0];
	    phoneData->record[linesFound].lineType	= atoi (items[1]);
	    phoneData->record[linesFound].phoneNumber	= items[2];
	    phoneData->record[linesFound].address	= items[3];
	    phoneData->record[linesFound].city		= items[4];
	    phoneData->record[linesFound].province	= items[5];
	    phoneData->record[linesFound].postalCode	= items[6];
	    phoneData->record[linesFound].desc		= items[7];
	    freeChar (items[1]);
	    free (items);
	    linesFound++;
	 }
	 else
	 {
	    /* Bad line in the file; recover the memory. */
	    CDKfreeStrings(items);
	 }
      }
   }

   /* Clean up. */
   freeCharList (lines, linesRead);

   /* Keep the record count and return. */
   phoneData->recordCount = linesFound;
   return linesFound;
}

/*
 * This opens a phone data file and returns the number of elements read.
 */
int savePhoneDataFile (char *filename, SPhoneData *phoneData)
{
   /* Declare variables. */
   SPhoneRecord *phoneRecord;
   time_t clck;
   FILE *fd;
   int x;

   /* Can we open the file? */
   if ((fd = fopen (filename, "w")) == 0)
   {
      return 0;
   }

   /* Get the current time. */
   time(&clck);

   /* Add the header to the file. */
   fprintf (fd, "#\n");
   fprintf (fd, "# This file was automatically saved on %s", ctime(&clck));
   fprintf (fd, "# There should be %d phone numbers in this file.\n", phoneData->recordCount);
   fprintf (fd, "#\n");

   /* Cycle through the data and start writing it to the file. */
   for (x=0; x < phoneData->recordCount; x++)
   {
      /* Get a pointer to the object. */
      phoneRecord = &phoneData->record[x];

      /* Check the phone type. */
      if (phoneRecord->lineType == vCell || phoneRecord->lineType == vPager)
      {
	 fprintf (fd, "%s%c%d%c%s%c-%c-%c-%c-%c%s\n",
			phoneRecord->name, CONTROL('V'),
			phoneRecord->lineType, CONTROL('V'),
			phoneRecord->phoneNumber, CONTROL('V'),
			CONTROL('V'), CONTROL('V'), CONTROL('V'), CONTROL('V'),
			phoneRecord->desc);
      }
      else
      {
	 fprintf (fd, "%s%c%d%c%s%c%s%c%s%c%s%c%s%c%s\n",
			phoneRecord->name, CONTROL('V'),
			phoneRecord->lineType, CONTROL('V'),
			phoneRecord->phoneNumber, CONTROL('V'),
			phoneRecord->address, CONTROL('V'),
			phoneRecord->city, CONTROL('V'),
			phoneRecord->province, CONTROL('V'),
			phoneRecord->postalCode, CONTROL('V'),
			phoneRecord->desc);
      }
   }
   fclose (fd);
   return 1;
}

/*
 * This displays the information about the phone record.
 */
void displayPhoneInfo (CDKSCREEN *screen, SPhoneRecord record)
{
   /* Declare local variables. */
   char *mesg[10], temp[256];

   /* Check the type of line it is. */
   if (record.lineType == vVoice ||
	record.lineType == vData1 ||
	record.lineType == vData2 ||
	record.lineType == vData3 ||
	record.lineType == vFAX1 ||
	record.lineType == vFAX2 ||
	record.lineType == vFAX3)
   {
      /* Create the information to display. */
      sprintf (temp, "<C></U>%s Phone Record", GLineType[record.lineType]);
      mesg[0] = copyChar (temp);

      sprintf (temp, "</B/29>Name        <!B!29>%s", record.name);
      mesg[1] = copyChar (temp);

      sprintf (temp, "</B/29>Phone Number<!B!29>%s", record.phoneNumber);
      mesg[2] = copyChar (temp);

      sprintf (temp, "</B/29>Address     <!B!29>%s", record.address);
      mesg[3] = copyChar (temp);

      sprintf (temp, "</B/29>City        <!B!29>%s", record.city);
      mesg[4] = copyChar (temp);

      sprintf (temp, "</B/29>Province    <!B!29>%s", record.province);
      mesg[5] = copyChar (temp);

      sprintf (temp, "</B/29>Postal Code <!B!29>%s", record.postalCode);
      mesg[6] = copyChar (temp);

      sprintf (temp, "</B/29>Comment     <!B!29>%s", record.desc);
      mesg[7] = copyChar (temp);

      /* Pop the information up on the screen. */
      popupLabel (screen, mesg, 8);

      /* Clean up. */
      freeCharList (mesg, 8);
   }
   else if (record.lineType == vPager || record.lineType == vCell)
   {
      /* Create the information to display. */
      sprintf (temp, "<C></U>%s Phone Record", GLineType[record.lineType]);
      mesg[0] = copyChar (temp);

      sprintf (temp, "</B/29>Name        <!B!29>%s", record.name);
      mesg[1] = copyChar (temp);

      sprintf (temp, "</B/29>Phone Number<!B!29>%s", record.phoneNumber);
      mesg[2] = copyChar (temp);

      sprintf (temp, "</B/29>Comment     <!B!29>%s", record.desc);
      mesg[3] = copyChar (temp);

      /* Pop the information up on the screen. */
      popupLabel (screen, mesg, 4);

      /* Clean up. */
      freeCharList (mesg, 4);
   }
   else
   {
      mesg[0] = "<C></R>Error<!R> </U>Unknown Phone Line Type";
      mesg[1] = "<C>Can not display information.";
      popupLabel (screen, mesg, 2);
   }
}

/*
 * This function displays a little pop up window discussing this demo.
 */
void aboutCdkRolodex (CDKSCREEN *screen)
{
   char *mesg[15];

   mesg[0] = "<C></U>About Cdk Rolodex";
   mesg[1] = " ";
   mesg[2] = "</B/24>This demo was written to demonstrate the widgets";
   mesg[3] = "</B/24>available with the Cdk library. Not all of the";
   mesg[4] = "</B/24>Cdk widgets are used, but most of them have been.";
   mesg[5] = "</B/24>I hope this little demonstration helps give you an";
   mesg[6] = "</B/24>understanding of what the Cdk library offers.";
   mesg[7] = " ";
   mesg[8] = "<C></B/24>Have fun with it.";
   mesg[9] = " ";
   mesg[10] = "</B/24>ttfn,";
   mesg[11] = "<C></B/24>Mike";
   mesg[12] = "<C><#HL(35)>";
   mesg[13] = "<R></B/24>March 1996";

   popupLabel (screen, mesg, 14);
}

/*
 * This deletes a rolodex group.
 */
int deleteRolodexGroup (CDKSCREEN *screen, SRolodex *groupList, int groupCount)
{
   /* Declare local variables. */
   char *mesg[10], *buttons[5], temp[256];
   int selection, choice, x;

   /* If there are no groups, pop up a message telling them. */
   if (groupCount == 0)
   {
      mesg[0] = "<C>Error";
      mesg[1] = "<C>There as no groups defined.";
      popupLabel (screen, mesg, 2);

      /* Return the current group count. */
      return groupCount;
   }

   /* Get the number of the group to delete. */
   selection = pickRolodexGroup (screen, "<C></U>Delete Which Rolodex Group?", groupList, groupCount);

   /* Check the results. */
   if (selection < 0)
   {
      mesg[0] = "<C>   Delete Canceled   ";
      mesg[1] = "<C>No Group Deleted";
      popupLabel (screen, mesg, 2);
      return groupCount;
   }

   /* Let's make sure they want to delete the group. */
   mesg[0] = "<C></U>Confirm Delete";
   mesg[1] = "<C>Are you sure you want to delete the group";
   sprintf (temp, "<C></R>%s<!R>?", groupList[selection].name);
   mesg[2] = copyChar (temp);
   buttons[0] = "<No>";
   buttons[1] = "<Yes>";
   choice = popupDialog (screen, mesg, 3, buttons, 2);
   freeChar (mesg[2]);

   /* Check the results of the confirmation. */
   if (choice == 0)
   {
      mesg[0] = "<C>   Delete Canceled   ";
      mesg[1] = "<C>No Group Deleted";
      popupLabel (screen, mesg, 2);
      return groupCount;
   }

   /* We need to delete the group file first. */
   unlink (groupList[selection].dbm);

   /* OK, lets delete the group. */
   freeChar (groupList[selection].name);
   freeChar (groupList[selection].desc);
   freeChar (groupList[selection].dbm);
   for (x=selection; x < groupCount-1; x++)
   {
      groupList[x].name = groupList[x+1].name;
      groupList[x].desc = groupList[x+1].desc;
      groupList[x].dbm = groupList[x+1].dbm;
   }
   groupCount--;
   GGroupModified = 1;

   /* Clean up. */
   return groupCount;
}

/*
 * This function gets information about a new phone number.
 */
int addPhoneRecord (CDKSCREEN *screen, SPhoneData *phoneData)
{
   /* Declare local variables. */
   CDKLABEL *title;
   CDKITEMLIST *itemList;
   SPhoneRecord *phoneRecord;
   char *titleMesg[3], *types[GLINETYPECOUNT], temp[256];
   int ret, x;

   /* Get the phone record pointer. */
   phoneRecord = &phoneData->record[phoneData->recordCount];

   /* Create a title label to display. */
   titleMesg[0] = "<C></B/16>Add New Phone Record";
   title = newCDKLabel (screen, CENTER, TOP, titleMesg, 1, FALSE, FALSE);
   drawCDKLabel (title, FALSE);

   /* Create the phone line type list. */
   for (x=0; x < GLINETYPECOUNT; x++)
   {
      sprintf (temp, "<C></U>%s", GLineType[x]);
      types[x] = copyChar (temp);
   }

   /* Get the phone line type. */
   itemList = newCDKItemlist (screen, CENTER, CENTER,
				"<C>What Type Of Line Is It?",
				"Type: ",
				types, GLINETYPECOUNT, 0,
				TRUE, FALSE);
   phoneRecord->lineType = activateCDKItemlist (itemList, 0);
   destroyCDKItemlist (itemList);

   /* Clean up. */
   freeCharList (types, GLINETYPECOUNT);

   /* Check the return code of the line type question. */
   if (phoneRecord->lineType == -1)
   {
      phoneRecord->lineType = 0;
      return 1;
   }
   else if (phoneRecord->lineType == vPager || phoneRecord->lineType == vCell)
   {
      ret = getSmallPhoneRecord (screen, phoneRecord);
   }
   else
   {
      ret = getLargePhoneRecord (screen, phoneRecord);
   }

   /* Check the return value from the getXXXPhoneRecord function.*/
   if (ret == 0)
   {
      phoneData->recordCount++;
   }

   /* Clean up. */
   destroyCDKLabel (title);

   /* Return the new phone list count. */
   return ret;
}

/*
 * This gets a phone record with all of the details.
 */
int getLargePhoneRecord (CDKSCREEN *screen, SPhoneRecord *phoneRecord)
{
   /* Declare local variables. */
   CDKENTRY *nameEntry, *addressEntry, *cityEntry;
   CDKENTRY *provEntry, *postalEntry, *descEntry;
   CDKTEMPLATE *phoneTemplate;
   char *buttons[5], *mesg[15];
   int ret;

   /* Define the widgets. */
   nameEntry = newCDKEntry (screen, LEFT, 5,
				0, "</B/5>Name: ",
				A_NORMAL,
				'_', vMIXED, 20, 2, 256,
				TRUE, FALSE);
   addressEntry = newCDKEntry (screen, RIGHT, 5,
				0, "</B/5>Address: ",
				A_NORMAL,
				'_', vMIXED, 40, 2, 256,
				TRUE, FALSE);
   cityEntry = newCDKEntry (screen, LEFT, 8,
				0, "</B/5>City: ",
				A_NORMAL,
				'_', vMIXED, 20, 2, 256,
				TRUE, FALSE);
   provEntry = newCDKEntry (screen, 29, 8,
				0, "</B/5>Province: ",
				A_NORMAL,
				'_', vMIXED, 15, 2, 256,
				TRUE, FALSE);
   postalEntry = newCDKEntry (screen, RIGHT, 8,
				0, "</B/5>Postal Code: ",
				A_NORMAL,
				'_', vUMIXED, 8, 2, 256,
				TRUE, FALSE);
   phoneTemplate = newCDKTemplate (screen, LEFT, 11,
				0,
				"</B/5>Number: ",
				"(###) ###-####",
				"(___) ___-____",
				TRUE, FALSE);
   descEntry = newCDKEntry (screen, RIGHT, 11,
				0,
				"</B/5>Description: ",
				A_NORMAL,
				'_', vMIXED, 20, 2, 256,
				TRUE, FALSE);

   /* Get the phone information. */
   for (;;)
   {
      /* Draw the widgets on the screen. */
      drawCDKEntry (nameEntry,	  ObjOf(nameEntry)->box);
      drawCDKEntry (addressEntry, ObjOf(addressEntry)->box);
      drawCDKEntry (cityEntry,	  ObjOf(cityEntry)->box);
      drawCDKEntry (provEntry,	  ObjOf(provEntry)->box);
      drawCDKEntry (postalEntry,  ObjOf(postalEntry)->box);
      drawCDKTemplate (phoneTemplate, ObjOf(phoneTemplate)->box);
      drawCDKEntry (descEntry,	  ObjOf(descEntry)->box);

      /* Activate the entries to get the information. */
      phoneRecord->name		= copyChar (activateCDKEntry (nameEntry, 0));
      phoneRecord->address	= copyChar (activateCDKEntry (addressEntry, 0));
      phoneRecord->city		= copyChar (activateCDKEntry (cityEntry, 0));
      phoneRecord->province	= copyChar (activateCDKEntry (provEntry, 0));
      phoneRecord->postalCode	= copyChar (activateCDKEntry (postalEntry, 0));
      activateCDKTemplate (phoneTemplate, 0);
      phoneRecord->phoneNumber	= mixCDKTemplate (phoneTemplate);
      phoneRecord->desc = copyChar (activateCDKEntry (descEntry, 0));

      /* Determine if the user wants to submit the info. */
      mesg[0] = "<C></U>Confirm New Phone Entry";
      mesg[1] = "<C>Do you want to add this phone number?";
      buttons[0] = "</B/24><Add Phone Number>";
      buttons[1] = "</B/16><Cancel>";
      buttons[2] = "</B/32><Modify Information>";
      ret = popupDialog (screen, mesg, 2, buttons, 3);

      /* Check the response of the popup dialog box. */
      if (ret == 0)
      {
	 /* The user wants to submit the information. */
	 destroyCDKEntry (nameEntry);
	 destroyCDKEntry (addressEntry);
	 destroyCDKEntry (cityEntry);
	 destroyCDKEntry (provEntry);
	 destroyCDKEntry (postalEntry);
	 destroyCDKEntry (descEntry);
	 destroyCDKTemplate (phoneTemplate);
	 return ret;
      }
      else if (ret == 1)
      {
	 /* The user does not want to submit the information. */
	 freeChar (phoneRecord->name);
	 freeChar (phoneRecord->address);
	 freeChar (phoneRecord->city);
	 freeChar (phoneRecord->province);
	 freeChar (phoneRecord->postalCode);
	 freeChar (phoneRecord->phoneNumber);
	 freeChar (phoneRecord->desc);
	 destroyCDKEntry (nameEntry);
	 destroyCDKEntry (addressEntry);
	 destroyCDKEntry (cityEntry);
	 destroyCDKEntry (provEntry);
	 destroyCDKEntry (postalEntry);
	 destroyCDKEntry (descEntry);
	 destroyCDKTemplate (phoneTemplate);
	 return ret;
      }
      else
      {
	 /* The user wants to edit the information again. */
	 freeChar (phoneRecord->name);
	 freeChar (phoneRecord->address);
	 freeChar (phoneRecord->city);
	 freeChar (phoneRecord->province);
	 freeChar (phoneRecord->postalCode);
	 freeChar (phoneRecord->phoneNumber);
	 freeChar (phoneRecord->desc);
      }
   }
}

/*
 * This gets a small phone record.
 */
int getSmallPhoneRecord (CDKSCREEN *screen, SPhoneRecord *phoneRecord)
{
   /* Declare local variables. */
   CDKENTRY *nameEntry, *descEntry;
   CDKTEMPLATE *phoneTemplate;
   char *buttons[5], *mesg[15];
   int ret;

   /* Define the widgets. */
   nameEntry = newCDKEntry (screen, CENTER, 8,
				0, "</B/5>Name: ",
				A_NORMAL,
				'_', vMIXED, 20, 2, 256,
				TRUE, FALSE);
   phoneTemplate = newCDKTemplate (screen, CENTER, 11,
				0, "</B/5>Number: ",
				"(###) ###-####",
				"(___) ___-____",
				TRUE, FALSE);
   descEntry = newCDKEntry (screen, CENTER, 14,
				0, "</B/5>Description: ",
				A_NORMAL,
				'_', vMIXED, 20, 2, 256,
				TRUE, FALSE);

   /* Get the phone information. */
   for (;;)
   {
      /* Draw the widgets on the screen. */
      drawCDKEntry (nameEntry, ObjOf(nameEntry)->box);
      drawCDKTemplate (phoneTemplate, ObjOf(phoneTemplate)->box);
      drawCDKEntry (descEntry, ObjOf(descEntry)->box);

      /* Activate the entries to get the information. */
      phoneRecord->name		= copyChar (activateCDKEntry (nameEntry, 0));
      activateCDKTemplate (phoneTemplate, 0);
      phoneRecord->phoneNumber	= mixCDKTemplate (phoneTemplate);
      phoneRecord->desc		= copyChar (activateCDKEntry (descEntry, 0));
      phoneRecord->address	= copyChar ("-");
      phoneRecord->city		= copyChar ("-");
      phoneRecord->province	= copyChar ("-");
      phoneRecord->postalCode	= copyChar ("-");

      /* Determine if the user wants to submit the info. */
      mesg[0] = "<C></B/5>Confirm New Phone Entry";
      mesg[1] = "<C>Do you want to add this phone number?";
      buttons[0] = "</B/24><Add Phone Number>";
      buttons[1] = "</B/16><Cancel>";
      buttons[2] = "</B/8><Modify Information>";
      ret = popupDialog (screen, mesg, 2, buttons, 3);

      /* Check the response of the popup dialog box. */
      if (ret == 0)
      {
	 /* The user wants to submit the information. */
	 destroyCDKEntry (nameEntry);
	 destroyCDKEntry (descEntry);
	 destroyCDKTemplate (phoneTemplate);
	 return ret;
      }
      else if (ret == 1)
      {
	 /* The user does not want to submit the information. */
	 freeChar (phoneRecord->name);
	 freeChar (phoneRecord->phoneNumber);
	 freeChar (phoneRecord->desc);
	 freeChar (phoneRecord->address);
	 freeChar (phoneRecord->city);
	 freeChar (phoneRecord->province);
	 freeChar (phoneRecord->postalCode);
	 destroyCDKEntry (nameEntry);
	 destroyCDKEntry (descEntry);
	 destroyCDKTemplate (phoneTemplate);
	 return ret;
      }
      else
      {
	 /* The user wants to edit the information again. */
	 freeChar (phoneRecord->name);
	 freeChar (phoneRecord->phoneNumber);
	 freeChar (phoneRecord->desc);
	 freeChar (phoneRecord->address);
	 freeChar (phoneRecord->city);
	 freeChar (phoneRecord->province);
	 freeChar (phoneRecord->postalCode);
      }
   }
}

/*
 * This prints a groups phone numbers.
 */
void printGroupNumbers (CDKSCREEN *screen, SRolodex *groupList, int groupCount)
{
   /* Declare local variables. */
   CDKSELECTION *selectionList;
   CDKENTRY *entry;
   CDKLABEL *title;
   char *itemList[MAX_ITEMS], *mesg[10], temp[256];
   char *choices[] = {"Print to Printer ", "Print to File", "Don't Print"};
   char *filename = 0;
   char *printer = 0;
   char *defaultPrinter = 0;
   int height = groupCount;
   int x;

   /* Create the group list. */
   for (x=0; x < groupCount; x++)
   {
      itemList[x] = copyChar (groupList[x].name);
   }

   /* Set the height of the selection list. */
   if (groupCount > 5)
   {
      height = 5;
   }
   height += 3;

   /* Create the selection list. */
   selectionList = newCDKSelection (screen, CENTER, CENTER, RIGHT,
					height, 40,
					"<C></U>Select Which Groups To Print",
					itemList, groupCount,
					choices, 3,
					A_REVERSE, TRUE, FALSE);

   /* Activate the selection list. */
   if (activateCDKSelection (selectionList, 0) == -1)
   {
      /* Tell the user they exited early. */
      destroyCDKSelection (selectionList);
      mesg[0] = "<C>Print Canceled.";
      popupLabel (screen, mesg, 1);

      /* Clean up. */
      freeCharList (itemList, groupCount);
      return;
   }
   eraseCDKSelection (selectionList);

   /* Determine which groups we want to print. */
   for (x=0; x < groupCount; x++)
   {
      if (selectionList->selections[x] == 0)
      {
	 /* Create a title. */
	 sprintf (temp, "<C></R>Printing Group [%s] to Printer", groupList[x].name);
	 mesg[0] = copyChar (temp);
	 title = newCDKLabel (screen, CENTER, TOP, mesg, 1, FALSE, FALSE);
	 drawCDKLabel (title, FALSE);
	 freeChar (mesg[0]);

	 /* Get the printer name to print to. */
	 entry = newCDKEntry (screen, CENTER, 8,
				0, "</R>Printer Name: ",
				A_NORMAL,
				'_', vMIXED, 20, 2, 256, TRUE, FALSE);

	 /* Set the printer name to the default printer. */
	 defaultPrinter = getenv("PRINTER");
	 setCDKEntry (entry, defaultPrinter, 2, 256, TRUE);
	 printer = copyChar (activateCDKEntry (entry, 0));
	 destroyCDKEntry (entry);

	 /* Print the group. */
	 if (printGroup (groupList[x], "/tmp/rolodex.tmp", printer) == 0)
	 {
	    /* The group could not be printed. */
	    sprintf (temp, "<C>Sorry the group '%s' could not be printed.", groupList[x].name);
	    mesg[0] = copyChar (temp);
	    popupLabel (screen, mesg, 1);
	    freeChar (mesg[0]);
	 }

	 /* Clean up. */
	 destroyCDKLabel (title);
	 freeChar (printer);
	 unlink ("/tmp/rolodex.tmp");
      }
      else if (selectionList->selections[x] == 1)
      {
	 /* Create a title. */
	 sprintf (temp, "<C></R>Printing Group [%s] to File", groupList[x].name);
	 mesg[0] = copyChar (temp);
	 title = newCDKLabel (screen, CENTER, TOP, mesg, 1, FALSE, FALSE);
	 drawCDKLabel (title, FALSE);
	 freeChar (mesg[0]);

	 /* Get the filename to print to. */
	 entry = newCDKEntry (screen, CENTER, 8,
				0, "</R>Filename: ",
				A_NORMAL, '_', vMIXED,
				20, 2, 256, TRUE, FALSE);
	 filename = copyChar (activateCDKEntry (entry, 0));
	 destroyCDKEntry (entry);

	 /* Print the group. */
	 if (printGroup (groupList[x], filename, printer) == 0)
	 {
	    /* The group could not be printed. */
	    sprintf (temp, "<C>Sorry the group '%s' could not be printed.", groupList[x].name);
	    mesg[0] = copyChar (temp);
	    popupLabel (screen, mesg, 1);
	    freeChar (mesg[0]);
	 }

	 /* Clean up. */
	 destroyCDKLabel (title);
	 freeChar (filename);
      }
   }

   /* Clean up. */
   destroyCDKSelection (selectionList);
   freeCharList (itemList, groupCount);
}

/*
 * This actually prints the phone record.
 */
int printGroup (SRolodex groupRecord, char *filename, char *printer)
{
   /* Declare local variables. */
   uid_t uid = getuid();
   char tempFilename[256], command[256];
   SPhoneData phoneData;
   SPhoneRecord *phoneRecord;
   int phoneCount, x;
   FILE *fd;

   /* Read the data file. */
   phoneCount = readPhoneDataFile (groupRecord.dbm, &phoneData);

   /* Create the temporary filename. */
   if (filename != 0)
   {
      sprintf (tempFilename, "%s", filename);
   }
   else
   {
      sprintf (tempFilename, "/tmp/rolodex.%d", (int)uid);
   }

   /* Open the file. */
   if ((fd = fopen (tempFilename, "a+")) == 0)
   {
      /* Clean up. */
      for (x=0; x < phoneCount; x++)
      {
	 freeChar (phoneData.record[x].name);
	 freeChar (phoneData.record[x].phoneNumber);
	 freeChar (phoneData.record[x].address);
	 freeChar (phoneData.record[x].city);
	 freeChar (phoneData.record[x].province);
	 freeChar (phoneData.record[x].postalCode);
	 freeChar (phoneData.record[x].desc);
      }
      return 0;
   }

   /* Start writing the group information to the temp file. */
   fprintf (fd, "Group Name: %40s\n", groupRecord.name);
   fprintf (fd, "==============================================================================\n");
   for (x=0; x < phoneCount; x++)
   {
      phoneRecord = &phoneData.record[x];
      fprintf (fd, "Name        : %s\n", phoneRecord->name);
      fprintf (fd, "Phone Number: %s (%s)\n", phoneRecord->phoneNumber, GLineType[phoneRecord->lineType]);
      if (phoneRecord->lineType != vPager && phoneRecord->lineType != vCell)
      {
	 fprintf (fd, "Address     : %-20s, %-20s\n", phoneRecord->address, phoneRecord->city);
	 fprintf (fd, "            : %-10s, %-10s\n", phoneRecord->province, phoneRecord->postalCode);
      }
      fprintf (fd, "Description : %-30s\n", phoneRecord->desc);
      fprintf (fd, "------------------------------------------------------------------------------\n");
   }

   /* Determine if the information is going to a file or printer. */
   if (printer != 0)
   {
      /* Print the file to the given printer. */
      sprintf (command, "lpr -P%s %s", printer, tempFilename);
      system (command);

      /* We have to unlkink the temp file. */
      unlink (tempFilename);
   }

   /* Clean up some memory. */
   for (x=0; x < phoneCount; x++)
   {
      freeChar (phoneData.record[x].name);
      freeChar (phoneData.record[x].phoneNumber);
      freeChar (phoneData.record[x].address);
      freeChar (phoneData.record[x].city);
      freeChar (phoneData.record[x].province);
      freeChar (phoneData.record[x].postalCode);
      freeChar (phoneData.record[x].desc);
   }

   /* Close the filename. */
   fclose (fd);
   return 1;
}

/*
 ****************************************************************
 *		    Start of callback functions.
 ****************************************************************
 */
int entryPreProcessCB (EObjectType cdkType GCC_UNUSED, void *object GCC_UNUSED, void *clientData GCC_UNUSED, chtype input)
{
   if (input == ' ')
   {
      Beep();
      return 0;
   }
   return 1;
}

/*
 * This allows the user to insert a new phone entry into the database.
 */
void insertPhoneEntryCB (EObjectType cdkType GCC_UNUSED, void *object, void *clientData, chtype key GCC_UNUSED)
{
   /* Declare local variables. */
   CDKSCROLL *scrollp= (CDKSCROLL *)object;
   SPhoneData *phoneData= (SPhoneData *)clientData;
   SPhoneRecord *phoneRecord= &phoneData->record[phoneData->recordCount];
   char temp[256];

   /* Make the scrolling list disappear. */
   eraseCDKScroll (scrollp);

   /* Call the function which gets phone record information. */
   if (addPhoneRecord (ScreenOf(scrollp), phoneData) == 0)
   {
      sprintf (temp, "%s (%s)", phoneRecord->name, GLineType[phoneRecord->lineType]);
      addCDKScrollItem (scrollp, temp);
   }

   /* Redraw the scrolling list. */
   drawCDKScroll (scrollp, ObjOf(scrollp)->box);
}

/*
 * This allows the user to delete a phone entry from the database.
 */
void deletePhoneEntryCB (EObjectType cdkType GCC_UNUSED, void *object, void *clientData, chtype key GCC_UNUSED)
{
   /* Declare local variables. */
   CDKSCROLL *scrollp = (CDKSCROLL *)object;
   SPhoneData *phoneData = (SPhoneData *)clientData;
   char *mesg[3], temp[256], *hold;
   char *buttons[] = {"</B/16><No>", "</B/24><Yes>"};
   int position = scrollp->currentItem;
   int x;

   /* Make the scrolling list disappear. */
   eraseCDKScroll (scrollp);

   /* Check the number of entries left in the list. */
   if (scrollp->listSize == 0)
   {
      mesg[0] = "There are no more numbers to delete.";
      popupLabel (ScreenOf(scrollp), mesg, 1);
      return;
   }

   /* Ask the user if they really want to delete the listing. */
   mesg[0] = "<C>Do you really want to delete the phone entry";
   hold = chtype2Char (scrollp->item[scrollp->currentItem]);
   sprintf (temp, "<C></B/16>%s", hold);
   freeChar (hold);
   mesg[1] = copyChar (temp);
   if (popupDialog (ScreenOf(scrollp), mesg, 2, buttons, 2) == 1)
   {
      /* Remove the item from the phone data record. */
      for (x=position; x < phoneData->recordCount-1; x++)
      {
	 phoneData->record[x].name = phoneData->record[x+1].name;
	 phoneData->record[x].lineType = phoneData->record[x+1].lineType;
	 phoneData->record[x].phoneNumber = phoneData->record[x+1].phoneNumber;
	 phoneData->record[x].address = phoneData->record[x+1].address;
	 phoneData->record[x].city = phoneData->record[x+1].city;
	 phoneData->record[x].province = phoneData->record[x+1].province;
	 phoneData->record[x].postalCode = phoneData->record[x+1].postalCode;
	 phoneData->record[x].desc = phoneData->record[x+1].desc;
      }
      phoneData->recordCount--;

      /* Nuke the entry. */
      deleteCDKScrollItem (scrollp, position);
   }

   /* Clean up. */
   freeChar (mesg[1]);
}

/*
 * This function provices help for the phone list editor.
 */
void phoneEntryHelpCB (EObjectType cdkType GCC_UNUSED, void *object, void *clientData GCC_UNUSED, chtype key GCC_UNUSED)
{
   /* Declare local variables. */
   CDKSCROLL *scrollp = (CDKSCROLL *)object;
   char *mesg[10], temp[100];

   /* Create the help title. */
   mesg[0] = "<C></R>Rolodex Phone Editor";
   mesg[1] = "<B=i     > Inserts a new phone entry.";
   mesg[2] = "<B=d     > Deletes the currently selected phone entry.";
   mesg[3] = "<B=Escape> Exits the scrolling list.";
   mesg[4] = "<B=?     > Pops up this help window.";

   /* Pop up the message. */
   popupLabel (ScreenOf(scrollp), mesg, 5);
}

/*
 * This is a callback to the menu widget. It allows the user to
 * ask for help about any sub-menu item.
 */
void helpCB (EObjectType cdkType GCC_UNUSED, void *object, void *clientData GCC_UNUSED, chtype key GCC_UNUSED)
{
   /* Declare local variables. */
   CDKMENU *menu= (CDKMENU *)object;
   int menuList= menu->currentTitle;
   int submenuList= menu->currentSubtitle;
   int selection= ((menuList * 100) + submenuList);
   char *mesg[20], *name, temp[100];

   /* Create the help title. */
   name = chtype2Char (menu->sublist[menuList][submenuList]);
   stripWhiteSpace (vBOTH, name);
   sprintf (temp, "<C></R>Help<!R> </U>%s<!U>", name);
   mesg[0] = copyChar (temp);
   freeChar (name);

   /* Set the default value for the message. */
   mesg[1] = "<C>No help defined for this menu.";

   /* Given the current menu item, create a message. */
   if (selection == 0)
   {
      mesg[1] = "<C>This reads a new rolodex RC file.";
   }
   else if (selection == 1)
   {
      mesg[1] = "<C>This saves the current group information in the default RC file.";
   }
   else if (selection == 2)
   {
      mesg[1] = "<C>This saves the current group information in a new RC file.";
   }
   else if (selection == 3)
   {
      mesg[1] = "<C>This exits this program.";
   }
   else if (selection == 100)
   {
      mesg[1] = "<C>This creates a new rolodex group.";
   }
   else if (selection == 101)
   {
      mesg[1] = "<C>This opens a rolodex group.";
   }
   else if (selection == 102)
   {
      mesg[1] = "<C>This deletes a rolodex group.";
   }
   else if (selection == 200)
   {
      mesg[1] = "<C>This prints out selected groups phone numbers.";
   }
   else if (selection == 300)
   {
      mesg[1] = "<C>This gives a little history on this program.";
   }
   else if (selection == 301)
   {
      mesg[1] = "<C>This provides information about the rolodex.";
   }

   /* Pop up the message. */
   popupLabel (ScreenOf(menu), mesg, 2);

   /* Clean up. */
   freeChar (mesg[0]);
}

/*
 * This is a callback to the group list scrolling list.
 */
void groupInfoCB (EObjectType cdkType GCC_UNUSED, void *object, void *clientData, chtype key GCC_UNUSED)
{
   /* Declare local variables. */
   CDKSCROLL *scrollp= (CDKSCROLL *)object;
   SRolodex *groupList= (SRolodex *)clientData;
   int selection= scrollp->currentItem;
   char *mesg[5], temp[100];

   /* Create the message to be displayed. */
   mesg[0] = "<C></U>Detailed Group Information.";

   sprintf (temp, "</R>Group Name         <!R> %s", groupList[selection].name);
   mesg[1] = copyChar (temp);

   sprintf (temp, "</R>Group Description  <!R> %s", groupList[selection].desc);
   mesg[2] = copyChar (temp);

   sprintf (temp, "</R>Group Database File<!R> %s", groupList[selection].dbm);
   mesg[3] = copyChar (temp);

   /* Display the message. */
   popupLabel (ScreenOf(scrollp), mesg, 4);

   /* Clean up. */
   freeChar (mesg[1]);
   freeChar (mesg[2]);
   freeChar (mesg[3]);
}
