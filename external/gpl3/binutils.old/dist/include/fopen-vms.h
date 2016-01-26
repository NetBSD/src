/* Macros for the 'type' part of an fopen, freopen or fdopen. 

	<Read|Write>[Update]<Binary file|text file>

   This version is for VMS systems, where text and binary files are
   different.
   This file is designed for inclusion by host-dependent .h files.  No
   user application should include it directly, since that would make
   the application unable to be configured for both "same" and "binary"
   variant systems.  */

#define FOPEN_RB	"rb,rfm=udf,rat=none"
#define FOPEN_WB 	"wb,rfm=udf,rat=none"
#define FOPEN_AB 	"ab,rfm=udf,rat=none"
#define FOPEN_RUB 	"r+b,rfm=udf,rat=none"
#define FOPEN_WUB 	"w+b,rfm=udf,rat=none"
#define FOPEN_AUB 	"a+b,rfm=udf,rat=none"

#define FOPEN_RT	"r"
#define FOPEN_WT 	"w"
#define FOPEN_AT 	"a"
#define FOPEN_RUT 	"r+"
#define FOPEN_WUT 	"w+"
#define FOPEN_AUT 	"a+"
