/* Copyright (C) 2021 Yubico AB - See COPYING */
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/bio.h>
#include <openssl/ec.h>
#include <fido.h>

#include "drop_privs.h"
#include "fuzz/fuzz.h"

#ifdef HAVE_PAM_MODUTIL_DROP_PRIV
typedef struct pam_modutil_privs fuzz_privs_t;
#else
typedef struct _ykman_privs fuzz_privs_t;
#endif

/* In order to be able to fuzz pam-u2f, we need to be able to have a some
 * predictable data regardless of where its being run. We therefore override
 * functions which retrieve the local system's users, uid, hostnames,
 * pam application data, and authenticator data. */
static const char *user_ptr = NULL;
static struct pam_conv *conv_ptr = NULL;
static uint8_t *wiredata_ptr = NULL;
static size_t wiredata_len = 0;
static int authfile_fd = -1;
static char env[] = "value";

/* wrap a function, make it fail 0.25% of the time */
#define WRAP(type, name, args, retval, param)                                  \
  extern type __wrap_##name args;                                              \
  extern type __real_##name args;                                              \
  type __wrap_##name args {                                                    \
    if (uniform_random(400) < 1) {                                             \
      return (retval);                                                         \
    }                                                                          \
                                                                               \
    return (__real_##name param);                                              \
  }

void set_wiredata(uint8_t *data, size_t len) {
  wiredata_ptr = data;
  wiredata_len = len;
}
void set_user(const char *user) { user_ptr = user; }
void set_conv(struct pam_conv *conv) { conv_ptr = conv; }
void set_authfile(int fd) { authfile_fd = fd; }

WRAP(int, close, (int fd), -1, (fd))
WRAP(void *, strdup, (const char *s), NULL, (s))
WRAP(void *, calloc, (size_t nmemb, size_t size), NULL, (nmemb, size))
WRAP(void *, malloc, (size_t size), NULL, (size))
WRAP(int, gethostname, (char *name, size_t len), -1, (name, len))
WRAP(FILE *, fdopen, (int fd, const char *mode), NULL, (fd, mode))
WRAP(int, fstat, (int fd, struct stat *st), -1, (fd, st))
WRAP(ssize_t, read, (int fd, void *buf, size_t count), -1, (fd, buf, count))
WRAP(BIO *, BIO_new, (const BIO_METHOD *type), NULL, (type))
WRAP(int, BIO_write, (BIO * b, const void *data, int len), -1, (b, data, len))
WRAP(int, BIO_read, (BIO * b, void *data, int len), -1, (b, data, len))
WRAP(int, BIO_ctrl, (BIO * b, int cmd, long larg, void *parg), -1,
     (b, cmd, larg, parg))
WRAP(BIO *, BIO_new_mem_buf, (const void *buf, int len), NULL, (buf, len))
WRAP(EC_KEY *, EC_KEY_new_by_curve_name, (int nid), NULL, (nid))
WRAP(const EC_GROUP *, EC_KEY_get0_group, (const EC_KEY *key), NULL, (key))

extern uid_t __wrap_geteuid(void);
extern uid_t __wrap_geteuid(void) {
  return (uniform_random(10) < 1) ? 0 : 1008;
}

extern int __real_open(const char *pathname, int flags);
extern int __wrap_open(const char *pathname, int flags);
extern int __wrap_open(const char *pathname, int flags) {
  if (uniform_random(400) < 1)
    return -1;
  /* open write-only files as /dev/null */
  if ((flags & O_ACCMODE) == O_WRONLY)
    return __real_open("/dev/null", flags);
  /* FIXME: special handling for /dev/random */
  if (strcmp(pathname, "/dev/urandom") == 0)
    return __real_open(pathname, flags);
  /* open read-only files using a shared fd for the authfile */
  if ((flags & O_ACCMODE) == O_RDONLY)
    return dup(authfile_fd);
  assert(0); /* unsupported */
  return -1;
}

extern int __wrap_getpwuid_r(uid_t, struct passwd *, char *, size_t,
                             struct passwd **);
extern int __wrap_getpwuid_r(uid_t uid, struct passwd *pwd, char *buf,
                             size_t buflen, struct passwd **result) {
  const char *user = user_ptr;
  int offset;

  *result = NULL;
  if (user == NULL || uniform_random(400) < 1)
    return EIO;
  if (uniform_random(400) < 1)
    return 0; /* No matching record */
  if (uniform_random(400) < 1)
    user = "root";

  pwd->pw_uid = uid;
  pwd->pw_gid = uid;

  if ((offset = snprintf(buf, buflen, "/home/")) < 0 ||
      (size_t) offset >= buflen)
    return ENOMEM;

  pwd->pw_dir = buf;
  buf += offset;
  buflen -= offset;

  if ((offset = snprintf(buf, buflen, "%s", user)) < 0 ||
      (size_t) offset >= buflen)
    return ENOMEM;

  if (offset > 1 && uniform_random(400) < 1)
    buf[offset - 1] = '\0'; /* unexpected username */

  pwd->pw_name = buf;
  *result = pwd;
  return 0;
}

extern int __wrap_getpwnam_r(const char *, struct passwd *, char *, size_t,
                             struct passwd **);
extern int __wrap_getpwnam_r(const char *name, struct passwd *pwd, char *buf,
                             size_t buflen, struct passwd **result) {
  assert(name);
  return __wrap_getpwuid_r(1008, pwd, buf, buflen, result);
}

extern int __wrap_pam_get_item(const pam_handle_t *, int, const void **);
extern int __wrap_pam_get_item(const pam_handle_t *pamh, int item_type,
                               const void **item) {
  assert(pamh == (void *) FUZZ_PAM_HANDLE);
  assert(item_type == PAM_CONV); /* other types unsupported */
  assert(item != NULL);
  *item = conv_ptr;

  return uniform_random(400) < 1 ? PAM_CONV_ERR : PAM_SUCCESS;
}

extern int __wrap_pam_get_user(pam_handle_t *, const char **, const char *);
extern int __wrap_pam_get_user(pam_handle_t *pamh, const char **user_p,
                               const char *prompt) {
  assert(pamh == (void *) FUZZ_PAM_HANDLE);
  assert(user_p != NULL);
  assert(prompt == NULL);
  *user_p = user_ptr;

  return uniform_random(400) < 1 ? PAM_CONV_ERR : PAM_SUCCESS;
}

extern int __wrap_pam_modutil_drop_priv(pam_handle_t *, fuzz_privs_t *,
                                        struct passwd *);
extern int __wrap_pam_modutil_drop_priv(pam_handle_t *pamh, fuzz_privs_t *privs,
                                        struct passwd *pwd) {
  assert(pamh == (void *) FUZZ_PAM_HANDLE);
  assert(privs != NULL);
  assert(pwd != NULL);

  return uniform_random(400) < 1 ? -1 : 0;
}

extern int __wrap_pam_modutil_regain_priv(pam_handle_t *, fuzz_privs_t *,
                                          struct passwd *);
extern int __wrap_pam_modutil_regain_priv(pam_handle_t *pamh,
                                          fuzz_privs_t *privs,
                                          struct passwd *pwd) {
  assert(pamh == (void *) FUZZ_PAM_HANDLE);
  assert(privs != NULL);
  assert(pwd != NULL);

  return uniform_random(400) < 1 ? -1 : 0;
}

extern char *__wrap_secure_getenv(const char *);
extern char *__wrap_secure_getenv(const char *name) {
  (void) name;

  if (uniform_random(400) < 1)
    return env;
  return NULL;
}

static int buf_read(unsigned char *ptr, size_t len, int ms) {
  size_t n;

  (void) ms;

  if (wiredata_len < len)
    n = wiredata_len;
  else
    n = len;

  memcpy(ptr, wiredata_ptr, n);
  wiredata_ptr += n;
  wiredata_len -= n;

  return (int) n;
}

static int buf_write(const unsigned char *ptr, size_t len) {
  (void) ptr;
  return (int) len;
}

static void *dev_open(const char *path) {
  (void) path;
  return (void *) FUZZ_DEV_HANDLE;
}

static void dev_close(void *handle) {
  assert(handle == (void *) FUZZ_DEV_HANDLE);
}

static int dev_read(void *handle, unsigned char *ptr, size_t len, int ms) {
  assert(handle == (void *) FUZZ_DEV_HANDLE);
  return buf_read(ptr, len, ms);
}

static int dev_write(void *handle, const unsigned char *ptr, size_t len) {
  assert(handle == (void *) FUZZ_DEV_HANDLE);
  return buf_write(ptr, len);
}

extern int __wrap_fido_dev_open(fido_dev_t *dev, const char *path);
extern int __real_fido_dev_open(fido_dev_t *dev, const char *path);
int __wrap_fido_dev_open(fido_dev_t *dev, const char *path) {
  fido_dev_io_t io;
  int r;

  (void) path;

  memset(&io, 0, sizeof(io));

  io.open = dev_open;
  io.close = dev_close;
  io.read = dev_read;
  io.write = dev_write;

  if ((r = fido_dev_set_io_functions(dev, &io)) != FIDO_OK)
    goto err;

  if ((r = __real_fido_dev_open(dev, "nodev")) != FIDO_OK)
    goto err;

err:
  return r;
}

extern int __wrap_fido_dev_info_manifest(fido_dev_info_t *, size_t, size_t *);
extern int __wrap_fido_dev_info_manifest(fido_dev_info_t *devlist, size_t ilen,
                                         size_t *olen) {
  (void) devlist;
  (void) ilen;

  *olen = (size_t) uniform_random((uint32_t) ilen);

  return uniform_random(400) < 1 ? FIDO_ERR_INTERNAL : FIDO_OK;
}
