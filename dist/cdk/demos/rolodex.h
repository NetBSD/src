#include <cdk.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXGROUPS	100
#define GLINETYPECOUNT	9

/*
 * Declare some global variables.
 */
char *GCurrentGroup	= 0;
char *GRCFile		= 0;
char *GDBMDir		= 0;
int GGroupModified	= FALSE;
int GPhoneListModified	= FALSE;
char *GLineType[]	= {"Voice", "Cell", "Pager", "First FAX", "Second FAX", "Third FAX", "First Data Line", "Second Data Line", "Third Data Line"};

/*
 * Create some definitions.
 */
typedef enum {vUnknown = -1, vVoice = 0, vCell, vPager, vFAX1, vFAX2, vFAX3, vData1, vData2, vData3} ELineType;

/*
 * Define the group record structure.
 */
struct _group_st {
	char *name;
	char *desc;
	char *dbm;
};
typedef struct _group_st SRolodex;

/*
 * Define a phone record structure;
 */
struct _record_st {
	char *name;
	ELineType lineType;
	char *phoneNumber;
	char *address;
	char *city;
	char *province;
	char *postalCode;
	char *desc;
};
typedef struct _record_st SPhoneRecord;

struct _phone_data_st {
	SPhoneRecord record[MAX_ITEMS];
	int recordCount;
};
typedef struct _phone_data_st SPhoneData;

/*
 * Define the callback prototypes.
 */
BINDFN_PROTO(helpCB);
BINDFN_PROTO(groupInfoCB);
BINDFN_PROTO(insertPhoneEntryCB);
BINDFN_PROTO(deletePhoneEntryCB);
BINDFN_PROTO(phoneEntryHelpCB);
int entryPreProcessCB (EObjectType cdkType, void *object, void *clientData, chtype input);

/*
 * These functions use/modify the rolodex RC file.
 */
int readRCFile (char *filename, SRolodex *groupInfo);
int openNewRCFile (CDKSCREEN *screen, SRolodex *groups, int groupCount);
int writeRCFile (CDKSCREEN *screen, char *file, SRolodex *groups, int count);
int writeRCFileAs (CDKSCREEN *screen, SRolodex *groups, int count);

/*
 * These functions use/modify rolodex phone groups.
 */
int addRolodexGroup (CDKSCREEN *screen, SRolodex *groups, int count);
int deleteRolodexGroup (CDKSCREEN *screen, SRolodex *groups, int count);
int pickRolodexGroup (CDKSCREEN *screen, char *title, SRolodex *groups, int count);
void useRolodexGroup (CDKSCREEN *screen, char *name, char *desc, char *dbm);

/*
 * These functions display misc information about the rolodex program.
 */
void displayRolodexStats (CDKSCREEN *screen, int groupCount);
void aboutCdkRolodex (CDKSCREEN *screen);
void displayPhoneInfo (CDKSCREEN *screen, SPhoneRecord record);

/*
 * These functions use/modify phone data lists.
 */
int readPhoneDataFile (char *filename, SPhoneData *record);
int savePhoneDataFile (char *filename, SPhoneData *record);
int addPhoneRecord (CDKSCREEN *screen, SPhoneData *phoneData);
int deletePhoneRecord (CDKSCREEN *screen, SPhoneData *phoneData);
int getLargePhoneRecord (CDKSCREEN *screen, SPhoneRecord *phoneRecord);
int getSmallPhoneRecord (CDKSCREEN *screen, SPhoneRecord *phoneRecord);

/*
 * These functions allow us to print out phone numbers.
 */
void printGroupNumbers (CDKSCREEN *screen, SRolodex *groups, int count);
int printGroup (SRolodex groupRecord, char *filename, char *printer);
