/*
 rawrite.c	Write a binary image to a 360K diskette.
		By Mark Becker

 Usage:
	MS-DOS prompt> RAWRITE

	And follow the prompts.

History
-------

  1.0	-	Initial release
  1.1	-	Beta test (fixing bugs)				4/5/91
  		Some BIOS's don't like full-track writes.
  1.101	-	Last beta release.				4/8/91
  		Fixed BIOS full-track write by only
		writing 3 sectors at a time.
  1.2	-	Final code and documentation clean-ups.		4/9/91
*/
#include <alloc.h>
#include <bios.h>
#include <ctype.h>
#include <dir.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define SECTORSIZE	512

#define	RESET	0
#define	LAST	1
#define	READ	2
#define	WRITE	3
#define	VERIFY	4
#define	FORMAT	5

int	done;

/*
 Catch ^C and ^Break.
*/
int	handler(void)
{
  done = true;
  return(0);
}
void msg(char (*s))
{
	fprintf(stderr, "%s\n", s);
	_exit(1);
}
/*
 Identify the error code with a real error message.
*/
void Error(int (status))
{
  switch (status) {
    case 0x00:	msg("Operation Successful");				break;
    case 0x01:	msg("Bad command");					break;
    case 0x02:	msg("Address mark not found");				break;
    case 0x03:	msg("Attempt to write on write-protected disk");	break;
    case 0x04:	msg("Sector not found");				break;
    case 0x05:	msg("Reset failed (hard disk)");			break;
    case 0x06:	msg("Disk changed since last operation");		break;
    case 0x07:	msg("Drive parameter activity failed");			break;
    case 0x08:	msg("DMA overrun");					break;
    case 0x09:	msg("Attempt to DMA across 64K boundary");		break;
    case 0x0A:	msg("Bad sector detected");				break;
    case 0x0B:	msg("Bad track detected");				break;
    case 0x0C:	msg("Unsupported track");				break;
    case 0x10:	msg("Bad CRC/ECC on disk read");			break;
    case 0x11:	msg("CRC/ECC corrected data error");			break;
    case 0x20:	msg("Controller has failed");				break;
    case 0x40:	msg("Seek operation failed");				break;
    case 0x80:	msg("Attachment failed to respond");			break;
    case 0xAA:	msg("Drive not ready (hard disk only");			break;
    case 0xBB:	msg("Undefined error occurred (hard disk only)");	break;
    case 0xCC:	msg("Write fault occurred");				break;
    case 0xE0:	msg("Status error");					break;
    case 0xFF:	msg("Sense operation failed");				break;
  }
  _exit(1);
}

/*
 Identify what kind of diskette is installed in the specified drive.
 Return the number of sectors per track assumed as follows:
 9	-	360 K and 720 K 5.25".
15	-	1.2 M HD	5.25".
18	-	1.44 M		3.5".
*/
int nsects(int (drive))
{
  static	int	nsect[] = {18, 15, 9};

  char	*buffer;
  int	i, status;
/*
 Read sector 1, head 0, track 0 to get the BIOS running.
*/
  buffer = (char *)malloc(SECTORSIZE);
  biosdisk(RESET, drive, 0, 0, 0, 0, buffer);
  status = biosdisk(READ, drive, 0, 10, 1, 1, buffer);
  if (status == 0x06)			/* Door signal change?	*/
  status = biosdisk(READ, drive, 0, 0, 1, 1, buffer);

  for (i=0; i < sizeof(nsect)/sizeof(int); ++i) {
    biosdisk(RESET, drive, 0, 0, 0, 0, buffer);
    status = biosdisk(READ, drive, 0, 0, nsect[i], 1, buffer);
    if (status == 0x06)
      status = biosdisk(READ, drive, 0, 0, nsect[i], 1, buffer);
      if (status == 0x00) break;
    }
    if (i == sizeof(nsect)/sizeof(int)) {
      msg("Can't figure out how many sectors/track for this diskette.");
    }
    free(buffer);
    return(nsect[i]);
}

void main(void)
{
  char	 fname[MAXPATH];
  char	*buffer, *pbuf;
  int	 count, fdin, drive, head, track, status, spt, buflength, ns;

  puts("RaWrite 1.2 - Write disk file to raw floppy diskette\n");
  ctrlbrk(handler);
  printf("Enter source file name: ");
  scanf("%s", fname);
  _fmode = O_BINARY;
  if ((fdin = open(fname, O_RDONLY)) <= 0) {
     perror(fname);
     exit(1);
  }

  printf("Enter destination drive: ");
  scanf("%s", fname);
  drive = (fname[0] - 'A') & 0xf;
  printf("Please insert a formatted diskette into ");
  printf("drive %c: and press -ENTER- :", drive + 'A');
  while (bioskey(1) == 0) ;				/* Wait...	*/
  if ((bioskey(0) & 0x7F) == 3) exit(1);		/* Check for ^C	*/
  putchar('\n');
  done = false;
/*
 * Determine number of sectors per track and allocate buffers.
 */
  spt = nsects(drive);
  buflength = spt * SECTORSIZE;
  buffer = (char *)malloc(buflength);
  printf("Number of sectors per track for this disk is %d\n", spt);
  printf("Writing image to drive %c:.  Press ^C to abort.\n", drive+'A');
/*
 * Start writing data to diskette until there is no more data to write.
 */
   head = track = 0;
   while ((count = read(fdin, buffer, buflength)) > 0 && !done) {
     pbuf = buffer;
     for (ns = 1; count > 0 && !done; ns+=3) {
       printf("Track: %02d  Head: %2d Sector: %2d\r", track, head, ns);
       status = biosdisk(WRITE, drive, head, track, ns, 3, pbuf);

       if (status != 0) Error(status);

       count -= (3*SECTORSIZE);
       pbuf  += (3*SECTORSIZE);
     }
     if ((head = (head + 1) & 1) == 0) ++track;
   }
   if (eof(fdin)) {
     printf("\nDone.\n");
     biosdisk(2, drive, 0, 0, 1, 1, buffer);		/* Retract head	*/
   }
}	/* end main */
