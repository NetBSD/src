/*	$NetBSD: setbootimg.c,v 1.1.140.1 2009/05/13 17:18:42 jym Exp $	*/

/*
 *	set boot title image (converted by xpm2bootimg)
 *	to boot file or installed boot block
 *
 *	use with care, not to destroy the existent boot or the disklabel
 *
 *	written by Yasha (ITOH Yasufumi), public domain
 */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#ifdef __NetBSD__
#include <err.h>
#endif

/*
 * define here for cross env
 */
#define SIZE_BOOTBLK	8192		/* <ufs/ffs/fs.h>	BBSIZE */
#define MAGIC_DISKLABEL	0x82564557	/* <sys/disklabel.h>	DISKMAGIC */
#define X68K_LABELOFF	64		/* <x68k/disklabel.h>	LABELOFFSET */

#ifdef __STDC__
# define PROTO(x)	x
#else
# define PROTO(x)	()
# ifndef const
#  define const
# endif
#endif

int main PROTO((int argc, char *argv[]));
static unsigned get_uint16 PROTO((void *));
static unsigned get_uint32 PROTO((void *));

#ifndef __NetBSD__
/* for cross env */

#ifdef __STDC__
# include <stdarg.h>
# define VA_START(a, v)	va_start(a, v)
# include <errno.h>
#else
# include <varargs.h>
# define VA_START(a, v)	va_start(a)
extern int errno;
#endif

static void err PROTO((int eval, const char *fmt, ...));
static void errx PROTO((int eval, const char *fmt, ...));

static void
#ifdef __STDC__
err(int eval, const char *fmt, ...)
#else
err(eval, fmt, va_alist)
	int eval;
	const char *fmt;
	va_dcl
#endif
{
	int e = errno;
	va_list ap;

	fprintf(stderr, "setbootimg: ");
	if (fmt) {
		VA_START(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fprintf(stderr, ": ");
	}
	errno = e;
	perror((char *) 0);
	exit(eval);
}

static void
#ifdef __STDC__
errx(int eval, const char *fmt, ...)
#else
errx(eval, fmt, va_alist)
	int eval;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;

	fprintf(stderr, "setbootimg: ");
	if (fmt) {
		VA_START(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	fprintf(stderr, "\n");
	exit(eval);
}

#endif

static unsigned
get_uint16(void *p)
{
	unsigned char *q = p;

	return q[0] << 8 | q[1];
}

static unsigned
get_uint32(void *p)
{
	unsigned char *q = p;

	return q[0] << 24 | q[1] << 16 | q[2] << 8 | q[3];
}


static const char boottop[] = {
	0x60, 0x24, 0x53, 0x48, 0x41, 0x52, 0x50, 0x2F,
	0x58, 0x36, 0x38, 0x30, 0x78, 0x30, 0x81, 0x99,
	0x94, 0xE6, 0x82, 0xEA, 0x82, 0xBD, 0x8E, 0x9E,
	0x82, 0xC9, 0x82, 0xCD, 0x8C, 0xBB, 0x8E, 0xC0,
	0x93, 0xA6, 0x94, 0xF0, 0x81, 0x49
};

int
main(int argc, char *argv[])
{
	char *imgfile, *bootfile;
	char img[SIZE_BOOTBLK], boot[SIZE_BOOTBLK];
	int size_img, size_boot;
	int fd;
	int labelstart, labelend, imgstart, imgend;

	if (argc != 3) {
		fprintf(stderr, "usage: %s image_file boot_block\n", argv[0]);
		return 1;
	}

	imgfile = argv[1];
	bootfile = argv[2];

	/*
	 * read image
	 */
	if ((fd = open(imgfile, O_RDONLY)) < 0)
		err(1, "%s", imgfile);

	if ((size_img = read(fd, img, sizeof img)) < 0)
		err(1, "%s", imgfile);

	if (size_img >= (int) sizeof img)
		errx(1, "%s: file too large", imgfile);

	(void) close(fd);

	/*
	 * read boot block
	 */
	if ((fd = open(bootfile, O_RDWR)) < 0)
		err(1, "%s", bootfile);

	if ((size_boot = read(fd, boot, sizeof boot)) < 0)
		err(1, "%s", bootfile);

	if (lseek(fd, (off_t) 0, SEEK_SET))
		err(1, "%s: lseek", bootfile);

	if (size_boot < 4096)			/* XXX */
		errx(1, "%s: too short", bootfile);

	/*
	 * check boot block
	 */
	if (memcmp(boot, boottop, sizeof boottop))
		errx(1, "%s: not a boot", bootfile);

	labelstart = X68K_LABELOFF;
	if (get_uint16(boot + labelstart - 4) != 0x6000)	/* bra */
badboot:	errx(1, "%s: wrong boot block", bootfile);

	labelend = labelstart + get_uint16(boot + labelstart - 2) - 4;
	if (labelend >= size_boot)
		goto badboot;

	imgstart = get_uint16(boot + labelend);
	if (imgstart == 0)
		errx(1, "%s: no image support by this boot", bootfile);

	imgend = get_uint16(boot + imgstart);
	imgstart += 2;

	if (imgend < imgstart || imgend >= size_boot)
		goto badboot;

	/* disklabel exists? */
	if (get_uint32(boot + labelstart) == MAGIC_DISKLABEL)
		labelstart = labelend;		/* don't destroy disklabel */
	else
		labelstart += 2;

	/*
	 * the image fits this boot?
	 */
	if (size_img > (imgend - imgstart) + (labelend - labelstart))
		errx(1, "%s: image doesn't fit (max %d bytes)",
		     imgfile, (imgend - imgstart) + (labelend - labelstart));

	/*
	 * put image into boot
	 */
	if (size_img <= (imgend - imgstart)) {
		memcpy(boot + imgstart, img, size_img);
	} else {
		memcpy(boot + imgstart, img, imgend - imgstart);
		boot[labelstart - 2] = 'i';
		boot[labelstart - 1] = 'm';
		memcpy(boot + labelstart, img + (imgend - imgstart),
					size_img - (imgend - imgstart));
	}

	/*
	 * write back boot block
	 */
	if (write(fd, boot, size_boot) != size_boot)
		err(1, "%s: write back", bootfile);

	if (close(fd))
		err(1, "%s: close write", bootfile);

	return 0;
}
