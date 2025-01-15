/*
 * Copyright (C) 2014-2019 Yubico AB - See COPYING
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <security/pam_appl.h>

#define BUFSIZE 1024
#define MAX_DEVS 24
#define DEFAULT_AUTHFILE_DIR_VAR "XDG_CONFIG_HOME"
#define DEFAULT_AUTHFILE "Yubico/u2f_keys"
#define DEFAULT_AUTHFILE_SSH "id_ecdsa_sk"
#define DEFAULT_AUTHFILE_DIR ".config"
#define DEFAULT_AUTHFILE_DIR_SSH ".ssh"
#define DEFAULT_AUTHPENDING_FILE_PATH "/var/run/user/%d/pam-u2f-authpending"
#define DEFAULT_PROMPT "Insert your U2F device, then press ENTER."
#define DEFAULT_CUE "Please touch the device."
#define DEFAULT_ORIGIN_PREFIX "pam://"
#define SSH_ORIGIN "ssh:"

#define DEVLIST_LEN 64

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
  int expand;
  const char *auth_file;
  const char *authpending_file;
  const char *origin;
  const char *appid;
  const char *prompt;
  const char *cue_prompt;
  FILE *debug_file;
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
char *expand_variables(const char *, const char *);

#if !defined(HAVE_EXPLICIT_BZERO)
void explicit_bzero(void *, size_t);
#endif

#endif /* UTIL_H */
