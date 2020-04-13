/*
 * Copyright (C) 2014-2018 Yubico AB - See COPYING
 */

#define BUFSIZE 1024
#define PAM_PREFIX "pam://"
#define TIMEOUT 15
#define FREQUENCY 1

#include <fido.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "b64.h"
#include "cmdline.h"
#include "util.h"
#ifndef HAVE_READPASSPHRASE
#include "_readpassphrase.h"
#else
#include <readpassphrase.h>
#endif

int main(int argc, char *argv[]) {
  int exit_code = EXIT_FAILURE;
  struct gengetopt_args_info args_info;
  char buf[BUFSIZE];
  char prompt[BUFSIZE];
  char pin[BUFSIZE];
  char *p;
  char *response;
  fido_cred_t *cred = NULL;
  fido_dev_info_t *devlist = NULL;
  fido_dev_t *dev = NULL;
  const fido_dev_info_t *di = NULL;
  size_t ndevs;
  int cose_type;
  int resident_key;
  int user_presence;
  int user_verification;
  int pin_verification;
  int r;
  int n;
  char *origin = NULL;
  char *appid = NULL;
  char *user = NULL;
  char *b64_kh;
  char *b64_pk;
  struct passwd *passwd;
  const unsigned char *kh = NULL;
  size_t kh_len;
  const unsigned char *pk = NULL;
  size_t pk_len;
  unsigned char userid[32];
  unsigned char challenge[32];
  unsigned i;
  unsigned max_index = 0;

  if (cmdline_parser(argc, argv, &args_info) != 0)
    exit(EXIT_FAILURE);

  if (args_info.help_given) {
    cmdline_parser_print_help();
    printf("\nReport bugs at <https://github.com/Yubico/pam-u2f>.\n");
    exit(EXIT_SUCCESS);
  }

  fido_init(args_info.debug_flag ? FIDO_DEBUG : 0);

  cred = fido_cred_new();
  if (!cred) {
    fprintf(stderr, "fido_cred_new failed\n");
    exit(EXIT_FAILURE);
  }

  if (!random_bytes(challenge, sizeof(challenge))) {
    fprintf(stderr, "random_bytes failed\n");
    exit(EXIT_FAILURE);
  }

  if (args_info.type_given) {
    if (!strcasecmp(args_info.type_arg, "es256"))
      cose_type = COSE_ES256;
    else if (!strcasecmp(args_info.type_arg, "rs256"))
      cose_type = COSE_RS256;
    else {
      fprintf(stderr, "Unknown COSE type '%s'.\n", args_info.type_arg);
      exit(EXIT_FAILURE);
    }
  } else
    cose_type = COSE_ES256;

  r = fido_cred_set_type(cred, cose_type);
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_type (%d): %s\n", r, fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  r = fido_cred_set_clientdata_hash(cred, challenge, sizeof(challenge));
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_clientdata_hash (%d): %s\n", r,
            fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  if (args_info.origin_given)
    origin = args_info.origin_arg;
  else {
    if (!strcpy(buf, PAM_PREFIX)) {
      fprintf(stderr, "strcpy failed\n");
      exit(EXIT_FAILURE);
    }
    if (gethostname(buf + strlen(PAM_PREFIX), BUFSIZE - strlen(PAM_PREFIX)) ==
        -1) {
      perror("gethostname");
      exit(EXIT_FAILURE);
    }
    origin = buf;
  }

  if (args_info.verbose_given)
    fprintf(stderr, "Setting origin to %s\n", origin);

  if (args_info.appid_given)
    appid = args_info.appid_arg;
  else {
    appid = origin;
  }

  if (args_info.verbose_given)
    fprintf(stderr, "Setting appid to %s\n", appid);

  r = fido_cred_set_rp(cred, origin, appid);
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_rp (%d) %s\n", r, fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  if (args_info.username_given)
    user = args_info.username_arg;
  else {
    passwd = getpwuid(getuid());
    if (passwd == NULL) {
      perror("getpwuid");
      exit(EXIT_FAILURE);
    }
    user = passwd->pw_name;
  }

  if (!random_bytes(userid, sizeof(userid))) {
    fprintf(stderr, "random_bytes failed\n");
    exit(EXIT_FAILURE);
  }

  if (args_info.verbose_given) {
    fprintf(stderr, "Setting user to %s\n", user);
    fprintf(stderr, "Setting user id to ");
    for (size_t i = 0; i < sizeof(userid); i++)
      fprintf(stderr, "%02x", userid[i]);
    fprintf(stderr, "\n");
  }

  r = fido_cred_set_user(cred, userid, sizeof(userid), user, NULL, NULL);
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_user (%d) %s\n", r, fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  if (args_info.resident_given)
    resident_key = 1;
  else
    resident_key = 0;

  if (args_info.no_user_presence_given)
    user_presence = 0;
  else
    user_presence = 1;

  if (args_info.user_verification_given)
    user_verification = 1;
  else
    user_verification = 0;

  if (args_info.pin_verification_given)
    pin_verification = 1;
  else
    pin_verification = 0;

  r = fido_cred_set_rk(cred, resident_key);
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_rk (%d) %s\n", r, fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  r = fido_cred_set_uv(cred, false);
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_uv (%d) %s\n", r, fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  devlist = fido_dev_info_new(64);
  if (!devlist) {
    fprintf(stderr, "error: fido_dev_info_new failed\n");
    exit(EXIT_FAILURE);
  }

  r = fido_dev_info_manifest(devlist, 64, &ndevs);
  if (r != FIDO_OK) {
    fprintf(stderr, "Unable to discover device(s), %s (%d)\n", fido_strerr(r),
            r);
    exit(EXIT_FAILURE);
  }

  if (ndevs == 0) {
    for (i = 0; i < TIMEOUT; i += FREQUENCY) {
      fprintf(stderr,
              "\rNo U2F device available, please insert one now, you "
              "have %2d seconds",
              TIMEOUT - i);
      fflush(stderr);
      sleep(FREQUENCY);

      r = fido_dev_info_manifest(devlist, 64, &ndevs);
      if (r != FIDO_OK) {
        fprintf(stderr, "\nUnable to discover device(s), %s (%d)",
                fido_strerr(r), r);
        exit(EXIT_FAILURE);
      }

      if (ndevs != 0) {
        fprintf(stderr, "\nDevice found!\n");
        break;
      }
    }
  }

  if (ndevs == 0) {
    fprintf(stderr, "\rNo device found. Aborting.                              "
                    "           \n");
    exit(EXIT_FAILURE);
  }

  /* XXX loop over every device? */
  dev = fido_dev_new();
  if (!dev) {
    fprintf(stderr, "fido_dev_new failed\n");
    exit(EXIT_FAILURE);
  }

  di = fido_dev_info_ptr(devlist, 0);
  if (!di) {
    fprintf(stderr, "error: fido_dev_info_ptr returned NULL\n");
    exit(EXIT_FAILURE);
  }

  r = fido_dev_open(dev, fido_dev_info_path(di));
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_dev_open (%d) %s\n", r, fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  r = fido_dev_make_cred(dev, cred, NULL);
  if (r == FIDO_ERR_PIN_REQUIRED) {
    n = snprintf(prompt, sizeof(prompt),
                 "Enter PIN for %s: ", fido_dev_info_path(di));
    if (n < 0 || (size_t) n >= sizeof(prompt)) {
      fprintf(stderr, "error: snprintf prompt");
      exit(EXIT_FAILURE);
    }
    if (!readpassphrase(prompt, pin, sizeof(pin), RPP_ECHO_OFF)) {
      fprintf(stderr, "error: failed to read pin");
      exit(EXIT_FAILURE);
    }
    r = fido_dev_make_cred(dev, cred, pin);
  }
  explicit_bzero(pin, sizeof(pin));

  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_dev_make_cred (%d) %s\n", r, fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  r = fido_cred_verify(cred);
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_verify (%d) %s\n", r, fido_strerr(r));
    exit(EXIT_FAILURE);
  }

  kh = fido_cred_id_ptr(cred);
  if (!kh) {
    fprintf(stderr, "error: fido_cred_id_ptr returned NULL\n");
    exit(EXIT_FAILURE);
  }

  kh_len = fido_cred_id_len(cred);
  if (kh_len == 0) {
    fprintf(stderr, "error: fido_cred_id_len returned 0\n");
    exit(EXIT_FAILURE);
  }

  pk = (const unsigned char *) fido_cred_pubkey_ptr(cred);
  if (!pk) {
    fprintf(stderr, "error: fido_cred_pubkey_ptr returned NULL\n");
    exit(EXIT_FAILURE);
  }

  pk_len = fido_cred_pubkey_len(cred);
  if (pk_len == 0) {
    fprintf(stderr, "error: fido_cred_pubkey_len returned 0\n");
    exit(EXIT_FAILURE);
  }

  if (!b64_encode(kh, kh_len, &b64_kh)) {
    fprintf(stderr, "error: failed to encode key handle\n");
    exit(EXIT_FAILURE);
  }

  if (!b64_encode(pk, pk_len, &b64_pk)) {
    fprintf(stderr, "error: failed to encode public key\n");
    exit(EXIT_FAILURE);
  }

  if (!args_info.nouser_given)
    printf("%s", user);

  printf(":%s,%s,%s,%s%s%s", resident_key ? "*" : b64_kh, b64_pk,
         cose_type == COSE_ES256 ? "es256" : "rs256",
         user_presence ? "+presence" : "",
         user_verification ? "+verification" : "",
         pin_verification ? "+pin" : "");

  exit_code = EXIT_SUCCESS;

  fido_dev_info_free(&devlist, ndevs);
  fido_cred_free(&cred);
  fido_dev_free(&dev);

  free(b64_kh);
  free(b64_pk);

  exit(exit_code);
}
