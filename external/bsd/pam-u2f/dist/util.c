/*
 * Copyright (C) 2014-2019 Yubico AB - See COPYING
 */

#include <fido.h>
#include <fido/es256.h>
#include <fido/rs256.h>

#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <syslog.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "b64.h"
#include "util.h"

static int hex_decode(const char *ascii_hex, unsigned char **blob,
                      size_t *blob_len) {
  *blob = NULL;
  *blob_len = 0;

  if (ascii_hex == NULL || (strlen(ascii_hex) % 2) != 0)
    return (0);

  *blob_len = strlen(ascii_hex) / 2;
  *blob = calloc(1, *blob_len);
  if (*blob == NULL)
    return (0);

  for (size_t i = 0; i < *blob_len; i++) {
    unsigned int c;
    int n = -1;
    int r = sscanf(ascii_hex, "%02x%n", &c, &n);
    if (r != 1 || n != 2 || c > UCHAR_MAX) {
      free(*blob);
      *blob = NULL;
      *blob_len = 0;
      return (0);
    }
    (*blob)[i] = (unsigned char) c;
    ascii_hex += n;
  }

  return (1);
}

static char *normal_b64(const char *websafe_b64) {
  char *b64;
  char *p;
  size_t n;

  n = strlen(websafe_b64);
  if (n > SIZE_MAX - 3)
    return (NULL);

  b64 = calloc(1, n + 3);
  if (b64 == NULL)
    return (NULL);

  memcpy(b64, websafe_b64, n);
  p = b64;

  while ((p = strpbrk(p, "-_")) != NULL) {
    switch (*p) {
      case '-':
        *p++ = '+';
        break;
      case '_':
        *p++ = '/';
        break;
    }
  }

  switch (n % 4) {
    case 1:
      b64[n] = '=';
      break;
    case 2:
    case 3:
      b64[n] = '=';
      b64[n + 1] = '=';
      break;
  }

  return (b64);
}

static es256_pk_t *translate_old_format_pubkey(const unsigned char *pk,
                                               size_t pk_len) {
  es256_pk_t *es256_pk = NULL;
  EC_KEY *ec = NULL;
  EC_POINT *q = NULL;
  const EC_GROUP *g = NULL;
  int ok = 0;

  if ((ec = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) == NULL ||
      (g = EC_KEY_get0_group(ec)) == NULL)
    goto fail;

  if ((q = EC_POINT_new(g)) == NULL ||
      !EC_POINT_oct2point(g, q, pk, pk_len, NULL) ||
      !EC_KEY_set_public_key(ec, q))
    goto fail;

  es256_pk = es256_pk_new();
  if (es256_pk == NULL || es256_pk_from_EC_KEY(es256_pk, ec) < 0)
    goto fail;

  ok = 1;
fail:
  if (ec != NULL)
    EC_KEY_free(ec);
  if (q != NULL)
    EC_POINT_free(q);
  if (!ok)
    es256_pk_free(&es256_pk);

  return (es256_pk);
}

int get_devices_from_authfile(const char *authfile, const char *username,
                              unsigned max_devs, int verbose, FILE *debug_file,
                              device_t *devices, unsigned *n_devs) {

  char *buf = NULL;
  char *s_user, *s_token;
  int retval = 0;
  int fd = -1;
  struct stat st;
  struct passwd *pw = NULL, pw_s;
  char buffer[BUFSIZE];
  int gpu_ret;
  FILE *opwfile = NULL;
  unsigned i;

  /* Ensure we never return uninitialized count. */
  *n_devs = 0;

  fd = open(authfile, O_RDONLY | O_CLOEXEC | O_NOCTTY);
  if (fd < 0) {
    if (verbose)
      D(debug_file, "Cannot open file: %s (%s)", authfile, strerror(errno));
    goto err;
  }

  if (fstat(fd, &st) < 0) {
    if (verbose)
      D(debug_file, "Cannot stat file: %s (%s)", authfile, strerror(errno));
    goto err;
  }

  if (!S_ISREG(st.st_mode)) {
    if (verbose)
      D(debug_file, "%s is not a regular file", authfile);
    goto err;
  }

  if (st.st_size == 0) {
    if (verbose)
      D(debug_file, "File %s is empty", authfile);
    goto err;
  }

  gpu_ret = getpwuid_r(st.st_uid, &pw_s, buffer, sizeof(buffer), &pw);
  if (gpu_ret != 0 || pw == NULL) {
    D(debug_file, "Unable to retrieve credentials for uid %u, (%s)", st.st_uid,
      strerror(errno));
    goto err;
  }

  if (strcmp(pw->pw_name, username) != 0 && strcmp(pw->pw_name, "root") != 0) {
    if (strcmp(username, "root") != 0) {
      D(debug_file,
        "The owner of the authentication file is neither %s nor root",
        username);
    } else {
      D(debug_file, "The owner of the authentication file is not root");
    }
    goto err;
  }

  opwfile = fdopen(fd, "r");
  if (opwfile == NULL) {
    if (verbose)
      D(debug_file, "fdopen: %s", strerror(errno));
    goto err;
  } else {
    fd = -1; /* fd belongs to opwfile */
  }

  buf = malloc(sizeof(char) * (DEVSIZE * max_devs));
  if (!buf) {
    if (verbose)
      D(debug_file, "Unable to allocate memory");
    goto err;
  }

  retval = -2;
  while (fgets(buf, (int) (DEVSIZE * (max_devs - 1)), opwfile)) {
    char *saveptr = NULL;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
      buf[len - 1] = '\0';

    if (verbose)
      D(debug_file, "Authorization line: %s", buf);

    s_user = strtok_r(buf, ":", &saveptr);
    if (s_user && strcmp(username, s_user) == 0) {
      if (verbose)
        D(debug_file, "Matched user: %s", s_user);

      retval = -1; // We found at least one line for the user

      // only keep last line for this user
      for (i = 0; i < *n_devs; i++) {
        free(devices[i].keyHandle);
        free(devices[i].publicKey);
        free(devices[i].coseType);
        free(devices[i].attributes);
        devices[i].keyHandle = NULL;
        devices[i].publicKey = NULL;
        devices[i].coseType = NULL;
        devices[i].attributes = NULL;
        devices[i].old_format = 0;
      }
      *n_devs = 0;

      i = 0;
      while ((s_token = strtok_r(NULL, ",", &saveptr)) != NULL) {
        if ((*n_devs)++ > max_devs - 1) {
          *n_devs = max_devs;
          if (verbose)
            D(debug_file,
              "Found more than %d devices, ignoring the remaining ones",
              max_devs);
          break;
        }

        devices[i].keyHandle = NULL;
        devices[i].publicKey = NULL;
        devices[i].coseType = NULL;
        devices[i].attributes = NULL;
        devices[i].old_format = 0;

        if (verbose)
          D(debug_file, "KeyHandle for device number %d: %s", i + 1, s_token);

        devices[i].keyHandle = strdup(s_token);

        if (!devices[i].keyHandle) {
          if (verbose)
            D(debug_file, "Unable to allocate memory for keyHandle number %d",
              i);
          goto err;
        }

        if (!strcmp(devices[i].keyHandle, "*") && verbose)
          D(debug_file, "Credential is resident");

        s_token = strtok_r(NULL, ",", &saveptr);

        if (!s_token) {
          if (verbose)
            D(debug_file, "Unable to retrieve publicKey number %d", i + 1);
          goto err;
        }

        if (verbose)
          D(debug_file, "publicKey for device number %d: %s", i + 1, s_token);

        devices[i].publicKey = strdup(s_token);

        if (!devices[i].publicKey) {
          if (verbose)
            D(debug_file, "Unable to allocate memory for publicKey number %d",
              i);
          goto err;
        }

        s_token = strtok_r(NULL, ",", &saveptr);

        devices[i].old_format = 0;

        if (!s_token) {
          if (verbose) {
            D(debug_file, "Unable to retrieve COSE type %d", i + 1);
            D(debug_file, "Assuming ES256 (backwards compatibility)");
          }
          devices[i].old_format = 1;
          devices[i].coseType = strdup("es256");
        } else {
          if (verbose)
            D(debug_file, "COSE type for device number %d: %s", i + 1, s_token);
          devices[i].coseType = strdup(s_token);
        }

        if (!devices[i].coseType) {
          if (verbose)
            D(debug_file, "Unable to allocate memory for COSE type number %d",
              i);
          goto err;
        }

        s_token = strtok_r(NULL, ":", &saveptr);

        if (!s_token) {
          if (verbose) {
            D(debug_file, "Unable to retrieve attributes %d", i + 1);
            D(debug_file, "Assuming 'p' (backwards compatibility)");
          }
          devices[i].attributes = strdup("p");
        } else {
          if (verbose)
            D(debug_file, "Attributes for device number %d: %s", i + 1,
              s_token);
          devices[i].attributes = strdup(s_token);
        }

        if (!devices[i].attributes) {
          if (verbose)
            D(debug_file, "Unable to allocate memory for attributes number %d",
              i);
          goto err;
        }

        if (devices[i].old_format) {
          char *websafe_b64 = devices[i].keyHandle;
          devices[i].keyHandle = normal_b64(websafe_b64);
          free(websafe_b64);
          if (!devices[i].keyHandle) {
            if (verbose)
              D(debug_file, "Unable to allocate memory for keyHandle number %d",
                i);
            goto err;
          }
        }

        i++;
      }
    }
  }

  if (verbose)
    D(debug_file, "Found %d device(s) for user %s", *n_devs, username);

  retval = 1;
  goto out;

err:
  for (i = 0; i < *n_devs; i++) {
    free(devices[i].keyHandle);
    free(devices[i].publicKey);
    free(devices[i].coseType);
    free(devices[i].attributes);
    devices[i].keyHandle = NULL;
    devices[i].publicKey = NULL;
    devices[i].coseType = NULL;
    devices[i].attributes = NULL;
  }

  *n_devs = 0;

out:
  if (buf) {
    free(buf);
    buf = NULL;
  }

  if (opwfile)
    fclose(opwfile);

  if (fd != -1)
    close(fd);

  return retval;
}

void free_devices(device_t *devices, const unsigned n_devs) {
  unsigned i;

  if (!devices)
    return;

  for (i = 0; i < n_devs; i++) {
    free(devices[i].keyHandle);
    devices[i].keyHandle = NULL;

    free(devices[i].publicKey);
    devices[i].publicKey = NULL;

    free(devices[i].coseType);
    devices[i].coseType = NULL;

    free(devices[i].attributes);
    devices[i].attributes = NULL;
  }

  free(devices);
  devices = NULL;
}

static int get_authenticators(const cfg_t *cfg, const fido_dev_info_t *devlist,
                              size_t devlist_len, fido_assert_t *assert,
                              const void *kh, fido_dev_t **authlist) {
  const fido_dev_info_t *di = NULL;
  fido_dev_t *dev = NULL;
  int r;
  size_t i;
  size_t j;

  if (cfg->debug)
    D(cfg->debug_file, "Working with %zu authenticator(s)", devlist_len);

  for (i = 0, j = 0; i < devlist_len; i++) {
    if (cfg->debug)
      D(cfg->debug_file, "Checking whether key exists in authenticator %zu", i);

    di = fido_dev_info_ptr(devlist, i);
    if (!di) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to get device pointer");
      continue;
    }

    if (cfg->debug)
      D(cfg->debug_file, "Authenticator path: %s", fido_dev_info_path(di));

    dev = fido_dev_new();
    if (!dev) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to allocate device type");
      continue;
    }

    r = fido_dev_open(dev, fido_dev_info_path(di));
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to open authenticator: %s (%d)",
          fido_strerr(r), r);
      fido_dev_free(&dev);
      continue;
    }

    if (kh == NULL || cfg->nodetect) {
      /* resident credential or nodetect: try all authenticators */
      authlist[j++] = dev;
    } else {
      r = fido_dev_get_assert(dev, assert, NULL);
      if ((!fido_dev_is_fido2(dev) && r == FIDO_ERR_USER_PRESENCE_REQUIRED) ||
          (fido_dev_is_fido2(dev) && r == FIDO_OK)) {
        authlist[j++] = dev;
        if (cfg->debug)
          D(cfg->debug_file, "Found key in authenticator %zu", i);
        return (1);
      }
      if (cfg->debug)
        D(cfg->debug_file, "Key not found in authenticator %zu", i);

      fido_dev_close(dev);
      fido_dev_free(&dev);
    }
  }

  if (kh == NULL && j != 0)
    return (1);
  else {
    if (cfg->debug)
      D(cfg->debug_file, "Key not found");
    return (0);
  }
}

int do_authentication(const cfg_t *cfg, const device_t *devices,
                      const unsigned n_devs, pam_handle_t *pamh) {
  es256_pk_t *es256_pk = NULL;
  rs256_pk_t *rs256_pk = NULL;
  fido_assert_t *assert = NULL;
  fido_dev_info_t *devlist = NULL;
  fido_dev_t **authlist = NULL;
  int cued = 0;
  int r;
  int retval = -2;
  int cose_type;
  size_t kh_len;
  size_t ndevs = 0;
  size_t ndevs_prev = 0;
  size_t pk_len;
  unsigned char challenge[32];
  unsigned char *kh = NULL;
  unsigned char *pk = NULL;
  unsigned i = 0;
  fido_opt_t user_presence = FIDO_OPT_OMIT;
  fido_opt_t user_verification = FIDO_OPT_OMIT;
  fido_opt_t pin_verification = FIDO_OPT_OMIT;
  char *pin = NULL;

  fido_init(cfg->debug ? FIDO_DEBUG : 0);

  devlist = fido_dev_info_new(64);
  if (!devlist) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to allocate devlist");
    goto out;
  }

  r = fido_dev_info_manifest(devlist, 64, &ndevs);
  if (r != FIDO_OK) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to discover device(s), %s (%d)",
        fido_strerr(r), r);
    goto out;
  }

  ndevs_prev = ndevs;

  if (cfg->debug)
    D(cfg->debug_file, "Device max index is %u", ndevs);

  es256_pk = es256_pk_new();
  if (!es256_pk) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to allocate ES256 public key");
    goto out;
  }

  rs256_pk = rs256_pk_new();
  if (!rs256_pk) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to allocate RS256 public key");
    goto out;
  }

  authlist = calloc(64 + 1, sizeof(fido_dev_t *));
  if (!authlist) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to allocate authenticator list");
    goto out;
  }

  if (cfg->nodetect && cfg->debug)
    D(cfg->debug_file,
      "nodetect option specified, suitable key detection will be skipped");

  i = 0;
  while (i < n_devs) {
    retval = -2;

    if (cfg->debug)
      D(cfg->debug_file, "Attempting authentication with device number %d",
        i + 1);

    assert = fido_assert_new();
    if (!assert) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to allocate assertion");
      goto out;
    }

    r = fido_assert_set_rp(assert, cfg->origin);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to set origin: %s (%d)", fido_strerr(r), r);
      goto out;
    }

    if (!strcmp(devices[i].keyHandle, "*")) {
      if (cfg->debug)
        D(cfg->debug_file, "Credential is resident");
    } else {
      if (cfg->debug)
        D(cfg->debug_file, "Key handle: %s", devices[i].keyHandle);
      if (!b64_decode(devices[i].keyHandle, (void **) &kh, &kh_len)) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to decode key handle");
        goto out;
      }

      r = fido_assert_allow_cred(assert, kh, kh_len);
      if (r != FIDO_OK) {
        if (cfg->debug)
          D(cfg->debug_file, "Unable to set keyHandle: %s (%d)", fido_strerr(r),
            r);
        goto out;
      }
    }

    if (devices[i].old_format) {
      if (!hex_decode(devices[i].publicKey, &pk, &pk_len)) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to decode public key");
        goto out;
      }
    } else {
      if (!b64_decode(devices[i].publicKey, (void **) &pk, &pk_len)) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to decode public key");
        goto out;
      }
    }

    if (!strcmp(devices[i].coseType, "es256")) {
      if (devices[i].old_format) {
        es256_pk = translate_old_format_pubkey(pk, pk_len);
        if (es256_pk == NULL) {
          if (cfg->debug)
            D(cfg->debug_file, "Failed to convert ES256 public key");
        }
      } else {
        r = es256_pk_from_ptr(es256_pk, pk, pk_len);
        if (r != FIDO_OK) {
          if (cfg->debug)
            D(cfg->debug_file, "Failed to convert ES256 public key");
        }
      }
      cose_type = COSE_ES256;
    } else if (!strcmp(devices[i].coseType, "rs256")) {
      r = rs256_pk_from_ptr(rs256_pk, pk, pk_len);
      if (r != FIDO_OK) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to convert RS256 public key");
      }
      cose_type = COSE_RS256;
    } else {
      if (cfg->debug)
        D(cfg->debug_file, "Unknown COSE type '%s'", devices[i].coseType);
      goto out;
    }

    if (cfg->userpresence == 1 || strstr(devices[i].attributes, "presence"))
      user_presence = FIDO_OPT_TRUE;
    else if (cfg->userpresence == 0)
      user_presence = FIDO_OPT_FALSE;
    else
      user_presence = FIDO_OPT_OMIT;

    if (cfg->userverification == 1 ||
        strstr(devices[i].attributes, "verification"))
      user_verification = FIDO_OPT_TRUE;
    else if (cfg->userverification == 0)
      user_verification = FIDO_OPT_FALSE;
    else
      user_verification = FIDO_OPT_OMIT;

    if (cfg->pinverification == 1 || strstr(devices[i].attributes, "pin")) {
      pin_verification = FIDO_OPT_TRUE;
      user_verification = FIDO_OPT_TRUE;
    } else if (cfg->pinverification == 0)
      pin_verification = FIDO_OPT_FALSE;
    else
      pin_verification = FIDO_OPT_OMIT;

    r = fido_assert_set_up(assert, FIDO_OPT_FALSE);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to set UP");
      goto out;
    }

    r = fido_assert_set_uv(assert, FIDO_OPT_OMIT);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to set UV");
      goto out;
    }

    if (!random_bytes(challenge, sizeof(challenge))) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to generate challenge");
      goto out;
    }

    if (cfg->debug) {
      char *b64_challenge;
      if (!b64_encode(challenge, sizeof(challenge), &b64_challenge)) {
        D(cfg->debug_file, "Failed to encode challenge");
      } else {
        D(cfg->debug_file, "Challenge: %s", b64_challenge);
        free(b64_challenge);
      }
    }

    r = fido_assert_set_clientdata_hash(assert, challenge, sizeof(challenge));
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to set challenge: %s( %d)", fido_strerr(r),
          r);
      goto out;
    }

    if (get_authenticators(cfg, devlist, ndevs, assert, kh, authlist)) {
      for (size_t j = 0; authlist[j] != NULL; j++) {
        r = fido_assert_set_up(assert, user_presence);
        if (r != FIDO_OK) {
          if (cfg->debug)
            D(cfg->debug_file, "Failed to reset UP");
          goto out;
        }

        r = fido_assert_set_uv(assert, user_verification);
        if (r != FIDO_OK) {
          if (cfg->debug)
            D(cfg->debug_file, "Failed to reset UV");
          goto out;
        }

        if (!random_bytes(challenge, sizeof(challenge))) {
          if (cfg->debug)
            D(cfg->debug_file, "Failed to regenerate challenge");
          goto out;
        }

        r =
          fido_assert_set_clientdata_hash(assert, challenge, sizeof(challenge));
        if (r != FIDO_OK) {
          if (cfg->debug)
            D(cfg->debug_file, "Unable to reset challenge: %s( %d)",
              fido_strerr(r), r);
          goto out;
        }

        if (pin_verification == FIDO_OPT_TRUE)
          pin = converse(pamh, PAM_PROMPT_ECHO_OFF, "Please enter the PIN: ");
        if (user_presence == FIDO_OPT_TRUE ||
            user_verification == FIDO_OPT_TRUE) {
          if (cfg->manual == 0 && cfg->cue && !cued) {
            cued = 1;
            converse(pamh, PAM_TEXT_INFO,
                     cfg->cue_prompt != NULL ? cfg->cue_prompt : DEFAULT_CUE);
          }
        }
        r = fido_dev_get_assert(authlist[j], assert, pin);
        if (pin) {
          explicit_bzero(pin, strlen(pin));
          free(pin);
          pin = NULL;
        }
        if (r == FIDO_OK) {
          r = fido_assert_verify(assert, 0, cose_type,
                                 cose_type == COSE_ES256
                                   ? (const void *) es256_pk
                                   : (const void *) rs256_pk);
          if (r == FIDO_OK) {
            retval = 1;
            goto out;
          }
        }
      }
    } else {
      if (cfg->debug)
        D(cfg->debug_file, "Device for this keyhandle is not present.");
    }

    i++;

    fido_dev_info_free(&devlist, ndevs);

    devlist = fido_dev_info_new(64);
    if (!devlist) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to allocate devlist");
      goto out;
    }

    r = fido_dev_info_manifest(devlist, 64, &ndevs);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to discover device(s), %s (%d)",
          fido_strerr(r), r);
      goto out;
    }

    if (ndevs > ndevs_prev) {
      if (cfg->debug)
        D(cfg->debug_file,
          "Devices max_index has changed: %zu (was %zu). Starting over", ndevs,
          ndevs_prev);
      ndevs_prev = ndevs;
      i = 0;
    }

    free(kh);
    free(pk);

    kh = NULL;
    pk = NULL;

    for (size_t j = 0; authlist[j] != NULL; j++) {
      fido_dev_close(authlist[j]);
      fido_dev_free(&authlist[j]);
    }

    fido_assert_free(&assert);
  }

out:
  es256_pk_free(&es256_pk);
  rs256_pk_free(&rs256_pk);
  fido_assert_free(&assert);
  fido_dev_info_free(&devlist, ndevs);

  if (authlist) {
    for (size_t j = 0; authlist[j] != NULL; j++) {
      fido_dev_close(authlist[j]);
      fido_dev_free(&authlist[j]);
    }
    free(authlist);
  }

  free(kh);
  free(pk);

  return retval;
}

#define MAX_PROMPT_LEN (1024)

int do_manual_authentication(const cfg_t *cfg, const device_t *devices,
                             const unsigned n_devs, pam_handle_t *pamh) {
  fido_assert_t *assert[n_devs];
  es256_pk_t *es256_pk[n_devs];
  rs256_pk_t *rs256_pk[n_devs];
  unsigned char challenge[32];
  unsigned char *kh = NULL;
  unsigned char *pk = NULL;
  unsigned char *authdata = NULL;
  unsigned char *sig = NULL;
  char *b64_challenge = NULL;
  char *b64_cdh = NULL;
  char *b64_rpid = NULL;
  char *b64_authdata = NULL;
  char *b64_sig = NULL;
  char prompt[MAX_PROMPT_LEN];
  char buf[MAX_PROMPT_LEN];
  size_t kh_len;
  size_t pk_len;
  size_t authdata_len;
  size_t sig_len;
  int cose_type[n_devs];
  int retval = -2;
  int n;
  int r;
  unsigned i = 0;
  bool user_presence = false;
  bool user_verification = false;

  memset(assert, 0, sizeof(assert));
  memset(es256_pk, 0, sizeof(es256_pk));
  memset(rs256_pk, 0, sizeof(rs256_pk));

  fido_init(cfg->debug ? FIDO_DEBUG : 0);

  for (i = 0; i < n_devs; ++i) {

    assert[i] = fido_assert_new();
    if (!assert[i]) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to allocate assertion %u", i);
      goto out;
    }

    r = fido_assert_set_rp(assert[i], cfg->origin);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to set origin: %s (%d)", fido_strerr(r), r);
      goto out;
    }

    if (strstr(devices[i].attributes, "presence"))
      user_presence = true;
    if (strstr(devices[i].attributes, "verification"))
      user_verification = true;

    r = fido_assert_set_up(assert[i], user_presence);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to set UP: %s (%d)", fido_strerr(r), r);
      goto out;
    }

    r = fido_assert_set_uv(assert[i], user_verification);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to set UV: %s (%d)", fido_strerr(r), r);
      goto out;
    }

    if (cfg->debug)
      D(cfg->debug_file, "Attempting authentication with device number %d",
        i + 1);

    if (!strcmp(devices[i].keyHandle, "*")) {
      if (cfg->debug)
        D(cfg->debug_file, "Credential is resident");
    } else {
      if (!b64_decode(devices[i].keyHandle, (void **) &kh, &kh_len)) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to decode key handle");
        goto out;
      }

      r = fido_assert_allow_cred(assert[i], kh, kh_len);
      if (r != FIDO_OK) {
        if (cfg->debug)
          D(cfg->debug_file, "Unable to set keyHandle: %s (%d)", fido_strerr(r),
            r);
        goto out;
      }

      free(kh);
      kh = NULL;
    }

    if (devices[i].old_format) {
      if (!hex_decode(devices[i].publicKey, &pk, &pk_len)) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to decode public key");
        goto out;
      }
    } else {
      if (!b64_decode(devices[i].publicKey, (void **) &pk, &pk_len)) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to decode public key");
        goto out;
      }
    }

    if (!strcmp(devices[i].coseType, "es256")) {
      es256_pk[i] = es256_pk_new();
      if (!es256_pk[i]) {
        if (cfg->debug)
          D(cfg->debug_file, "Unable to allocate key %u", i);
        goto out;
      }

      if (es256_pk_from_ptr(es256_pk[i], pk, pk_len) != FIDO_OK) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to convert public key");
        goto out;
      }

      cose_type[i] = COSE_ES256;
    } else {
      rs256_pk[i] = rs256_pk_new();
      if (!rs256_pk[i]) {
        if (cfg->debug)
          D(cfg->debug_file, "Unable to allocate key %u", i);
        goto out;
      }

      if (rs256_pk_from_ptr(rs256_pk[i], pk, pk_len) != FIDO_OK) {
        if (cfg->debug)
          D(cfg->debug_file, "Failed to convert public key");
        goto out;
      }

      cose_type[i] = COSE_RS256;
    }

    free(pk);
    pk = NULL;

    if (!random_bytes(challenge, sizeof(challenge))) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to generate challenge");
      goto out;
    }

    r =
      fido_assert_set_clientdata_hash(assert[i], challenge, sizeof(challenge));
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to set challenge");
      goto out;
    }

    if (!b64_encode(challenge, sizeof(challenge), &b64_challenge)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to encode challenge");
      goto out;
    }

    if (cfg->debug)
      D(cfg->debug_file, "Challenge: %s", b64_challenge);

    n = snprintf(prompt, sizeof(prompt), "Challenge #%d:", i + 1);
    if (n <= 0 || (size_t) n >= sizeof(prompt)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to print challenge prompt");
      goto out;
    }

    converse(pamh, PAM_TEXT_INFO, prompt);

    n = snprintf(buf, sizeof(buf), "%s\n%s\n%s", b64_challenge, cfg->origin,
                 devices[i].keyHandle);
    if (n <= 0 || (size_t) n >= sizeof(buf)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to print fido2-assert input string");
      goto out;
    }

    converse(pamh, PAM_TEXT_INFO, buf);

    free(b64_challenge);
    b64_challenge = NULL;
  }

  converse(pamh, PAM_TEXT_INFO,
           "Please pass the challenge(s) above to fido2-assert, and "
           "paste the results in the prompt below.");

  retval = -1;

  for (i = 0; i < n_devs; ++i) {
    n = snprintf(prompt, sizeof(prompt), "Response #%d: ", i + 1);
    if (n <= 0 || (size_t) n >= sizeof(prompt)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to print response prompt");
      goto out;
    }

    b64_cdh = converse(pamh, PAM_PROMPT_ECHO_ON, prompt);
    b64_rpid = converse(pamh, PAM_PROMPT_ECHO_ON, prompt);
    b64_authdata = converse(pamh, PAM_PROMPT_ECHO_ON, prompt);
    b64_sig = converse(pamh, PAM_PROMPT_ECHO_ON, prompt);

    if (!b64_decode(b64_authdata, (void **) &authdata, &authdata_len)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to decode authenticator data");
      goto out;
    }

    if (!b64_decode(b64_sig, (void **) &sig, &sig_len)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to decode signature");
      goto out;
    }

    free(b64_cdh);
    free(b64_rpid);
    free(b64_authdata);
    free(b64_sig);

    b64_cdh = NULL;
    b64_rpid = NULL;
    b64_authdata = NULL;
    b64_sig = NULL;

    r = fido_assert_set_count(assert[i], 1);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to set signature count of assertion %u", i);
      goto out;
    }

    r = fido_assert_set_authdata(assert[i], 0, authdata, authdata_len);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to set authdata of assertion %u", i);
      goto out;
    }

    r = fido_assert_set_sig(assert[i], 0, sig, sig_len);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to set signature of assertion %u", i);
      goto out;
    }

    free(authdata);
    free(sig);

    authdata = NULL;
    sig = NULL;

    if (cose_type[i] == COSE_ES256)
      r = fido_assert_verify(assert[i], 0, COSE_ES256, es256_pk[i]);
    else
      r = fido_assert_verify(assert[i], 0, COSE_RS256, rs256_pk[i]);

    if (r == FIDO_OK) {
      retval = 1;
      break;
    }
  }

out:
  for (i = 0; i < n_devs; i++) {
    fido_assert_free(&assert[i]);
    es256_pk_free(&es256_pk[i]);
    rs256_pk_free(&rs256_pk[i]);
  }

  free(kh);
  free(pk);
  free(b64_challenge);
  free(b64_cdh);
  free(b64_rpid);
  free(b64_authdata);
  free(b64_sig);
  free(authdata);
  free(sig);

  return retval;
}

static int _converse(pam_handle_t *pamh, int nargs,
                     const struct pam_message **message,
                     struct pam_response **response) {
  struct pam_conv *conv;
  int retval;

  retval = pam_get_item(pamh, PAM_CONV, (void *) &conv);

  if (retval != PAM_SUCCESS) {
    return retval;
  }

  return conv->conv(nargs, message, response, conv->appdata_ptr);
}

char *converse(pam_handle_t *pamh, int echocode, const char *prompt) {
  const struct pam_message msg = {.msg_style = echocode,
                                  .msg = (char *)(uintptr_t)prompt};
  const struct pam_message *msgs = &msg;
  struct pam_response *resp = NULL;
  int retval = _converse(pamh, 1, &msgs, &resp);
  char *ret = NULL;

  if (retval != PAM_SUCCESS || resp == NULL || resp->resp == NULL ||
      *resp->resp == '\000') {

    if (retval == PAM_SUCCESS && resp && resp->resp) {
      ret = resp->resp;
    }
  } else {
    ret = resp->resp;
  }

  // Deallocate temporary storage.
  if (resp) {
    if (!ret) {
      free(resp->resp);
    }
    free(resp);
  }

  return ret;
}

#if defined(PAM_DEBUG)
void _debug(FILE *debug_file, const char *file, int line, const char *func,
            const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
#ifdef LOG_DEBUG
  if (debug_file == (FILE *) -1) {
    syslog(LOG_AUTHPRIV | LOG_DEBUG, DEBUG_STR, file, line, func);
    vsyslog(LOG_AUTHPRIV | LOG_DEBUG, fmt, ap);
  } else {
    fprintf(debug_file, DEBUG_STR, file, line, func);
    vfprintf(debug_file, fmt, ap);
    fprintf(debug_file, "\n");
  }
#else  /* Windows, MAC */
  fprintf(debug_file, DEBUG_STR, file, line, func);
  vfprintf(debug_file, fmt, ap);
  fprintf(debug_file, "\n");
#endif /* __linux__ */
  va_end(ap);
}
#endif /* PAM_DEBUG */

#ifndef RANDOM_DEV
#define RANDOM_DEV "/dev/urandom"
#endif

int random_bytes(void *buf, size_t cnt) {
  int fd;
  ssize_t n;

  fd = open(RANDOM_DEV, O_RDONLY);
  if (fd < 0)
    return (0);

  n = read(fd, buf, cnt);
  close(fd);
  if (n < 0 || (size_t) n != cnt)
    return (0);

  return (1);
}
