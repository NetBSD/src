/* RTC toolkit version 1.08b, copyright 1995, erik vogan

   all rights and priveledges to this code hereby donated
   to the ALICE group for use in NetBSD.  see their copy-
   right for further restrictions, etc, blah, blah.

   C interface header - this does nothing more interesting
   than to provide the function delcarations for the C
   interfaces to the assembly language routines.
   written 2/8/95	emv */

void readPram    (char *addr, int loc, int len);
void writePram   (char *addr, int loc, int len);
void readExtPram (char *addr, int loc, int len);
void writeExtPram(char *addr, int loc, int len);

/* where addr is a pointer to the data to write/read from/to PRAM (bytes)
   and loc is the PRAM address to read/write, and len is the number of
   contisecutive bytes to write/read.

	possible values for:		loc		len
	read/writePram		     $00-$13 **	      $01-10 **
	read/writeExtPram	     $00-$ff	      $00-ff

	** - due to the way the PRAM is set up, $00-0f must be read with one
	     command, and $10-$13 must be read with another.  Please, do not
	     attempt to read across the $0f/$10 boundary!!  You have been
	     warned!!
*/

unsigned long	getPramTime(void);
void 		setPramTime(unsigned long time);

/* obviously these routines are used to get/set the PRAM time, which I might
   mention is in seconds since 1904!!! */
