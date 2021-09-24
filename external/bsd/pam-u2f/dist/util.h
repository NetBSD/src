/*
 * Copyright (C) 2014-2019 Yubico AB - See COPYING
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <security/pam_appl.h>

#define BUFSIZE 1024
#define MAX_DEVS 24
#define PK_LEN 130 // Public key
#define KH_LEN 86  // Key handle
#define RD_LEN 40  // Rounding
#define DEVSIZE (((PK_LEN) + (KH_LEN) + (RD_LEN)))
#define DEFAULT_AUTHFILE_DIR_VAR "XDG_CONFIG_HOME"
#define DEFAULT_AUTHFILE "/Yubico/u2f_keys"
#define DEFAULT_AUTHFILE_SSH "/id_ecdsa_sk"
#define DEFAULT_AUTHFILE_DIR "/.config"
#define DEFAULT_AUTHFILE_DIR_SSH "/.ssh"
#define DEFAULT_AUTHPENDING_FILE_PATH "/var/run/user/%d/pam-u2f-authpending"
#define DEFAULT_PROMPT "Insert your U2F device, then press ENTER."
#define DEFAULT_CUE "Please touch the device."
#define DEFAULT_ORIGIN_PREFIX "pam://"
#define SSH_ORIGIN "ssh:"
#define DEBUG_STR "debug(pam_u2f): %s:%d (%s): "

#if defined(DEBUG_PAM)
#define D(file, ...) _debug(file, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define D(file, ...) ((void) 0)
#endif /* DEBUG_PAM */

typedef struct {
  unsigned max_devs;
  int manual;
  int debug;
  int nouserok;
  int openasuser;
  int alwaysok;
  int interactive;
  int cue;
  int nodetect;
  int userpresence;
  int userverification;
  int pinverification;
  int sshformat;
  const char *auth_file;
  const char *authpending_file;
  const char *origin;
  const char *appid;
  const char *prompt;
  const char *cue_prompt;
  FILE *debug_file;
  int is_custom_debug_file;
} cfg_t;

typedef struct {
  char *publicKey;
  char *keyHandle;
  char *coseType;
  char *attributes;
  int old_format;
} device_t;

int get_devices_from_authfile(const cfg_t *cfg, const char *username,
                              device_t *devices, unsigned *n_devs);
void free_devices(device_t *devices, const unsigned n_devs);

int do_authentication(const cfg_t *cfg, const device_t *devices,
                      const unsigned n_devs, pam_handle_t *pamh);
int do_manual_authentication(const cfg_t *cfg, const device_t *devices,
                             const unsigned n_devs, pam_handle_t *pamh);
char *converse(pam_handle_t *pamh, int echocode, const char *prompt);
int random_bytes(void *, size_t);
int cose_type(const char *, int *);
const char *cose_string(int);

#ifdef __GNUC__
void _debug(FILE *, const char *, int, const char *, const char *, ...)
  __attribute__((__format__(printf, 5, 6)));
#else
void _debug(FILE *, const char *, int, const char *, const char *, ...);
#endif /* __GNUC__ */

#if !defined(HAVE_EXPLICIT_BZERO)
void explicit_bzero(void *, size_t);
#endif

#endif /* UTIL_H */
