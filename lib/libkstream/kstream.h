/* Header file for encrypted-stream library.
   Written by Ken Raeburn (Raeburn@Cygnus.COM).
   Copyright (C) 1991, 1992 by Cygnus Support.

   This file is distributed under the same terms as Kerberos.
   For copying and distribution information, please see the file
   <mit-copyright.h>.

   $NetBSD: kstream.h,v 1.1.1.1.4.2 2000/06/23 16:16:46 minoura Exp $
 */

#include <sys/types.h>		/* for size_t */

/* Each stream is set up for two-way communication; if buffering is
   requested, output will be flushed before input is read, or when
   kstream_flush is called.  */

#if defined (__STDC__) || defined (__cplusplus)
typedef void *kstream_ptr;
#else
typedef char *kstream_ptr;
#endif
typedef struct kstream_rec *kstream;
struct kstream_data_block {
  kstream_ptr ptr;
  size_t length;
};
struct kstream_crypt_ctl_block {
  /* Don't rely on anything in this structure.
     It is almost guaranteed to change.
     Right now, it's just a hack so we can bang out the interface
     in some form that lets us run both rcp and rlogin.  This is also
     the only reason the contents of this structure are public.  */
#if defined (__STDC__) || defined (__cplusplus)
  int (*encrypt) (struct kstream_data_block *, /* output -- written */
		  struct kstream_data_block*, /* input */
		  kstream str
		  );		/* ret val = # input bytes used */
  int (*decrypt) (struct kstream_data_block *, /* output -- written */
		  struct kstream_data_block*, /* input */
		  kstream str
		  );		/* ret val = # input bytes used */
  int (*init) (kstream str, kstream_ptr data);
  void (*destroy) (kstream str);
#else
  int (*encrypt) (), (*decrypt) (), (*init) ();
  void (*destroy) ();
#endif
};

/* ctl==0 means no encryption.  data is specific to crypt functions */
#if defined (__STDC__) || defined (__cplusplus)
kstream kstream_create_from_fd (int fd,
				const struct kstream_crypt_ctl_block *ctl,
				kstream_ptr data);
/* There should be a "standard" DES mode used here somewhere.
   These differ, and I haven't chosen one over the other (yet).  */
kstream kstream_create_rlogin_from_fd (int fd, void* sched,
				       unsigned char (*ivec)[8]);
kstream kstream_create_rcp_from_fd (int fd, void* sched,
				    unsigned char (*ivec)[8]);
int kstream_write (kstream, void*, size_t);
int kstream_read (kstream, void*, size_t);
int kstream_flush (kstream);
int kstream_destroy (kstream);
void kstream_set_buffer_mode (kstream, int);
#else
kstream kstream_create_from_fd (),
	kstream_create_rlogin_from_fd (),
	kstream_create_rcp_from_fd ();
void kstream_set_buffer_mode ();
#endif

#if 0 /* Perhaps someday... */
kstream kstream_create (principal, host, port, ...);
#endif

#if !defined (__STDC__) && !defined (__cplusplus) && !defined (const)
#define const /* empty */
#endif

typedef struct fifo {
  char data[10*1024];
  size_t next_write, next_read;
} fifo;

typedef struct kstream_rec {
  const struct kstream_crypt_ctl_block *ctl;
  int fd;
  int buffering : 2;
  kstream_ptr data;
  /* These should be made pointers as soon as code has been
     written to reallocate them.  Also, it would be more efficient
     to use pointers into the buffers, rather than continually shifting
     them down so unprocessed data starts at index 0.  */
  /* incoming */
  fifo in_crypt, in_clear;
  /* outgoing */
  fifo out_clear;
} kstream_rec;
