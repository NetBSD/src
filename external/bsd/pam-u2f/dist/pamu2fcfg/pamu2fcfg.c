/*
 * Copyright (C) 2014-2022 Yubico AB - See COPYING
 */

#define BUFSIZE 1024
#define PAM_PREFIX "pam://"
#define TIMEOUT 15
#define FREQUENCY 1

#define PIN_SET 0x01
#define PIN_UNSET 0x02
#define UV_SET 0x04
#define UV_UNSET 0x08
#define UV_REQD 0x10
#define UV_NOT_REQD 0x20

#include <fido.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <err.h>

#include "b64.h"
#include "util.h"

#include "openbsd-compat.h"

#ifndef FIDO_ERR_UV_BLOCKED /* XXX: compat libfido2 <1.5.0 */
#define FIDO_ERR_UV_BLOCKED 0x3c
#endif

struct args {
  const char *appid;
  const char *origin;
  const char *type;
  const char *username;
  int resident;
  int no_user_presence;
  int pin_verification;
  int user_verification;
  int debug;
  int verbose;
  int nouser;
};

static fido_cred_t *prepare_cred(const struct args *const args) {
  fido_cred_t *cred = NULL;
  const char *appid = NULL;
  const char *user = NULL;
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
  if (args->type && !cose_type(args->type, &type)) {
    fprintf(stderr, "Unknown COSE type '%s'.\n", args->type);
    goto err;
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

  if (args->origin) {
    if (strlcpy(origin, args->origin, sizeof(origin)) >= sizeof(origin)) {
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

  if (args->appid) {
    appid = args->appid;
  } else {
    appid = origin;
  }

  if (args->verbose) {
    fprintf(stderr, "Setting origin to %s\n", origin);
    fprintf(stderr, "Setting appid to %s\n", appid);
  }

  if ((r = fido_cred_set_rp(cred, origin, appid)) != FIDO_OK) {
    fprintf(stderr, "error: fido_cred_set_rp (%d) %s\n", r, fido_strerr(r));
    goto err;
  }

  if (args->username) {
    user = args->username;
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

  if (args->verbose) {
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

  if ((r = fido_cred_set_rk(cred, args->resident ? FIDO_OPT_TRUE
                                                 : FIDO_OPT_OMIT)) != FIDO_OK) {
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

static int make_cred(const struct args *args, const char *path, fido_dev_t *dev,
                     fido_cred_t *cred, int devopts) {
  char prompt[BUFSIZE];
  char pin[BUFSIZE];
  int n;
  int r;

  if (path == NULL || dev == NULL || cred == NULL) {
    fprintf(stderr, "%s: args\n", __func__);
    return -1;
  }

  /* Some form of UV required; built-in UV is available. */
  if (args->user_verification || (devopts & (UV_SET | UV_NOT_REQD)) == UV_SET) {
    if ((r = fido_cred_set_uv(cred, FIDO_OPT_TRUE)) != FIDO_OK) {
      fprintf(stderr, "error: fido_cred_set_uv: %s (%d)\n", fido_strerr(r), r);
      return -1;
    }
  }

  /* Let built-in UV have precedence over PIN. No UV also handled here. */
  if (args->user_verification || !args->pin_verification) {
    r = fido_dev_make_cred(dev, cred, NULL);
  } else {
    r = FIDO_ERR_PIN_REQUIRED;
  }

  /* Some form of UV required; built-in UV failed or is not available. */
  if ((devopts & PIN_SET) &&
      (r == FIDO_ERR_PIN_REQUIRED || r == FIDO_ERR_UV_BLOCKED ||
       r == FIDO_ERR_PIN_BLOCKED)) {
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

static int print_authfile_line(const struct args *const args,
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

  if (!args->nouser) {
    if ((user = fido_cred_user_name(cred)) == NULL) {
      fprintf(stderr, "error: fido_cred_user_name returned NULL\n");
      goto err;
    }
    printf("%s", user);
  }

  printf(":%s,%s,%s,%s%s%s", args->resident ? "*" : b64_kh, b64_pk,
         cose_string(fido_cred_type(cred)),
         !args->no_user_presence ? "+presence" : "",
         args->user_verification ? "+verification" : "",
         args->pin_verification ? "+pin" : "");

  ok = 0;

err:
  free(b64_kh);
  free(b64_pk);

  return ok;
}

static int get_device_options(fido_dev_t *dev, int *devopts) {
  char *const *opts;
  const bool *vals;
  fido_cbor_info_t *info;
  int r;

  *devopts = 0;

  if (!fido_dev_is_fido2(dev))
    return 0;

  if ((info = fido_cbor_info_new()) == NULL) {
    fprintf(stderr, "fido_cbor_info_new failed\n");
    return -1;
  }
  if ((r = fido_dev_get_cbor_info(dev, info)) != FIDO_OK) {
    fprintf(stderr, "fido_dev_get_cbor_info: %s (%d)\n", fido_strerr(r), r);
    fido_cbor_info_free(&info);
    return -1;
  }

  opts = fido_cbor_info_options_name_ptr(info);
  vals = fido_cbor_info_options_value_ptr(info);
  for (size_t i = 0; i < fido_cbor_info_options_len(info); i++) {
    if (strcmp(opts[i], "clientPin") == 0) {
      *devopts |= vals[i] ? PIN_SET : PIN_UNSET;
    } else if (strcmp(opts[i], "uv") == 0) {
      *devopts |= vals[i] ? UV_SET : UV_UNSET;
    } else if (strcmp(opts[i], "makeCredUvNotRqd") == 0) {
      *devopts |= vals[i] ? UV_NOT_REQD : UV_REQD;
    }
  }

  fido_cbor_info_free(&info);

  return 0;
}

static void parse_args(int argc, char *argv[], struct args *args) {
  int c;
  enum {
    OPT_VERSION = 0x100,
  };
  /* clang-format off */
  static const struct option options[] = {
    { "help",              no_argument,       NULL, 'h'         },
    { "version",           no_argument,       NULL, OPT_VERSION },
    { "origin",            required_argument, NULL, 'o'         },
    { "appid",             required_argument, NULL, 'i'         },
    { "type",              required_argument, NULL, 't'         },
    { "resident",          no_argument,       NULL, 'r'         },
    { "no-user-presence",  no_argument,       NULL, 'P'         },
    { "pin-verification",  no_argument,       NULL, 'N'         },
    { "user-verification", no_argument,       NULL, 'V'         },
    { "debug",             no_argument,       NULL, 'd'         },
    { "verbose",           no_argument,       NULL, 'v'         },
    { "username",          required_argument, NULL, 'u'         },
    { "nouser",            no_argument,       NULL, 'n'         },
    { 0,                   0,                 0,    0           }
  };
  const char *usage =
"Usage: pamu2fcfg [OPTION]...\n"
"Perform a FIDO2/U2F registration operation and print a configuration line that\n"
"can be used with the pam_u2f module.\n"
"\n"
"  -h, --help               Print help and exit\n"
"      --version            Print version and exit\n"
"  -o, --origin=STRING      Relying party ID to use during registration,\n"
"                             defaults to pam://hostname\n"
"  -i, --appid=STRING       Relying party name to use during registration,\n"
"                             defaults to the value of origin\n"
"  -t, --type=STRING        COSE type to use during registration (ES256, EDDSA,\n"
"                             or RS256), defaults to ES256\n"
"  -r, --resident           Generate a resident (discoverable) credential\n"
"  -P, --no-user-presence   Allow the credential to be used without ensuring the\n"
"                             user's presence\n"
"  -N, --pin-verification   Require PIN verification during authentication\n"
"  -V, --user-verification  Require user verification during authentication\n"
"  -d, --debug              Print debug information\n"
"  -v, --verbose            Print information about chosen origin and appid\n"
"  -u, --username=STRING    The name of the user registering the device,\n"
"                             defaults to the current user name\n"
"  -n, --nouser             Print only registration information (key handle,\n"
"                             public key, and options), useful for appending\n"
"\n"
"Report bugs at <" PACKAGE_BUGREPORT ">.\n";
  /* clang-format on */

  while ((c = getopt_long(argc, argv, "ho:i:t:rPNVdvu:n", options, NULL)) !=
         -1) {
    switch (c) {
      case 'h':
        printf("%s", usage);
        exit(EXIT_SUCCESS);
      case 'o':
        args->origin = optarg;
        break;
      case 'i':
        args->appid = optarg;
        break;
      case 't':
        args->type = optarg;
        break;
      case 'u':
        args->username = optarg;
        break;
      case 'r':
        args->resident = 1;
        break;
      case 'P':
        args->no_user_presence = 1;
        break;
      case 'N':
        args->pin_verification = 1;
        break;
      case 'V':
        args->user_verification = 1;
        break;
      case 'd':
        args->debug = 1;
        break;
      case 'v':
        args->verbose = 1;
        break;
      case 'n':
        args->nouser = 1;
        break;
      case OPT_VERSION:
        printf("pamu2fcfg " PACKAGE_VERSION "\n");
        exit(EXIT_SUCCESS);
      case '?':
        exit(EXIT_FAILURE);
      default:
        errx(EXIT_FAILURE, "unknown option 0x%x", c);
    }
  }

  if (optind != argc)
    errx(EXIT_FAILURE, "unsupported positional argument(s)");
}

int main(int argc, char *argv[]) {
  int exit_code = EXIT_FAILURE;
  struct args args = {0};
  fido_cred_t *cred = NULL;
  fido_dev_info_t *devlist = NULL;
  fido_dev_t *dev = NULL;
  const fido_dev_info_t *di = NULL;
  const char *path = NULL;
  size_t ndevs = 0;
  int devopts = 0;
  int r;

  parse_args(argc, argv, &args);
  fido_init(args.debug ? FIDO_DEBUG : 0);

  devlist = fido_dev_info_new(DEVLIST_LEN);
  if (!devlist) {
    fprintf(stderr, "error: fido_dev_info_new failed\n");
    goto err;
  }

  r = fido_dev_info_manifest(devlist, DEVLIST_LEN, &ndevs);
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

      r = fido_dev_info_manifest(devlist, DEVLIST_LEN, &ndevs);
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

  if (get_device_options(dev, &devopts) != 0) {
    goto err;
  }
  if (args.pin_verification && !(devopts & PIN_SET)) {
    warnx("%s", devopts & PIN_UNSET ? "device has no PIN"
                                    : "device does not support PIN");
    goto err;
  }
  if (args.user_verification && !(devopts & UV_SET)) {
    warnx("%s", devopts & UV_UNSET
                  ? "device has no built-in user verification configured"
                  : "device does not support built-in user verification");
    goto err;
  }
  if ((devopts & (UV_REQD | PIN_SET | UV_SET)) == UV_REQD) {
    warnx("%s", "some form of user verification required but none configured");
    goto err;
  }

  if ((cred = prepare_cred(&args)) == NULL)
    goto err;

  if (make_cred(&args, path, dev, cred, devopts) != 0 ||
      verify_cred(cred) != 0 || print_authfile_line(&args, cred) != 0)
    goto err;

  exit_code = EXIT_SUCCESS;

err:
  if (dev != NULL)
    fido_dev_close(dev);
  fido_dev_info_free(&devlist, ndevs);
  fido_cred_free(&cred);
  fido_dev_free(&dev);

  exit(exit_code);
}
