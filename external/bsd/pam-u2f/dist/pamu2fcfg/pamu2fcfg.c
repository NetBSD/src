/*
 * Copyright (C) 2014-2021 Yubico AB - See COPYING
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

#include "openbsd-compat.h"

static fido_cred_t *prepare_cred(const struct gengetopt_args_info *const args) {
  fido_cred_t *cred = NULL;
  fido_opt_t resident_key;
  char *appid = NULL;
  char *user = NULL;
  struct passwd *passwd;
  unsigned char userid[32];
  unsigned char cdh[32];
  char origin[BUFSIZE];
  int type;
  int ok = -1;
  size_t n;
  int r;

  if ((cred = fido_cred_new()) == NULL) {
    fprintf(stderr, "fido_cred_new failed\n");
    goto err;
  }

  type = COSE_ES256; /* default */
  if (args->type_given) {
    if (!cose_type(args->type_arg, &type)) {
      fprintf(stderr, "Unknown COSE type '%s'.\n", args->type_arg);
      goto err;
    }
  }

  if ((r = fido_cred_set_type(cred, type)) != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_type (%d): %s\n", r, fido_strerr(r));
    goto err;
  }

  if (!random_bytes(cdh, sizeof(cdh))) {
    fprintf(stderr, "random_bytes failed\n");
    goto err;
  }

  if ((r = fido_cred_set_clientdata_hash(cred, cdh, sizeof(cdh))) != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_clientdata_hash (%d): %s\n", r,
            fido_strerr(r));
    goto err;
  }

  if (args->origin_given) {
    if (strlcpy(origin, args->origin_arg, sizeof(origin)) >= sizeof(origin)) {
      fprintf(stderr, "error: strlcpy failed\n");
      goto err;
    }
  } else {
    if ((n = strlcpy(origin, PAM_PREFIX, sizeof(origin))) >= sizeof(origin)) {
      fprintf(stderr, "error: strlcpy failed\n");
      goto err;
    }
    if (gethostname(origin + n, sizeof(origin) - n) == -1) {
      perror("gethostname");
      goto err;
    }
  }

  if (args->appid_given) {
    appid = args->appid_arg;
  } else {
    appid = origin;
  }

  if (args->verbose_given) {
    fprintf(stderr, "Setting origin to %s\n", origin);
    fprintf(stderr, "Setting appid to %s\n", appid);
  }

  if ((r = fido_cred_set_rp(cred, origin, appid)) != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_rp (%d) %s\n", r, fido_strerr(r));
    goto err;
  }

  if (args->username_given) {
    user = args->username_arg;
  } else {
    if ((passwd = getpwuid(getuid())) == NULL) {
      perror("getpwuid");
      goto err;
    }
    user = passwd->pw_name;
  }

  if (!random_bytes(userid, sizeof(userid))) {
    fprintf(stderr, "random_bytes failed\n");
    goto err;
  }

  if (args->verbose_given) {
    fprintf(stderr, "Setting user to %s\n", user);
    fprintf(stderr, "Setting user id to ");
    for (size_t i = 0; i < sizeof(userid); i++)
      fprintf(stderr, "%02x", userid[i]);
    fprintf(stderr, "\n");
  }

  if ((r = fido_cred_set_user(cred, userid, sizeof(userid), user, user,
                              NULL)) != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_user (%d) %s\n", r, fido_strerr(r));
    goto err;
  }

  if (args->resident_given) {
    resident_key = FIDO_OPT_TRUE;
  } else {
    resident_key = FIDO_OPT_OMIT;
  }

  if ((r = fido_cred_set_rk(cred, resident_key)) != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_rk (%d) %s\n", r, fido_strerr(r));
    goto err;
  }

  if ((r = fido_cred_set_uv(cred, FIDO_OPT_OMIT)) != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_uv (%d) %s\n", r, fido_strerr(r));
    goto err;
  }

  ok = 0;

err:
  if (ok != 0) {
    fido_cred_free(&cred);
  }

  return cred;
}

static int make_cred(const char *path, fido_dev_t *dev, fido_cred_t *cred) {
  char prompt[BUFSIZE];
  char pin[BUFSIZE];
  int n;
  int r;

  if (path == NULL || dev == NULL || cred == NULL) {
    fprintf(stderr, "%s: args\n", __func__);
    return -1;
  }

  r = fido_dev_make_cred(dev, cred, NULL);
  if (r == FIDO_ERR_PIN_REQUIRED) {
    n = snprintf(prompt, sizeof(prompt), "Enter PIN for %s: ", path);
    if (n < 0 || (size_t) n >= sizeof(prompt)) {
      fprintf(stderr, "error: snprintf prompt");
      return -1;
    }
    if (!readpassphrase(prompt, pin, sizeof(pin), RPP_ECHO_OFF)) {
      fprintf(stderr, "error: failed to read pin");
      explicit_bzero(pin, sizeof(pin));
      return -1;
    }
    r = fido_dev_make_cred(dev, cred, pin);
  }
  explicit_bzero(pin, sizeof(pin));

  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_dev_make_cred (%d) %s\n", r, fido_strerr(r));
    return -1;
  }

  return 0;
}

static int verify_cred(const fido_cred_t *const cred) {
  int r;

  if (cred == NULL) {
    fprintf(stderr, "%s: args\n", __func__);
    return -1;
  }

  if (fido_cred_x5c_ptr(cred) == NULL) {
    if ((r = fido_cred_verify_self(cred)) != FIDO_OK) {
      fprintf(stderr, "error: fido_cred_verify_self (%d) %s\n", r,
              fido_strerr(r));
      return -1;
    }
  } else {
    if ((r = fido_cred_verify(cred)) != FIDO_OK) {
      fprintf(stderr, "error: fido_cred_verify (%d) %s\n", r, fido_strerr(r));
      return -1;
    }
  }

  return 0;
}

static int print_authfile_line(const struct gengetopt_args_info *const args,
                               const fido_cred_t *const cred) {
  const unsigned char *kh = NULL;
  const unsigned char *pk = NULL;
  const char *user = NULL;
  char *b64_kh = NULL;
  char *b64_pk = NULL;
  size_t kh_len;
  size_t pk_len;
  int ok = -1;

  if ((kh = fido_cred_id_ptr(cred)) == NULL) {
    fprintf(stderr, "error: fido_cred_id_ptr returned NULL\n");
    goto err;
  }

  if ((kh_len = fido_cred_id_len(cred)) == 0) {
    fprintf(stderr, "error: fido_cred_id_len returned 0\n");
    goto err;
  }

  if ((pk = fido_cred_pubkey_ptr(cred)) == NULL) {
    fprintf(stderr, "error: fido_cred_pubkey_ptr returned NULL\n");
    goto err;
  }

  if ((pk_len = fido_cred_pubkey_len(cred)) == 0) {
    fprintf(stderr, "error: fido_cred_pubkey_len returned 0\n");
    goto err;
  }

  if (!b64_encode(kh, kh_len, &b64_kh)) {
    fprintf(stderr, "error: failed to encode key handle\n");
    goto err;
  }

  if (!b64_encode(pk, pk_len, &b64_pk)) {
    fprintf(stderr, "error: failed to encode public key\n");
    goto err;
  }

  if (!args->nouser_given) {
    if ((user = fido_cred_user_name(cred)) == NULL) {
      fprintf(stderr, "error: fido_cred_user_name returned NULL\n");
      goto err;
    }
    printf("%s", user);
  }

  printf(":%s,%s,%s,%s%s%s\n", args->resident_given ? "*" : b64_kh, b64_pk,
         cose_string(fido_cred_type(cred)),
         !args->no_user_presence_given ? "+presence" : "",
         args->user_verification_given ? "+verification" : "",
         args->pin_verification_given ? "+pin" : "");

  ok = 0;

err:
  free(b64_kh);
  free(b64_pk);

  return ok;
}

int main(int argc, char *argv[]) {
  int exit_code = EXIT_FAILURE;
  struct gengetopt_args_info args_info;
  fido_cred_t *cred = NULL;
  fido_dev_info_t *devlist = NULL;
  fido_dev_t *dev = NULL;
  const fido_dev_info_t *di = NULL;
  const char *path = NULL;
  size_t ndevs = 0;
  int r;

  /* NOTE: initializes args_info. on error, frees args_info and calls exit() */
  if (cmdline_parser(argc, argv, &args_info) != 0)
    goto err;

  if (args_info.help_given) {
    cmdline_parser_print_help();
    printf("\nReport bugs at <https://github.com/Yubico/pam-u2f>.\n");
    exit_code = EXIT_SUCCESS;
    goto err;
  }

  fido_init(args_info.debug_flag ? FIDO_DEBUG : 0);

  if ((cred = prepare_cred(&args_info)) == NULL)
    goto err;

  devlist = fido_dev_info_new(64);
  if (!devlist) {
    fprintf(stderr, "error: fido_dev_info_new failed\n");
    goto err;
  }

  r = fido_dev_info_manifest(devlist, 64, &ndevs);
  if (r != FIDO_OK) {
    fprintf(stderr, "Unable to discover device(s), %s (%d)\n", fido_strerr(r),
            r);
    goto err;
  }

  if (ndevs == 0) {
    for (int i = 0; i < TIMEOUT; i += FREQUENCY) {
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
        goto err;
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
    goto err;
  }

  /* XXX loop over every device? */
  dev = fido_dev_new();
  if (!dev) {
    fprintf(stderr, "fido_dev_new failed\n");
    goto err;
  }

  di = fido_dev_info_ptr(devlist, 0);
  if (!di) {
    fprintf(stderr, "error: fido_dev_info_ptr returned NULL\n");
    goto err;
  }

  if ((path = fido_dev_info_path(di)) == NULL) {
    fprintf(stderr, "error: fido_dev_path returned NULL\n");
    goto err;
  }

  r = fido_dev_open(dev, path);
  if (r != FIDO_OK) {
    fprintf(stderr, "error: fido_dev_open (%d) %s\n", r, fido_strerr(r));
    goto err;
  }

  if (make_cred(path, dev, cred) != 0 || verify_cred(cred) != 0 ||
      print_authfile_line(&args_info, cred) != 0)
    goto err;

  exit_code = EXIT_SUCCESS;

err:
  if (dev != NULL)
    fido_dev_close(dev);
  fido_dev_info_free(&devlist, ndevs);
  fido_cred_free(&cred);
  fido_dev_free(&dev);

  cmdline_parser_free(&args_info);

  exit(exit_code);
}
