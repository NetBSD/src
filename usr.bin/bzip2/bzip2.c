/*	$NetBSD: bzip2.c,v 1.5 2001/02/05 01:35:45 christos Exp $	*/

/*-----------------------------------------------------------*/
/*--- A block-sorting, lossless compressor        bzip2.c ---*/
/*-----------------------------------------------------------*/

/*--
  This file is a part of bzip2 and/or libbzip2, a program and
  library for lossless, block-sorting data compression.

  Copyright (C) 1996-1998 Julian R Seward.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. The origin of this software must not be misrepresented; you must 
     not claim that you wrote the original software.  If you use this 
     software in a product, an acknowledgment in the product 
     documentation would be appreciated but is not required.

  3. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  4. The name of the author may not be used to endorse or promote 
     products derived from this software without specific prior written 
     permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Julian Seward, Guildford, Surrey, UK.
  jseward@acm.org
  bzip2/libbzip2 version 0.9.0b of 9 Sept 1998

  This program is based on (at least) the work of:
     Mike Burrows
     David Wheeler
     Peter Fenwick
     Alistair Moffat
     Radford Neal
     Ian H. Witten
     Robert Sedgewick
     Jon L. Bentley

  For more information on these sources, see the manual.
--*/


/*----------------------------------------------------*/
/*--- IMPORTANT                                    ---*/
/*----------------------------------------------------*/

/*--
   WARNING:
      This program and library (attempts to) compress data by 
      performing several non-trivial transformations on it.  
      Unless you are 100% familiar with *all* the algorithms 
      contained herein, and with the consequences of modifying them, 
      you should NOT meddle with the compression or decompression 
      machinery.  Incorrect changes can and very likely *will* 
      lead to disasterous loss of data.

   DISCLAIMER:
      I TAKE NO RESPONSIBILITY FOR ANY LOSS OF DATA ARISING FROM THE
      USE OF THIS PROGRAM, HOWSOEVER CAUSED.

      Every compression of a file implies an assumption that the
      compressed file can be decompressed to reproduce the original.
      Great efforts in design, coding and testing have been made to
      ensure that this program works correctly.  However, the
      complexity of the algorithms, and, in particular, the presence
      of various special cases in the code which occur with very low
      but non-zero probability make it impossible to rule out the
      possibility of bugs remaining in the program.  DO NOT COMPRESS
      ANY DATA WITH THIS PROGRAM AND/OR LIBRARY UNLESS YOU ARE PREPARED 
      TO ACCEPT THE POSSIBILITY, HOWEVER SMALL, THAT THE DATA WILL 
      NOT BE RECOVERABLE.

      That is not to say this program is inherently unreliable.
      Indeed, I very much hope the opposite is true.  bzip2/libbzip2
      has been carefully constructed and extensively tested.

   PATENTS:
      To the best of my knowledge, bzip2/libbzip2 does not use any 
      patented algorithms.  However, I do not have the resources 
      available to carry out a full patent search.  Therefore I cannot 
      give any guarantee of the above statement.
--*/



/*----------------------------------------------------*/
/*--- and now for something much more pleasant :-) ---*/
/*----------------------------------------------------*/

/*---------------------------------------------*/
/*--
  Place a 1 beside your platform, and 0 elsewhere.
--*/

/*--
  Generic 32-bit Unix.
  Also works on 64-bit Unix boxes.
--*/
#define BZ_UNIX      1

/*--
  Win32, as seen by Jacob Navia's excellent
  port of (Chris Fraser & David Hanson)'s excellent
  lcc compiler.
--*/
#define BZ_LCCWIN32  0

#ifdef _WIN32
#define BZ_LCCWIN32 1
#define BZ_UNIX 0
#endif


/*---------------------------------------------*/
/*--
  Some stuff for all platforms.
--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include "bzlib.h"

#define ERROR_IF_EOF(i)       { if ((i) == EOF)  ioError(); }
#define ERROR_IF_NOT_ZERO(i)  { if ((i) != 0)    ioError(); }
#define ERROR_IF_MINUS_ONE(i) { if ((i) == (-1)) ioError(); }


/*---------------------------------------------*/
/*--
   Platform-specific stuff.
--*/

#if BZ_UNIX
#   include <sys/types.h>
#   include <utime.h>
#   include <unistd.h>
#   include <sys/stat.h>
#   include <sys/times.h>

#   define PATH_SEP    '/'
#   define MY_LSTAT    lstat
#   define MY_S_IFREG  S_ISREG
#   define MY_STAT     stat

#   define APPEND_FILESPEC(root, name) \
      root=snocString((root), (name))

#   define SET_BINARY_MODE(fd) /**/

#   ifdef __GNUC__
#      define NORETURN __attribute__ ((noreturn))
#   else
#      define NORETURN /**/
#   endif
#endif



#if BZ_LCCWIN32
#   include <io.h>
#   include <fcntl.h>
#   include <sys\stat.h>

#   define NORETURN       /**/
#   define PATH_SEP       '\\'
#   define MY_LSTAT       _stat
#   define MY_STAT        _stat
#   define MY_S_IFREG(x)  ((x) & _S_IFREG)

#   if 0
   /*-- lcc-win32 seems to expand wildcards itself --*/
#   define APPEND_FILESPEC(root, spec)                \
      do {                                            \
         if ((spec)[0] == '-') {                      \
            root = snocString((root), (spec));        \
         } else {                                     \
            struct _finddata_t c_file;                \
            long hFile;                               \
            hFile = _findfirst((spec), &c_file);      \
            if ( hFile == -1L ) {                     \
               root = snocString ((root), (spec));    \
            } else {                                  \
               int anInt = 0;                         \
               while ( anInt == 0 ) {                 \
                  root = snocString((root),           \
                            &c_file.name[0]);         \
                  anInt = _findnext(hFile, &c_file);  \
               }                                      \
            }                                         \
         }                                            \
      } while ( 0 )
#   else
#   define APPEND_FILESPEC(root, name)                \
      root = snocString ((root), (name))
#   endif

#   define SET_BINARY_MODE(fd)                        \
      do {                                            \
         int retVal = setmode ( fileno ( fd ),        \
                               O_BINARY );            \
         ERROR_IF_MINUS_ONE ( retVal );               \
      } while ( 0 )

#endif


/*---------------------------------------------*/
/*--
  Some more stuff for all platforms :-)
--*/

typedef char            Char;
typedef unsigned char   Bool;
typedef unsigned char   UChar;
typedef int             Int32;
typedef unsigned int    UInt32;
typedef short           Int16;
typedef unsigned short  UInt16;
                                       
#define True  ((Bool)1)
#define False ((Bool)0)

/*--
  IntNative is your platform's `native' int size.
  Only here to avoid probs with 64-bit platforms.
--*/
typedef int IntNative;


/*---------------------------------------------------*/
/*--- Misc (file handling) data decls             ---*/
/*---------------------------------------------------*/

Int32   verbosity;
Bool    keepInputFiles, smallMode;
Bool    forceOverwrite, testFailsExist;
Int32   numFileNames, numFilesProcessed, blockSize100k;


/*-- source modes; F==file, I==stdin, O==stdout --*/
#define SM_I2O           1
#define SM_F2O           2
#define SM_F2F           3

/*-- operation modes --*/
#define OM_Z             1
#define OM_UNZ           2
#define OM_TEST          3

Int32   opMode;
Int32   srcMode;

#define FILE_NAME_LEN 1034

Int32   longestFileName;
Char    inName[FILE_NAME_LEN];
Char    outName[FILE_NAME_LEN];
Char    *progName;
Char    progNameReally[FILE_NAME_LEN];
FILE    *outputHandleJustInCase;
Int32   workFactor;


typedef
   struct zzzz {
      Char        *name;
      struct zzzz *link;
   }
   Cell;


void    panic                 ( Char* )   NORETURN;
void    ioError               ( void )    NORETURN;
void    outOfMemory           ( void )    NORETURN;
void    blockOverrun          ( void )    NORETURN;
void    badBlockHeader        ( void )    NORETURN;
void    badBGLengths          ( void )    NORETURN;
void    crcError              ( void )    NORETURN;
void    bitStreamEOF          ( void )    NORETURN;
void    cleanUpAndFail        ( Int32 )   NORETURN;
void    compressedStreamEOF   ( void )    NORETURN;

void    copyFileName ( Char*, Char* );
void*   myMalloc ( Int32 );


void	compressStream ( FILE *, FILE * );
Bool	uncompressStream ( FILE *, FILE * );
Bool	testStream ( FILE * );
void	cadvise ( void );
void	showFileNames ( void );
void	mySignalCatcher ( IntNative );
void	mySIGSEGVorSIGBUScatcher ( IntNative );
void	pad ( Char * );
Bool	fileExists ( Char * );
Bool	notAStandardFile ( Char *, IntNative );
void	copyDatePermissionsAndOwner ( Char *, Char * );
void	setInterimPermissions ( Char * );
Bool	endsInBz2 ( Char * );
Bool	containsDubiousChars ( Char * );
void	compress ( Char * );
void	uncompress ( Char * );
void	testf ( Char * );
void	license ( void );
void	usage ( Char * );
Cell	*mkCell ( void );
Cell	*snocString ( Cell *, Char * );
Bool	myfeof ( FILE* f );
IntNative	main ( IntNative, Char *[] );


/*---------------------------------------------------*/
/*--- Processing of complete files and streams    ---*/
/*---------------------------------------------------*/

/*---------------------------------------------*/
Bool myfeof ( FILE* f )
{
   Int32 c = fgetc ( f );
   if (c == EOF) return True;
   ungetc ( c, f );
   return False;
}


/*---------------------------------------------*/
void compressStream ( FILE *stream, FILE *zStream )
{
   BZFILE* bzf = NULL;
   UChar   ibuf[5000];
   Int32   nIbuf;
   UInt32  nbytes_in, nbytes_out;
   Int32   bzerr, bzerr_dummy, ret;

   SET_BINARY_MODE(stream);
   SET_BINARY_MODE(zStream);

   if (ferror(stream)) goto errhandler_io;
   if (ferror(zStream)) goto errhandler_io;

   bzf = bzWriteOpen ( &bzerr, zStream, 
                       blockSize100k, verbosity, workFactor );   
   if (bzerr != BZ_OK) goto errhandler;

   if (verbosity >= 2) fprintf ( stderr, "\n" );

   while (True) {

      if (myfeof(stream)) break;
      nIbuf = fread ( ibuf, sizeof(UChar), 5000, stream );
      if (ferror(stream)) goto errhandler_io;
      if (nIbuf > 0) bzWrite ( &bzerr, bzf, (void*)ibuf, nIbuf );
      if (bzerr != BZ_OK) goto errhandler;

   }

   bzWriteClose ( &bzerr, bzf, 0, &nbytes_in, &nbytes_out );
   if (bzerr != BZ_OK) goto errhandler;

   if (ferror(zStream)) goto errhandler_io;
   ret = fflush ( zStream );
   if (ret == EOF) goto errhandler_io;
   if (zStream != stdout) {
      ret = fclose ( zStream );
      if (ret == EOF) goto errhandler_io;
   }
   if (ferror(stream)) goto errhandler_io;
   ret = fclose ( stream );
   if (ret == EOF) goto errhandler_io;

   if (nbytes_in == 0) nbytes_in = 1;

   if (verbosity >= 1)
      fprintf ( stderr, "%6.3f:1, %6.3f bits/byte, "
                        "%5.2f%% saved, %d in, %d out.\n",
                (float)nbytes_in / (float)nbytes_out,
                (8.0 * (float)nbytes_out) / (float)nbytes_in,
                100.0 * (1.0 - (float)nbytes_out / (float)nbytes_in),
                nbytes_in,
                nbytes_out
              );

   return;

   errhandler:
   bzWriteClose ( &bzerr_dummy, bzf, 1, &nbytes_in, &nbytes_out );
   switch (bzerr) {
      case BZ_MEM_ERROR:
         outOfMemory ();
      case BZ_IO_ERROR:
         errhandler_io:
         ioError(); break;
      default:
         panic ( "compress:unexpected error" );
   }

   panic ( "compress:end" );
   /*notreached*/
}



/*---------------------------------------------*/
Bool uncompressStream ( FILE *zStream, FILE *stream )
{
   BZFILE* bzf = NULL;
   Int32   bzerr, bzerr_dummy, ret, nread, streamNo, i;
   UChar   obuf[5000];
   UChar   unused[BZ_MAX_UNUSED];
   Int32   nUnused;
   UChar*  unusedTmp;

   nUnused = 0;
   streamNo = 0;

   SET_BINARY_MODE(stream);
   SET_BINARY_MODE(zStream);

   if (ferror(stream)) goto errhandler_io;
   if (ferror(zStream)) goto errhandler_io;

   while (True) {

      bzf = bzReadOpen ( 
               &bzerr, zStream, verbosity, 
               (int)smallMode, unused, nUnused
            );
      if (bzf == NULL || bzerr != BZ_OK) goto errhandler;
      streamNo++;

      while (bzerr == BZ_OK) {
         nread = bzRead ( &bzerr, bzf, obuf, 5000 );
         if (bzerr == BZ_DATA_ERROR_MAGIC) goto errhandler;
         if ((bzerr == BZ_OK || bzerr == BZ_STREAM_END) && nread > 0)
            fwrite ( obuf, sizeof(UChar), nread, stream );
         if (ferror(stream)) goto errhandler_io;
      }
      if (bzerr != BZ_STREAM_END) goto errhandler;

      bzReadGetUnused ( &bzerr, bzf, (void**)(&unusedTmp), &nUnused );
      if (bzerr != BZ_OK) panic ( "decompress:bzReadGetUnused" );

      for (i = 0; i < nUnused; i++) unused[i] = unusedTmp[i];

      bzReadClose ( &bzerr, bzf );
      if (bzerr != BZ_OK) panic ( "decompress:bzReadGetUnused" );

      if (nUnused == 0 && myfeof(zStream)) break;

   }

   if (ferror(zStream)) goto errhandler_io;
   ret = fclose ( zStream );
   if (ret == EOF) goto errhandler_io;

   if (ferror(stream)) goto errhandler_io;
   ret = fflush ( stream );
   if (ret != 0) goto errhandler_io;
   if (stream != stdout) {
      ret = fclose ( stream );
      if (ret == EOF) goto errhandler_io;
   }
   if (verbosity >= 2) fprintf ( stderr, "\n    " );
   return True;

   errhandler:
   bzReadClose ( &bzerr_dummy, bzf );
   switch (bzerr) {
      case BZ_IO_ERROR:
         errhandler_io:
         ioError(); break;
      case BZ_DATA_ERROR:
         crcError();
      case BZ_MEM_ERROR:
         outOfMemory();
      case BZ_UNEXPECTED_EOF:
         compressedStreamEOF();
      case BZ_DATA_ERROR_MAGIC:
         if (streamNo == 1) {
            return False;
         } else {
            fprintf ( stderr, 
                      "\n%s: %s: trailing garbage after EOF ignored\n",
                      progName, inName );
            return True;       
         }
      default:
         panic ( "decompress:unexpected error" );
   }

   panic ( "decompress:end" );
   return True; /*notreached*/
}


/*---------------------------------------------*/
Bool testStream ( FILE *zStream )
{
   BZFILE* bzf = NULL;
   Int32   bzerr, bzerr_dummy, ret, nread, streamNo, i;
   UChar   obuf[5000];
   UChar   unused[BZ_MAX_UNUSED];
   Int32   nUnused;
   UChar*  unusedTmp;

   nUnused = 0;
   streamNo = 0;

   SET_BINARY_MODE(zStream);
   if (ferror(zStream)) goto errhandler_io;

   while (True) {

      bzf = bzReadOpen ( 
               &bzerr, zStream, verbosity, 
               (int)smallMode, unused, nUnused
            );
      if (bzf == NULL || bzerr != BZ_OK) goto errhandler;
      streamNo++;

      while (bzerr == BZ_OK) {
         nread = bzRead ( &bzerr, bzf, obuf, 5000 );
         if (bzerr == BZ_DATA_ERROR_MAGIC) goto errhandler;
      }
      if (bzerr != BZ_STREAM_END) goto errhandler;

      bzReadGetUnused ( &bzerr, bzf, (void**)(&unusedTmp), &nUnused );
      if (bzerr != BZ_OK) panic ( "test:bzReadGetUnused" );

      for (i = 0; i < nUnused; i++) unused[i] = unusedTmp[i];

      bzReadClose ( &bzerr, bzf );
      if (bzerr != BZ_OK) panic ( "test:bzReadGetUnused" );
      if (nUnused == 0 && myfeof(zStream)) break;

   }

   if (ferror(zStream)) goto errhandler_io;
   ret = fclose ( zStream );
   if (ret == EOF) goto errhandler_io;

   if (verbosity >= 2) fprintf ( stderr, "\n    " );
   return True;

   errhandler:
   bzReadClose ( &bzerr_dummy, bzf );
   switch (bzerr) {
      case BZ_IO_ERROR:
         errhandler_io:
         ioError(); break;
      case BZ_DATA_ERROR:
         fprintf ( stderr,
                   "\n%s: data integrity (CRC) error in data\n",
                   inName );
         return False;
      case BZ_MEM_ERROR:
         outOfMemory();
      case BZ_UNEXPECTED_EOF:
         fprintf ( stderr,
                   "\n%s: file ends unexpectedly\n",
                   inName );
         return False;
      case BZ_DATA_ERROR_MAGIC:
         if (streamNo == 1) {
          fprintf ( stderr, 
                    "\n%s: bad magic number (ie, not created by bzip2)\n",
                    inName );
            return False;
         } else {
            fprintf ( stderr, 
                      "\n%s: %s: trailing garbage after EOF ignored\n",
                      progName, inName );
            return True;       
         }
      default:
         panic ( "test:unexpected error" );
   }

   panic ( "test:end" );
   return True; /*notreached*/
}


/*---------------------------------------------------*/
/*--- Error [non-] handling grunge                ---*/
/*---------------------------------------------------*/

/*---------------------------------------------*/
void cadvise ( void )
{
   fprintf (
      stderr,
      "\nIt is possible that the compressed file(s) have become corrupted.\n"
        "You can use the -tvv option to test integrity of such files.\n\n"
        "You can use the `bzip2recover' program to *attempt* to recover\n"
        "data from undamaged sections of corrupted files.\n\n"
    );
}


/*---------------------------------------------*/
void showFileNames ( void )
{
   fprintf (
      stderr,
      "\tInput file = %s, output file = %s\n",
      inName, outName 
   );
}


/*---------------------------------------------*/
void cleanUpAndFail ( Int32 ec )
{
   IntNative retVal;

   if ( srcMode == SM_F2F && opMode != OM_TEST ) {
      fprintf ( stderr, "%s: Deleting output file %s, if it exists.\n",
                progName, outName );
      if (outputHandleJustInCase != NULL)
         fclose ( outputHandleJustInCase );
      retVal = remove ( outName );
      if (retVal != 0)
         fprintf ( stderr,
                   "%s: WARNING: deletion of output file (apparently) failed.\n",
                   progName );
   }
   if (numFileNames > 0 && numFilesProcessed < numFileNames) {
      fprintf ( stderr, 
                "%s: WARNING: some files have not been processed:\n"
                "\t%d specified on command line, %d not processed yet.\n\n",
                progName, numFileNames, 
                          numFileNames - numFilesProcessed );
   }
   exit ( ec );
}


/*---------------------------------------------*/
void panic ( Char* s )
{
   fprintf ( stderr,
             "\n%s: PANIC -- internal consistency error:\n"
             "\t%s\n"
             "\tThis is a BUG.  Please report it to me at:\n"
             "\tjseward@acm.org\n",
             progName, s );
   showFileNames();
   cleanUpAndFail( 3 );
}


/*---------------------------------------------*/
void crcError ()
{
   fprintf ( stderr,
             "\n%s: Data integrity error when decompressing.\n",
             progName );
   showFileNames();
   cadvise();
   cleanUpAndFail( 2 );
}


/*---------------------------------------------*/
void compressedStreamEOF ( void )
{
   fprintf ( stderr,
             "\n%s: Compressed file ends unexpectedly;\n\t"
             "perhaps it is corrupted?  *Possible* reason follows.\n",
             progName );
   perror ( progName );
   showFileNames();
   cadvise();
   cleanUpAndFail( 2 );
}


/*---------------------------------------------*/
void ioError ( )
{
   fprintf ( stderr,
             "\n%s: I/O or other error, bailing out.  Possible reason follows.\n",
             progName );
   perror ( progName );
   showFileNames();
   cleanUpAndFail( 1 );
}


/*---------------------------------------------*/
void mySignalCatcher ( IntNative n )
{
   fprintf ( stderr,
             "\n%s: Control-C (or similar) caught, quitting.\n",
             progName );
   cleanUpAndFail(1);
}


/*---------------------------------------------*/
void mySIGSEGVorSIGBUScatcher ( IntNative n )
{
   if (opMode == OM_Z)
      fprintf ( stderr,
                "\n%s: Caught a SIGSEGV or SIGBUS whilst compressing,\n"
                "\twhich probably indicates a bug in bzip2.  Please\n"
                "\treport it to me at: jseward@acm.org\n",
                progName );
      else
      fprintf ( stderr,
                "\n%s: Caught a SIGSEGV or SIGBUS whilst decompressing,\n"
                "\twhich probably indicates that the compressed data\n"
                "\tis corrupted.\n",
                progName );

   showFileNames();
   if (opMode == OM_Z)
      cleanUpAndFail( 3 ); else
      { cadvise(); cleanUpAndFail( 2 ); }
}


/*---------------------------------------------*/
void outOfMemory ( void )
{
   fprintf ( stderr,
             "\n%s: couldn't allocate enough memory\n",
             progName );
   showFileNames();
   cleanUpAndFail(1);
}


/*---------------------------------------------------*/
/*--- The main driver machinery                   ---*/
/*---------------------------------------------------*/

/*---------------------------------------------*/
void pad ( Char *s )
{
   Int32 i;
   if ( (Int32)strlen(s) >= longestFileName ) return;
   for (i = 1; i <= longestFileName - (Int32)strlen(s); i++)
      fprintf ( stderr, " " );
}


/*---------------------------------------------*/
void copyFileName ( Char* to, Char* from ) 
{
   if ( strlen(from) > FILE_NAME_LEN-10 )  {
      fprintf (
         stderr,
         "bzip2: file name\n`%s'\nis suspiciously (> 1024 chars) long.\n"
         "Try using a reasonable file name instead.  Sorry! :)\n",
         from
      );
      exit(1);
   }

  strncpy(to,from,FILE_NAME_LEN-10);
  to[FILE_NAME_LEN-10]='\0';
}


/*---------------------------------------------*/
Bool fileExists ( Char* name )
{
   FILE *tmp   = fopen ( name, "rb" );
   Bool exists = (tmp != NULL);
   if (tmp != NULL) fclose ( tmp );
   return exists;
}


/*---------------------------------------------*/
/*--
  if in doubt, return True
--*/
Bool notAStandardFile ( Char* name , IntNative allowSymbolicLinks )
{
   IntNative      i;
   struct MY_STAT statBuf;

   i = allowSymbolicLinks ? MY_STAT ( name, &statBuf ) :
                            MY_LSTAT ( name, &statBuf );
   if (i != 0) return True;
   if (MY_S_IFREG(statBuf.st_mode)) return False;

   return True;
}


/*---------------------------------------------*/
void copyDatePermissionsAndOwner ( Char *srcName, Char *dstName )
{
#if BZ_UNIX
   IntNative      retVal;
   struct MY_STAT statBuf;
   struct utimbuf uTimBuf;

   retVal = MY_LSTAT ( srcName, &statBuf );
   ERROR_IF_NOT_ZERO ( retVal );
   uTimBuf.actime = statBuf.st_atime;
   uTimBuf.modtime = statBuf.st_mtime;

   retVal = chmod ( dstName, statBuf.st_mode );
   ERROR_IF_NOT_ZERO ( retVal );
   /* Not sure if this is really portable or not.  Causes 
      problems on my x86-Linux Redhat 5.0 box.  Decided
      to omit it from 0.9.0.  JRS, 27 June 98.  If you 
      understand Unix file semantics and portability issues
      well enough to fix this properly, drop me a line
      at jseward@acm.org.
   retVal = chown ( dstName, statBuf.st_uid, statBuf.st_gid );
   ERROR_IF_NOT_ZERO ( retVal );
   */
   retVal = utime ( dstName, &uTimBuf );
   ERROR_IF_NOT_ZERO ( retVal );
#endif
}


/*---------------------------------------------*/
void setInterimPermissions ( Char *dstName )
{
#if BZ_UNIX
   IntNative      retVal;
   retVal = chmod ( dstName, S_IRUSR | S_IWUSR );
   ERROR_IF_NOT_ZERO ( retVal );
#endif
}



/*---------------------------------------------*/
Bool endsInBz2 ( Char* name )
{
   Int32 n = strlen ( name );
   if (n <= 4) return False;
   return
      (name[n-4] == '.' &&
       name[n-3] == 'b' &&
       name[n-2] == 'z' &&
       name[n-1] == '2');
}


/*---------------------------------------------*/
Bool containsDubiousChars ( Char* name )
{
   Bool cdc = False;
   for (; *name != '\0'; name++)
      if (*name == '?' || *name == '*') cdc = True;
   return cdc;
}


/*---------------------------------------------*/
void compress ( Char *name )
{
   FILE *inStr;
   FILE *outStr;

   if (name == NULL && srcMode != SM_I2O)
      panic ( "compress: bad modes\n" );

   switch (srcMode) {
      case SM_I2O: copyFileName ( inName, "(stdin)" );
                   copyFileName ( outName, "(stdout)" ); break;
      case SM_F2F: copyFileName ( inName, name );
                   copyFileName ( outName, name );
                   strcat ( outName, ".bz2" ); break;
      case SM_F2O: copyFileName ( inName, name );
                   copyFileName ( outName, "(stdout)" ); break;
   }

   if ( srcMode != SM_I2O && containsDubiousChars ( inName ) ) {
      fprintf ( stderr, "%s: There are no files matching `%s'.\n",
      progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && !fileExists ( inName ) ) {
      fprintf ( stderr, "%s: Input file %s doesn't exist, skipping.\n",
                progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && endsInBz2 ( inName )) {
      fprintf ( stderr, "%s: Input file name %s ends in `.bz2', skipping.\n",
                progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && notAStandardFile ( inName, False )) {
      fprintf ( stderr, "%s: Input file %s is not a normal file, skipping.\n",
                progName, inName );
      return;
   }
   if ( srcMode == SM_F2F && !forceOverwrite && fileExists ( outName ) ) {
      fprintf ( stderr, "%s: Output file %s already exists, skipping.\n",
                progName, outName );
      return;
   }

   switch ( srcMode ) {

      case SM_I2O:
         inStr = stdin;
         outStr = stdout;
         if ( isatty ( fileno ( stdout ) ) ) {
            fprintf ( stderr,
                      "%s: I won't write compressed data to a terminal.\n",
                      progName );
            fprintf ( stderr, "%s: For help, type: `%s --help'.\n",
                              progName, progName );
            return;
         };
         break;

      case SM_F2O:
         inStr = fopen ( inName, "rb" );
         outStr = stdout;
         if ( isatty ( fileno ( stdout ) ) ) {
            fprintf ( stderr,
                      "%s: I won't write compressed data to a terminal.\n",
                      progName );
            fprintf ( stderr, "%s: For help, type: `%s --help'.\n",
                              progName, progName );
            return;
         };
         if ( inStr == NULL ) {
            fprintf ( stderr, "%s: Can't open input file %s, skipping.\n",
                      progName, inName );
            return;
         };
         break;

      case SM_F2F:
         inStr = fopen ( inName, "rb" );
         outStr = fopen ( outName, "wb" );
         if ( outStr == NULL) {
            fprintf ( stderr, "%s: Can't create output file %s, skipping.\n",
                      progName, outName );
            return;
         }
         if ( inStr == NULL ) {
            fprintf ( stderr, "%s: Can't open input file %s, skipping.\n",
                      progName, inName );
            return;
         };
         setInterimPermissions ( outName );
         break;

      default:
         panic ( "compress: bad srcMode" );
         break;
   }

   if (verbosity >= 1) {
      fprintf ( stderr,  "  %s: ", inName );
      pad ( inName );
      fflush ( stderr );
   }

   /*--- Now the input and output handles are sane.  Do the Biz. ---*/
   outputHandleJustInCase = outStr;
   compressStream ( inStr, outStr );
   outputHandleJustInCase = NULL;

   /*--- If there was an I/O error, we won't get here. ---*/
   if ( srcMode == SM_F2F ) {
      copyDatePermissionsAndOwner ( inName, outName );
      if ( !keepInputFiles ) {
         IntNative retVal = remove ( inName );
         ERROR_IF_NOT_ZERO ( retVal );
      }
   }
}


/*---------------------------------------------*/
void uncompress ( Char *name )
{
   FILE *inStr;
   FILE *outStr;
   Bool magicNumberOK;

   if (name == NULL && srcMode != SM_I2O)
      panic ( "uncompress: bad modes\n" );

   switch (srcMode) {
      case SM_I2O: copyFileName ( inName, "(stdin)" );
                   copyFileName ( outName, "(stdout)" ); break;
      case SM_F2F: copyFileName ( inName, name );
                   copyFileName ( outName, name );
                   if (endsInBz2 ( outName ))
                      outName [ strlen ( outName ) - 4 ] = '\0';
                   break;
      case SM_F2O: copyFileName ( inName, name );
                   copyFileName ( outName, "(stdout)" ); break;
   }

   if ( srcMode != SM_I2O && containsDubiousChars ( inName ) ) {
      fprintf ( stderr, "%s: There are no files matching `%s'.\n",
                progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && !fileExists ( inName ) ) {
      fprintf ( stderr, "%s: Input file %s doesn't exist, skipping.\n",
                progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && !endsInBz2 ( inName )) {
      fprintf ( stderr,
                "%s: Input file name %s doesn't end in `.bz2', skipping.\n",
                progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && notAStandardFile ( inName, srcMode == SM_F2O )) {
      fprintf ( stderr, "%s: Input file %s is not a normal file, skipping.\n",
                progName, inName );
      return;
   }
   if ( srcMode == SM_F2F && !forceOverwrite && fileExists ( outName ) ) {
      fprintf ( stderr, "%s: Output file %s already exists, skipping.\n",
                progName, outName );
      return;
   }

   switch ( srcMode ) {

      case SM_I2O:
         inStr = stdin;
         outStr = stdout;
         if ( isatty ( fileno ( stdin ) ) ) {
            fprintf ( stderr,
                      "%s: I won't read compressed data from a terminal.\n",
                      progName );
            fprintf ( stderr, "%s: For help, type: `%s --help'.\n",
                              progName, progName );
            return;
         };
         break;

      case SM_F2O:
         inStr = fopen ( inName, "rb" );
         outStr = stdout;
         if ( inStr == NULL ) {
            fprintf ( stderr, "%s: Can't open input file %s, skipping.\n",
                      progName, inName );
            return;
         };
         break;

      case SM_F2F:
         inStr = fopen ( inName, "rb" );
         outStr = fopen ( outName, "wb" );
         if ( outStr == NULL) {
            fprintf ( stderr, "%s: Can't create output file %s, skipping.\n",
                      progName, outName );
            return;
         }
         if ( inStr == NULL ) {
            fprintf ( stderr, "%s: Can't open input file %s, skipping.\n",
                      progName, inName );
            return;
         };
         setInterimPermissions ( outName );
         break;

      default:
         panic ( "uncompress: bad srcMode" );
         break;
   }

   if (verbosity >= 1) {
      fprintf ( stderr, "  %s: ", inName );
      pad ( inName );
      fflush ( stderr );
   }

   /*--- Now the input and output handles are sane.  Do the Biz. ---*/
   outputHandleJustInCase = outStr;
   magicNumberOK = uncompressStream ( inStr, outStr );
   outputHandleJustInCase = NULL;

   /*--- If there was an I/O error, we won't get here. ---*/
   if ( magicNumberOK ) {
      if ( srcMode == SM_F2F ) {
         copyDatePermissionsAndOwner ( inName, outName );
         if ( !keepInputFiles ) {
            IntNative retVal = remove ( inName );
            ERROR_IF_NOT_ZERO ( retVal );
         }
      }
   } else {
      if ( srcMode == SM_F2F ) {
         IntNative retVal = remove ( outName );
         ERROR_IF_NOT_ZERO ( retVal );
      }
   }

   if ( magicNumberOK ) {
      if (verbosity >= 1)
         fprintf ( stderr, "done\n" );
   } else {
      if (verbosity >= 1)
         fprintf ( stderr, "not a bzip2 file, skipping.\n" ); else
         fprintf ( stderr,
                   "%s: %s is not a bzip2 file, skipping.\n",
                   progName, inName );
   }

}


/*---------------------------------------------*/
void testf ( Char *name )
{
   FILE *inStr;
   Bool allOK;

   if (name == NULL && srcMode != SM_I2O)
      panic ( "testf: bad modes\n" );

   copyFileName ( outName, "(none)" );
   switch (srcMode) {
      case SM_I2O: copyFileName ( inName, "(stdin)" ); break;
      case SM_F2F: copyFileName ( inName, name ); break;
      case SM_F2O: copyFileName ( inName, name ); break;
   }

   if ( srcMode != SM_I2O && containsDubiousChars ( inName ) ) {
      fprintf ( stderr, "%s: There are no files matching `%s'.\n",
                progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && !fileExists ( inName ) ) {
      fprintf ( stderr, "%s: Input file %s doesn't exist, skipping.\n",
                progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && !endsInBz2 ( inName )) {
      fprintf ( stderr,
                "%s: Input file name %s doesn't end in `.bz2', skipping.\n",
                progName, inName );
      return;
   }
   if ( srcMode != SM_I2O && notAStandardFile ( inName, True )) {
      fprintf ( stderr, "%s: Input file %s is not a normal file, skipping.\n",
                progName, inName );
      return;
   }

   switch ( srcMode ) {

      case SM_I2O:
         if ( isatty ( fileno ( stdin ) ) ) {
            fprintf ( stderr,
                      "%s: I won't read compressed data from a terminal.\n",
                      progName );
            fprintf ( stderr, "%s: For help, type: `%s --help'.\n",
                              progName, progName );
            return;
         };
         inStr = stdin;
         break;

      case SM_F2O: case SM_F2F:
         inStr = fopen ( inName, "rb" );
         if ( inStr == NULL ) {
            fprintf ( stderr, "%s: Can't open input file %s, skipping.\n",
                      progName, inName );
            return;
         };
         break;

      default:
         panic ( "testf: bad srcMode" );
         break;
   }

   if (verbosity >= 1) {
      fprintf ( stderr, "  %s: ", inName );
      pad ( inName );
      fflush ( stderr );
   }

   /*--- Now the input handle is sane.  Do the Biz. ---*/
   allOK = testStream ( inStr );

   if (allOK && verbosity >= 1) fprintf ( stderr, "ok\n" );
   if (!allOK) testFailsExist = True;
}


/*---------------------------------------------*/
void license ( void )
{
   fprintf ( stderr,

    "bzip2, a block-sorting file compressor.  "
    "Version 0.9.0b, 9-Sept-98.\n"
    "   \n"
    "   Copyright (C) 1996, 1997, 1998 by Julian Seward.\n"
    "   \n"
    "   This program is free software; you can redistribute it and/or modify\n"
    "   it under the terms set out in the LICENSE file, which is included\n"
    "   in the bzip2-0.9.0b source distribution.\n"
    "   \n"
    "   This program is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   LICENSE file for more details.\n"
    "   \n"
   );
}


/*---------------------------------------------*/
void usage ( Char *fullProgName )
{
   fprintf (
      stderr,
      "bzip2, a block-sorting file compressor.  "
      "Version 0.9.0b, 9-Sept-98.\n"
      "\n   usage: %s [flags and input files in any order]\n"
      "\n"
      "   -h --help           print this message\n"
      "   -d --decompress     force decompression\n"
      "   -z --compress       force compression\n"
      "   -k --keep           keep (don't delete) input files\n"
      "   -f --force          overwrite existing output filess\n"
      "   -t --test           test compressed file integrity\n"
      "   -c --stdout         output to standard out\n"
      "   -v --verbose        be verbose (a 2nd -v gives more)\n"
      "   -L --license        display software version & license\n"
      "   -V --version        display software version & license\n"
      "   -s --small          use less memory (at most 2500k)\n"
      "   -1 .. -9            set block size to 100k .. 900k\n"
      "   --repetitive-fast   compress repetitive blocks faster\n"
      "   --repetitive-best   compress repetitive blocks better\n"
      "\n"
      "   If invoked as `bzip2', default action is to compress.\n"
      "              as `bunzip2',  default action is to decompress.\n"
      "              as `bz2cat', default action is to decompress to stdout.\n"
      "\n"
      "   If no file names are given, bzip2 compresses or decompresses\n"
      "   from standard input to standard output.  You can combine\n"
      "   short flags, so `-v -4' means the same as -v4 or -4v, &c.\n"
#if BZ_UNIX
      "\n"
#endif
      ,

      fullProgName
   );
}


/*---------------------------------------------*/
/*--
  All the garbage from here to main() is purely to
  implement a linked list of command-line arguments,
  into which main() copies argv[1 .. argc-1].

  The purpose of this ridiculous exercise is to
  facilitate the expansion of wildcard characters
  * and ? in filenames for halfwitted OSs like
  MSDOS, Windows 95 and NT.

  The actual Dirty Work is done by the platform-specific
  macro APPEND_FILESPEC.
--*/
/*---------------------------------------------*/
void *myMalloc ( Int32 n )
{
   void* p;

   p = malloc ( (size_t)n );
   if (p == NULL) outOfMemory ();
   return p;
}


/*---------------------------------------------*/
Cell *mkCell ( void )
{
   Cell *c;

   c = (Cell*) myMalloc ( sizeof ( Cell ) );
   c->name = NULL;
   c->link = NULL;
   return c;
}


/*---------------------------------------------*/
Cell *snocString ( Cell *root, Char *name )
{
   if (root == NULL) {
      Cell *tmp = mkCell();
      tmp->name = (Char*) myMalloc ( 5 + strlen(name) );
      strcpy ( tmp->name, name );
      return tmp;
   } else {
      Cell *tmp = root;
      while (tmp->link != NULL) tmp = tmp->link;
      tmp->link = snocString ( tmp->link, name );
      return root;
   }
}


/*---------------------------------------------*/
#define ISFLAG(s) (strcmp(aa->name, (s))==0)


IntNative main ( IntNative argc, Char *argv[] )
{
   Int32  i, j;
   Char   *tmp;
   Cell   *argList;
   Cell   *aa;

   /*-- Be really really really paranoid :-) --*/
   if (sizeof(Int32) != 4 || sizeof(UInt32) != 4  ||
       sizeof(Int16) != 2 || sizeof(UInt16) != 2  ||
       sizeof(Char)  != 1 || sizeof(UChar)  != 1) {
      fprintf ( stderr,
                "bzip2: I'm not configured correctly for this platform!\n"
                "\tI require Int32, Int16 and Char to have sizes\n"
                "\tof 4, 2 and 1 bytes to run properly, and they don't.\n"
                "\tProbably you can fix this by defining them correctly,\n"
                "\tand recompiling.  Bye!\n" );
      exit(3);
   }


   /*-- Set up signal handlers --*/
   signal (SIGINT,  mySignalCatcher);
   signal (SIGTERM, mySignalCatcher);
   signal (SIGSEGV, mySIGSEGVorSIGBUScatcher);
#if BZ_UNIX
   signal (SIGHUP,  mySignalCatcher);
   signal (SIGBUS,  mySIGSEGVorSIGBUScatcher);
#endif


   /*-- Initialise --*/
   outputHandleJustInCase  = NULL;
   smallMode               = False;
   keepInputFiles          = False;
   forceOverwrite          = False;
   verbosity               = 0;
   blockSize100k           = 9;
   testFailsExist          = False;
   numFileNames            = 0;
   numFilesProcessed       = 0;
   workFactor              = 30;

   copyFileName ( inName,  "(none)" );
   copyFileName ( outName, "(none)" );

   copyFileName ( progNameReally, argv[0] );
   progName = &progNameReally[0];
   for (tmp = &progNameReally[0]; *tmp != '\0'; tmp++)
      if (*tmp == PATH_SEP) progName = tmp + 1;


   /*-- Expand filename wildcards in arg list --*/
   argList = NULL;
   for (i = 1; i <= argc-1; i++)
      APPEND_FILESPEC(argList, argv[i]);


   /*-- Find the length of the longest filename --*/
   longestFileName = 7;
   numFileNames    = 0;
   for (aa = argList; aa != NULL; aa = aa->link)
      if (aa->name[0] != '-') {
         numFileNames++;
         if (longestFileName < (Int32)strlen(aa->name) )
            longestFileName = (Int32)strlen(aa->name);
      }


   /*-- Determine source modes; flag handling may change this too. --*/
   if (numFileNames == 0)
      srcMode = SM_I2O; else srcMode = SM_F2F;


   /*-- Determine what to do (compress/uncompress/test/cat). --*/
   /*-- Note that subsequent flag handling may change this. --*/
   opMode = OM_Z;

   if ( (strstr ( progName, "unzip" ) != 0) ||
        (strstr ( progName, "UNZIP" ) != 0) )
      opMode = OM_UNZ;

   if ( (strstr ( progName, "z2cat" ) != 0) ||
        (strstr ( progName, "Z2CAT" ) != 0) ||
        (strstr ( progName, "zcat" ) != 0)  ||
        (strstr ( progName, "ZCAT" ) != 0) )  {
      opMode = OM_UNZ;
      srcMode = (numFileNames == 0) ? SM_I2O : SM_F2O;
   }


   /*-- Look at the flags. --*/
   for (aa = argList; aa != NULL; aa = aa->link)
      if (aa->name[0] == '-' && aa->name[1] != '-')
         for (j = 1; aa->name[j] != '\0'; j++)
            switch (aa->name[j]) {
               case 'c': srcMode          = SM_F2O; break;
               case 'd': opMode           = OM_UNZ; break;
               case 'z': opMode           = OM_Z; break;
               case 'f': forceOverwrite   = True; break;
               case 't': opMode           = OM_TEST; break;
               case 'k': keepInputFiles   = True; break;
               case 's': smallMode        = True; break;
               case '1': blockSize100k    = 1; break;
               case '2': blockSize100k    = 2; break;
               case '3': blockSize100k    = 3; break;
               case '4': blockSize100k    = 4; break;
               case '5': blockSize100k    = 5; break;
               case '6': blockSize100k    = 6; break;
               case '7': blockSize100k    = 7; break;
               case '8': blockSize100k    = 8; break;
               case '9': blockSize100k    = 9; break;
               case 'V':
               case 'L': license();            break;
               case 'v': verbosity++; break;
               case 'h': usage ( progName );
                         exit ( 1 );
                         break;
               default:  fprintf ( stderr, "%s: Bad flag `%s'\n",
                                   progName, aa->name );
                         usage ( progName );
                         exit ( 1 );
                         break;
         }

   /*-- And again ... --*/
   for (aa = argList; aa != NULL; aa = aa->link) {
      if (ISFLAG("--stdout"))            srcMode          = SM_F2O;  else
      if (ISFLAG("--decompress"))        opMode           = OM_UNZ;  else
      if (ISFLAG("--compress"))          opMode           = OM_Z;    else
      if (ISFLAG("--force"))             forceOverwrite   = True;    else
      if (ISFLAG("--test"))              opMode           = OM_TEST; else
      if (ISFLAG("--keep"))              keepInputFiles   = True;    else
      if (ISFLAG("--small"))             smallMode        = True;    else
      if (ISFLAG("--version"))           license();                  else
      if (ISFLAG("--license"))           license();                  else
      if (ISFLAG("--repetitive-fast"))   workFactor = 5;             else
      if (ISFLAG("--repetitive-best"))   workFactor = 150;           else
      if (ISFLAG("--verbose"))           verbosity++;                else
      if (ISFLAG("--help"))              { usage ( progName ); exit ( 1 ); }
         else
         if (strncmp ( aa->name, "--", 2) == 0) {
            fprintf ( stderr, "%s: Bad flag `%s'\n", progName, aa->name );
            usage ( progName );
            exit ( 1 );
         }
   }

   if (verbosity > 4) verbosity = 4;
   if (opMode == OM_Z && smallMode) blockSize100k = 2;

   if (srcMode == SM_F2O && numFileNames == 0) {
      fprintf ( stderr, "%s: -c expects at least one filename.\n",
                progName );
      exit ( 1 );
   }

   if (opMode == OM_TEST && srcMode == SM_F2O) {
      fprintf ( stderr, "%s: -c and -t cannot be used together.\n",
                progName );
      exit ( 1 );
   }

   if (opMode != OM_Z) blockSize100k = 0;

   if (opMode == OM_Z) {
      if (srcMode == SM_I2O)
         compress ( NULL );
         else
         for (aa = argList; aa != NULL; aa = aa->link)
            if (aa->name[0] != '-') {
               numFilesProcessed++;
               compress ( aa->name );
            }
   } else
   if (opMode == OM_UNZ) {
      if (srcMode == SM_I2O)
         uncompress ( NULL );
         else
         for (aa = argList; aa != NULL; aa = aa->link)
            if (aa->name[0] != '-') {
               numFilesProcessed++;
               uncompress ( aa->name );
            }
   } else {
      testFailsExist = False;
      if (srcMode == SM_I2O)
         testf ( NULL );
         else
         for (aa = argList; aa != NULL; aa = aa->link)
            if (aa->name[0] != '-') {
               numFilesProcessed++;
               testf ( aa->name );
            }
      if (testFailsExist) {
         fprintf ( stderr,
           "\n"
           "You can use the `bzip2recover' program to *attempt* to recover\n"
           "data from undamaged sections of corrupted files.\n\n"
         );
         exit(2);
      }
   }
   return 0;
}


/*-----------------------------------------------------------*/
/*--- end                                         bzip2.c ---*/
/*-----------------------------------------------------------*/
