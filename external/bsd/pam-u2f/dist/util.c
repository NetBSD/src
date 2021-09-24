/*
 * Copyright (C) 2014-2019 Yubico AB - See COPYING
 */

#include <fido.h>
#include <fido/es256.h>
#include <fido/rs256.h>
#include <fido/eddsa.h>

#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <syslog.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "b64.h"
#include "util.h"

#define SSH_HEADER "-----BEGIN OPENSSH PRIVATE KEY-----\n"
#define SSH_HEADER_LEN (sizeof(SSH_HEADER) - 1)
#define SSH_TRAILER "-----END OPENSSH PRIVATE KEY-----\n"
#define SSH_TRAILER_LEN (sizeof(SSH_TRAILER) - 1)
#define SSH_AUTH_MAGIC "openssh-key-v1"
#define SSH_AUTH_MAGIC_LEN (sizeof(SSH_AUTH_MAGIC)) // AUTH_MAGIC includes \0
#define SSH_ES256 "sk-ecdsa-sha2-nistp256@openssh.com"
#define SSH_ES256_LEN (sizeof(SSH_ES256) - 1)
#define SSH_ES256_POINT_LEN 65
#define SSH_P256_NAME "nistp256"
#define SSH_P256_NAME_LEN (sizeof(SSH_P256_NAME) - 1)
#define SSH_EDDSA "sk-ssh-ed25519@openssh.com"
#define SSH_EDDSA_LEN (sizeof(SSH_EDDSA) - 1)
#define SSH_EDDSA_POINT_LEN 32
#define SSH_SK_USER_PRESENCE_REQD 0x01
#define SSH_SK_USER_VERIFICATION_REQD 0x04
#define SSH_SK_RESIDENT_KEY 0x20

struct opts {
  fido_opt_t up;
  fido_opt_t uv;
  fido_opt_t pin;
};

struct pk {
  void *ptr;
  int type;
};

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

static int translate_old_format_pubkey(es256_pk_t *es256_pk,
                                       const unsigned char *pk, size_t pk_len) {
  EC_KEY *ec = NULL;
  EC_POINT *q = NULL;
  const EC_GROUP *g = NULL;
  int r = FIDO_ERR_INTERNAL;

  if (es256_pk == NULL)
    goto fail;

  if ((ec = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) == NULL ||
      (g = EC_KEY_get0_group(ec)) == NULL)
    goto fail;

  if ((q = EC_POINT_new(g)) == NULL ||
      !EC_POINT_oct2point(g, q, pk, pk_len, NULL) ||
      !EC_KEY_set_public_key(ec, q))
    goto fail;

  r = es256_pk_from_EC_KEY(es256_pk, ec);

fail:
  if (ec != NULL)
    EC_KEY_free(ec);
  if (q != NULL)
    EC_POINT_free(q);

  return r;
}

static int is_resident(const char *kh) { return strcmp(kh, "*") == 0; }

static void reset_device(device_t *device) {
  free(device->keyHandle);
  free(device->publicKey);
  free(device->coseType);
  free(device->attributes);
  memset(device, 0, sizeof(*device));
}

static int parse_native_format(const cfg_t *cfg, const char *username,
                               char *buf, FILE *opwfile, device_t *devices,
                               unsigned *n_devs) {

  char *s_user, *s_credential;
  const char *s_token;
  unsigned i;

  while (fgets(buf, (int) (DEVSIZE * (cfg->max_devs - 1)), opwfile)) {
    char *saveptr = NULL;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
      buf[len - 1] = '\0';

    if (cfg->debug)
      D(cfg->debug_file, "Authorization line: %s", buf);

    s_user = strtok_r(buf, ":", &saveptr);
    if (s_user && strcmp(username, s_user) == 0) {
      if (cfg->debug)
        D(cfg->debug_file, "Matched user: %s", s_user);

      // only keep last line for this user
      for (i = 0; i < *n_devs; i++) {
        reset_device(&devices[i]);
      }
      *n_devs = 0;

      i = 0;
      while ((s_credential = strtok_r(NULL, ":", &saveptr)) != NULL) {
        // s_credential is the whole line now
        char *credsaveptr = NULL;

        if ((*n_devs)++ > cfg->max_devs - 1) {
          *n_devs = cfg->max_devs;
          if (cfg->debug) {
            D(cfg->debug_file,
              "Found more than %d devices, ignoring the remaining ones",
              cfg->max_devs);
          }
          break;
        }

        reset_device(&devices[i]);

        s_token = strtok_r(s_credential, ",", &credsaveptr);

        if (!s_token) {
          if (cfg->debug) {
            D(cfg->debug_file,
              "Unable to retrieve keyHandle for device %d", i + 1);
          }
          return -1;
        }

        if (cfg->debug) {
          D(cfg->debug_file, "KeyHandle for device number %d: %s", i + 1,
            s_token);
        }

        devices[i].keyHandle = strdup(s_token);

        if (!devices[i].keyHandle) {
          if (cfg->debug) {
            D(cfg->debug_file,
              "Unable to allocate memory for keyHandle number %d", i);
          }
          return -1;
        }

        if (is_resident(devices[i].keyHandle) && cfg->debug) {
          D(cfg->debug_file, "Credential is resident");
        }

        s_token = strtok_r(NULL, ",", &credsaveptr);

        if (!s_token) {
          if (cfg->debug) {
            D(cfg->debug_file, "Unable to retrieve publicKey number %d", i + 1);
          }
          return -1;
        }

        if (cfg->debug) {
          D(cfg->debug_file, "publicKey for device number %d: %s", i + 1,
            s_token);
        }

        devices[i].publicKey = strdup(s_token);

        if (!devices[i].publicKey) {
          if (cfg->debug) {
            D(cfg->debug_file,
              "Unable to allocate memory for publicKey number %d", i);
          }
          return -1;
        }

        s_token = strtok_r(NULL, ",", &credsaveptr);

        if (!s_token) {
          if (cfg->debug) {
            D(cfg->debug_file, "Unable to retrieve COSE type %d", i + 1);
            D(cfg->debug_file, "Assuming ES256 (backwards compatibility)");
          }
          devices[i].old_format = 1;
          devices[i].coseType = strdup("es256");
        } else {
          if (cfg->debug) {
            D(cfg->debug_file, "COSE type for device number %d: %s", i + 1,
              s_token);
          }
          devices[i].coseType = strdup(s_token);
        }

        if (!devices[i].coseType) {
          if (cfg->debug) {
            D(cfg->debug_file,
              "Unable to allocate memory for COSE type number %d", i);
          }
          return -1;
        }

        s_token = strtok_r(NULL, ",", &credsaveptr);

        if (devices[i].old_format == 1) {
          if (cfg->debug) {
            D(cfg->debug_file, "Old format for device %d, no attributes",
              i + 1);
            D(cfg->debug_file, "Assuming 'presence' (backwards compatibility)");
          }
          s_token = "+presence";
        } else if (!s_token) {
          s_token = "";
        }

        if (cfg->debug) {
          D(cfg->debug_file, "Attributes for device number %d: %s", i + 1,
            s_token);
        }
        devices[i].attributes = strdup(s_token);

        if (!devices[i].attributes) {
          if (cfg->debug) {
            D(cfg->debug_file,
              "Unable to allocate memory for attributes number %d", i);
          }
          return -1;
        }

        if (devices[i].old_format) {
          char *websafe_b64 = devices[i].keyHandle;
          devices[i].keyHandle = normal_b64(websafe_b64);
          free(websafe_b64);
          if (!devices[i].keyHandle) {
            if (cfg->debug) {
              D(cfg->debug_file,
                "Unable to allocate memory for keyHandle number %d", i);
            }
            return -1;
          }
        }

        i++;
      }
    }
  }

  return 1;
}

static int load_ssh_key(const cfg_t *cfg, char *buf, size_t buf_size,
                        FILE *opwfile, size_t opwfile_size) {
  char *cp = buf;
  int ch;

  if (opwfile_size > buf_size ||
      opwfile_size < SSH_HEADER_LEN + SSH_TRAILER_LEN) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (length)");
    }
    return 0;
  }

  // NOTE(adma): +1 for \0
  if (fgets(buf, (int)(SSH_HEADER_LEN + 1), opwfile) == NULL ||
      strlen(buf) != SSH_HEADER_LEN ||
      strncmp(buf, SSH_HEADER, SSH_HEADER_LEN) != 0) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (header)");
    }
    return 0;
  }

  while (opwfile_size > 0 && buf_size > 1) {
    ch = fgetc(opwfile);
    if (ch == EOF) {
      if (cfg->debug) {
        D(cfg->debug_file, "Unexpected authfile termination");
      }
      return 0;
    }

    opwfile_size--;
    buf_size--;

    if (ch != '\n' && ch != '\r') {
      *cp = (char) ch;
      if (ch == '-') {
        // NOTE(adma): no +1 here since we already read one '-'
        if (buf_size < SSH_TRAILER_LEN ||
            fgets(cp + 1, (int)SSH_TRAILER_LEN, opwfile) == NULL ||
            strlen(cp) != SSH_TRAILER_LEN ||
            strncmp(cp, SSH_TRAILER, SSH_TRAILER_LEN) != 0) {
          if (cfg->debug) {
            D(cfg->debug_file, "Malformed SSH key (trailer)");
          }
          return 0;
        }

        *(cp) = '\0';
        break;
      } else {
        cp++;
      }
    }
  }

  if (cfg->debug) { // TODO(adma): too verbose? Delete?
    D(cfg->debug_file, "Credential is \"%s\"", buf);
  }

  return 1;
}

static int ssh_get(const unsigned char **buf, size_t *size, unsigned char *dst,
                   size_t len) {
  if (*size < len)
    return 0;
  if (dst != NULL)
    memcpy(dst, *buf, len);
  *buf += len;
  *size -= len;
  return 1;
}

static int ssh_get_u8(const unsigned char **buf, size_t *size, uint8_t *val) {
  return ssh_get(buf, size, val, sizeof(*val));
}

static int ssh_get_u32(const unsigned char **buf, size_t *size, uint32_t *val) {
  if (!ssh_get(buf, size, (unsigned char *) val, sizeof(*val)))
    return 0;
  if (val != NULL)
    *val = ntohl(*val);
  return 1;
}

static int ssh_get_string_ref(const unsigned char **buf, size_t *size,
                              const unsigned char **ref, size_t *lenp) {
  uint32_t len;

  if (!ssh_get_u32(buf, size, &len))
    return 0;
  if (!ssh_get(buf, size, NULL, len))
    return 0;
  if (ref != NULL)
    *ref = *buf - len;
  if (lenp != NULL)
    *lenp = len;
  return 1;
}

static int ssh_get_cstring(const unsigned char **buf, size_t *size, char **str,
                           size_t *lenp) {
  const unsigned char *ref;
  size_t len;

  if (!ssh_get_string_ref(buf, size, &ref, &len))
    return 0;
  if (str != NULL) {
    if (len > SIZE_MAX - 1 || (*str = calloc(1, len + 1)) == NULL)
      return 0;
    memcpy(*str, ref, len);
  }
  if (lenp != NULL)
    *lenp = len;
  return 1;
}

static int ssh_log_cstring(const cfg_t *cfg, const unsigned char **buf,
                           size_t *size, const char *name) {
  char *str = NULL;
  size_t len;

  (void) name; // silence compiler warnings if PAM_DEBUG disabled

  if (!ssh_get_cstring(buf, size, &str, &len)) {
    if (cfg->debug)
      D(cfg->debug_file, "Malformed SSH key (%s)", name);
    return 0;
  }
  if (cfg->debug)
    D(cfg->debug_file, "%s (%zu) \"%s\"", name, len, str);

  free(str);
  return 1;
}

static int ssh_get_attrs(const cfg_t *cfg, const unsigned char **buf,
                         size_t *size, char **attrs) {
  char tmp[32] = {0};
  uint8_t flags;
  int r;

  // flags
  if (!ssh_get_u8(buf, size, &flags)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (flags)");
    }
    return 0;
  }
  if (cfg->debug) {
    D(cfg->debug_file, "flags: %02x", flags);
  }

  r = snprintf(tmp, sizeof(tmp), "%s%s",
               flags & SSH_SK_USER_PRESENCE_REQD ? "+presence" : "",
               flags & SSH_SK_USER_VERIFICATION_REQD ? "+verification" : "");
  if (r < 0 || (size_t) r >= sizeof(tmp)) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to prepare flags");
    return 0;
  }

  if ((*attrs = strdup(tmp)) == NULL) {
    if (cfg->debug) {
      D(cfg->debug_file, "Unable to allocate attributes");
    }
    return 0;
  }

  return 1;
}

static int ssh_get_pubkey(const cfg_t *cfg, const unsigned char **buf,
                          size_t *size, char **type_p, char **pubkey_p) {
  char *ssh_type = NULL;
  char *ssh_curve = NULL;
  const unsigned char *blob;
  size_t len;
  int type;
  size_t point_len;
  int ok = 0;

  *type_p = NULL;
  *pubkey_p = NULL;

  // key type
  if (!ssh_get_cstring(buf, size, &ssh_type, &len)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (keytype)");
    }
    goto err;
  }

  if (len == SSH_ES256_LEN && memcmp(ssh_type, SSH_ES256, SSH_ES256_LEN) == 0) {
    type = COSE_ES256;
    point_len = SSH_ES256_POINT_LEN;
  } else if (len == SSH_EDDSA_LEN &&
             memcmp(ssh_type, SSH_EDDSA, SSH_EDDSA_LEN) == 0) {
    type = COSE_EDDSA;
    point_len = SSH_EDDSA_POINT_LEN;
  } else {
    if (cfg->debug) {
      D(cfg->debug_file, "Unknown key type %s", ssh_type);
    }
    goto err;
  }

  if (cfg->debug) {
    D(cfg->debug_file, "keytype (%zu) \"%s\"", len, ssh_type);
  }

  if (type == COSE_ES256) {
    // curve name
    if (!ssh_get_cstring(buf, size, &ssh_curve, &len)) {
      if (cfg->debug) {
        D(cfg->debug_file, "Malformed SSH key (curvename)");
      }
      goto err;
    }

    if (len == SSH_P256_NAME_LEN &&
        memcmp(ssh_curve, SSH_P256_NAME, SSH_P256_NAME_LEN) == 0) {
      if (cfg->debug) {
        D(cfg->debug_file, "curvename (%zu) \"%s\"", len, ssh_curve);
      }
    } else {
      if (cfg->debug) {
        D(cfg->debug_file, "Unknown curve %s", ssh_curve);
      }
      goto err;
    }
  }

  // point
  if (!ssh_get_string_ref(buf, size, &blob, &len)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (point)");
    }
    goto err;
  }

  if (len != point_len) {
    if (cfg->debug) {
      D(cfg->debug_file, "Invalid point length, should be %zu, found %zu",
        point_len, len);
    }
    goto err;
  }

  if (type == COSE_ES256) {
    // Skip the initial '04'
    if (len < 1) {
      if (cfg->debug) {
        D(cfg->debug_file, "Failed to skip initial '04'");
      }
      goto err;
    }
    blob++;
    len--;
  }

  if (!b64_encode(blob, len, pubkey_p)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Unable to allocate public key");
    }
    goto err;
  }

  if ((*type_p = strdup(cose_string(type))) == NULL) {
    if (cfg->debug) {
      D(cfg->debug_file, "Unable to allocate COSE type");
    }
    goto err;
  }

  ok = 1;
err:
  if (!ok) {
    free(*type_p);
    free(*pubkey_p);
    *type_p = NULL;
    *pubkey_p = NULL;
  }
  free(ssh_type);
  free(ssh_curve);

  return ok;
}

static int parse_ssh_format(const cfg_t *cfg, char *buf, size_t buf_size,
                            FILE *opwfile, size_t opwfile_size,
                            device_t *devices, unsigned *n_devs) {

  const unsigned char *decoded;
  unsigned char *decoded_initial = NULL;
  size_t decoded_len;
  const unsigned char *blob;
  uint32_t check1, check2, tmp;
  size_t len;

  // The logic below is inspired by
  // how ssh parses its own keys. See sshkey.c
  reset_device(&devices[0]);

  if (!load_ssh_key(cfg, buf, buf_size, opwfile, opwfile_size) ||
      !b64_decode(buf, (void **) &decoded_initial, &decoded_len)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Unable to decode credential");
    }
    goto out;
  }

  decoded = decoded_initial;

  // magic
  if (decoded_len < SSH_AUTH_MAGIC_LEN ||
      memcmp(decoded, SSH_AUTH_MAGIC, SSH_AUTH_MAGIC_LEN) != 0) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (magic)");
    }
    goto out;
  }

  decoded += SSH_AUTH_MAGIC_LEN;
  decoded_len -= SSH_AUTH_MAGIC_LEN;

  if (!ssh_log_cstring(cfg, &decoded, &decoded_len, "ciphername") ||
      !ssh_log_cstring(cfg, &decoded, &decoded_len, "kdfname") ||
      !ssh_log_cstring(cfg, &decoded, &decoded_len, "kdfoptions"))
    goto out;

  if (!ssh_get_u32(&decoded, &decoded_len, &tmp)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (nkeys)");
    }
    goto out;
  }
  if (cfg->debug) {
    D(cfg->debug_file, "nkeys: %" PRIu32, tmp);
  }
  if (tmp != 1) {
    if (cfg->debug) {
      D(cfg->debug_file, "Multiple keys not supported");
    }
    goto out;
  }

  // public_key (skip)
  if (!ssh_get_string_ref(&decoded, &decoded_len, NULL, NULL)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (pubkey)");
    }
    goto out;
  }

  // private key (consume length)
  if (!ssh_get_u32(&decoded, &decoded_len, &tmp) || decoded_len < tmp) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (pvtkey length)");
    }
    goto out;
  }

  // check1, check2
  if (!ssh_get_u32(&decoded, &decoded_len, &check1) ||
      !ssh_get_u32(&decoded, &decoded_len, &check2)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (check1, check2)");
    }
    goto out;
  }
  if (cfg->debug) {
    D(cfg->debug_file, "check1: %" PRIu32, check1);
    D(cfg->debug_file, "check2: %" PRIu32, check2);
  }
  if (check1 != check2) {
    if (cfg->debug) {
      D(cfg->debug_file, "Mismatched check values");
      goto out;
    }
  }

  if (!ssh_get_pubkey(cfg, &decoded, &decoded_len, &devices[0].coseType,
                      &devices[0].publicKey) ||
      !ssh_log_cstring(cfg, &decoded, &decoded_len, "application") ||
      !ssh_get_attrs(cfg, &decoded, &decoded_len, &devices[0].attributes))
    goto out;

  // keyhandle
  if (!ssh_get_string_ref(&decoded, &decoded_len, &blob, &len) ||
      !b64_encode(blob, len, &devices[0].keyHandle)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (keyhandle)");
    }
    goto out;
  }

  if (cfg->debug) {
    D(cfg->debug_file, "KeyHandle for device number 1: %s",
      devices[0].keyHandle);
    D(cfg->debug_file, "publicKey for device number 1: %s",
      devices[0].publicKey);
    D(cfg->debug_file, "COSE type for device number 1: %s",
      devices[0].coseType);
    D(cfg->debug_file, "Attributes for device number 1: %s",
      devices[0].attributes);
  }

  // reserved (skip)
  if (!ssh_get_string_ref(&decoded, &decoded_len, NULL, NULL)) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (reserved)");
    }
    goto out;
  }

  // comment
  if (!ssh_log_cstring(cfg, &decoded, &decoded_len, "comment"))
    goto out;

  // padding
  if (decoded_len >= 255) {
    if (cfg->debug) {
      D(cfg->debug_file, "Malformed SSH key (padding length)");
    }
    goto out;
  }

  for (int i = 1; (unsigned) i <= decoded_len; i++) {
    if (decoded[i - 1] != i) {
      if (cfg->debug) {
        D(cfg->debug_file, "Malformed SSH key (padding)");
      }
      goto out;
    }
  }

  free(decoded_initial);
  decoded_initial = NULL;

  *n_devs = 1;

  return 1;

out:
  reset_device(&devices[0]);

  if (decoded_initial) {
    free(decoded_initial);
    decoded_initial = NULL;
  }

  return -1;
}

int get_devices_from_authfile(const cfg_t *cfg, const char *username,
                              device_t *devices, unsigned *n_devs) {

  char *buf = NULL;
  int retval = 0;
  int fd = -1;
  struct stat st;
  struct passwd *pw = NULL, pw_s;
  char buffer[BUFSIZE];
  int gpu_ret;
  FILE *opwfile = NULL;
  size_t opwfile_size;
  unsigned i;

  /* Ensure we never return uninitialized count. */
  *n_devs = 0;

  fd = open(cfg->auth_file, O_RDONLY | O_CLOEXEC | O_NOCTTY);
  if (fd < 0) {
    if (cfg->debug)
      D(cfg->debug_file, "Cannot open file: %s (%s)", cfg->auth_file,
        strerror(errno));
    goto err;
  }

  if (fstat(fd, &st) < 0) {
    if (cfg->debug)
      D(cfg->debug_file, "Cannot stat file: %s (%s)", cfg->auth_file,
        strerror(errno));
    goto err;
  }

  if (!S_ISREG(st.st_mode)) {
    if (cfg->debug)
      D(cfg->debug_file, "%s is not a regular file", cfg->auth_file);
    goto err;
  }

  if (st.st_size == 0) {
    if (cfg->debug)
      D(cfg->debug_file, "File %s is empty", cfg->auth_file);
    goto err;
  }
  opwfile_size = (size_t)st.st_size;

  gpu_ret = getpwuid_r(st.st_uid, &pw_s, buffer, sizeof(buffer), &pw);
  if (gpu_ret != 0 || pw == NULL) {
    D(cfg->debug_file, "Unable to retrieve credentials for uid %u, (%s)",
      st.st_uid, strerror(errno));
    goto err;
  }

  if (strcmp(pw->pw_name, username) != 0 && strcmp(pw->pw_name, "root") != 0) {
    if (strcmp(username, "root") != 0) {
      D(cfg->debug_file,
        "The owner of the authentication file is neither %s nor root",
        username);
    } else {
      D(cfg->debug_file, "The owner of the authentication file is not root");
    }
    goto err;
  }

  opwfile = fdopen(fd, "r");
  if (opwfile == NULL) {
    if (cfg->debug)
      D(cfg->debug_file, "fdopen: %s", strerror(errno));
    goto err;
  } else {
    fd = -1; /* fd belongs to opwfile */
  }

  buf = calloc(1, (DEVSIZE * cfg->max_devs));
  if (!buf) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to allocate memory");
    goto err;
  }

  if (cfg->sshformat == 0) {
    retval = parse_native_format(cfg, username, buf, opwfile, devices, n_devs);
  } else {
    retval = parse_ssh_format(cfg, buf, DEVSIZE * cfg->max_devs, opwfile,
                              opwfile_size, devices, n_devs);
  }

  if (retval != 1) {
    // NOTE(adma): error message is logged by the previous function
    goto err;
  }

  if (cfg->debug)
    D(cfg->debug_file, "Found %d device(s) for user %s", *n_devs, username);

  retval = 1;
  goto out;

err:
  for (i = 0; i < *n_devs; i++) {
    reset_device(&devices[i]);
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
    reset_device(&devices[i]);
  }

  free(devices);
  devices = NULL;
}

static int get_authenticators(const cfg_t *cfg, const fido_dev_info_t *devlist,
                              size_t devlist_len, fido_assert_t *assert,
                              const int rk, fido_dev_t **authlist) {
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

    if (rk || cfg->nodetect) {
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

  if (j != 0)
    return (1);
  else {
    if (cfg->debug)
      D(cfg->debug_file, "Key not found");
    return (0);
  }
}

static void init_opts(struct opts *opts) {
  opts->up = FIDO_OPT_FALSE;
  opts->uv = FIDO_OPT_OMIT;
  opts->pin = FIDO_OPT_FALSE;
}

static void parse_opts(const cfg_t *cfg, const char *attr, struct opts *opts) {
  if (cfg->userpresence == 1 || strstr(attr, "+presence")) {
    opts->up = FIDO_OPT_TRUE;
  } else if (cfg->userpresence == 0) {
    opts->up = FIDO_OPT_FALSE;
  } else {
    opts->up = FIDO_OPT_OMIT;
  }

  if (cfg->userverification == 1 || strstr(attr, "+verification")) {
    opts->uv = FIDO_OPT_TRUE;
  } else if (cfg->userverification == 0)
    opts->uv = FIDO_OPT_FALSE;
  else {
    opts->uv = FIDO_OPT_OMIT;
  }

  if (cfg->pinverification == 1 || strstr(attr, "+pin")) {
    opts->pin = FIDO_OPT_TRUE;
  } else if (cfg->pinverification == 0) {
    opts->pin = FIDO_OPT_FALSE;
  } else {
    opts->pin = FIDO_OPT_OMIT;
  }
}

static int get_device_opts(fido_dev_t *dev, int *pin, int *uv) {
  fido_cbor_info_t *info = NULL;
  char *const *ptr;
  const bool *val;
  size_t len;

  *pin = *uv = -1; /* unsupported */

  if (fido_dev_is_fido2(dev)) {
    if ((info = fido_cbor_info_new()) == NULL ||
        fido_dev_get_cbor_info(dev, info) != FIDO_OK) {
      fido_cbor_info_free(&info);
      return 0;
    }

    ptr = fido_cbor_info_options_name_ptr(info);
    val = fido_cbor_info_options_value_ptr(info);
    len = fido_cbor_info_options_len(info);
    for (size_t i = 0; i < len; i++) {
      if (strcmp(ptr[i], "clientPin") == 0) {
        *pin = val[i];
      } else if (strcmp(ptr[i], "uv") == 0) {
        *uv = val[i];
      }
    }
  }

  fido_cbor_info_free(&info);
  return 1;
}

static int match_device_opts(fido_dev_t *dev, struct opts *opts) {
  int pin, uv;

  /* FIXME: fido_dev_{supports,has}_{pin,uv} (1.7.0) */
  if (!get_device_opts(dev, &pin, &uv)) {
    return -1;
  }

  if (opts->uv == FIDO_OPT_FALSE && uv < 0) {
    opts->uv = FIDO_OPT_OMIT;
  }

  if ((opts->pin == FIDO_OPT_TRUE && pin != 1) ||
      (opts->uv == FIDO_OPT_TRUE && uv != 1)) {
    return 0;
  }

  return 1;
}

static int set_opts(const cfg_t *cfg, const struct opts *opts,
                    fido_assert_t *assert) {
  if (fido_assert_set_up(assert, opts->up) != FIDO_OK) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to set UP");
    return 0;
  }
  if (fido_assert_set_uv(assert, opts->uv) != FIDO_OK) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to set UV");
    return 0;
  }

  return 1;
}

static int set_cdh(const cfg_t *cfg, fido_assert_t *assert) {
  unsigned char cdh[32];
  int r;

  if (!random_bytes(cdh, sizeof(cdh))) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to generate challenge");
    return 0;
  }

  r = fido_assert_set_clientdata_hash(assert, cdh, sizeof(cdh));
  if (r != FIDO_OK) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to set challenge: %s (%d)", fido_strerr(r), r);
    return 0;
  }

  return 1;
}

static fido_assert_t *prepare_assert(const cfg_t *cfg, const device_t *device,
                                     const struct opts *opts) {
  fido_assert_t *assert = NULL;
  unsigned char *buf = NULL;
  size_t buf_len;
  int ok = 0;
  int r;

  if ((assert = fido_assert_new()) == NULL) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to allocate assertion");
    goto err;
  }

  if (device->old_format && strcmp(cfg->origin, cfg->appid) != 0)
    r = fido_assert_set_rp(assert, cfg->appid);
  else
    r = fido_assert_set_rp(assert, cfg->origin);

  if (r != FIDO_OK) {
    if (cfg->debug)
      D(cfg->debug_file, "Unable to set origin: %s (%d)", fido_strerr(r), r);
    goto err;
  }

  if (is_resident(device->keyHandle)) {
    if (cfg->debug)
      D(cfg->debug_file, "Credential is resident");
  } else {
    if (cfg->debug)
      D(cfg->debug_file, "Key handle: %s", device->keyHandle);
    if (!b64_decode(device->keyHandle, (void **) &buf, &buf_len)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to decode key handle");
      goto err;
    }

    r = fido_assert_allow_cred(assert, buf, buf_len);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to set keyHandle: %s (%d)", fido_strerr(r),
          r);
      goto err;
    }
  }

  if (!set_opts(cfg, opts, assert)) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to set assert options");
    goto err;
  }

  if (!set_cdh(cfg, assert)) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to set client data hash");
    goto err;
  }

  ok = 1;

err:
  if (!ok)
    fido_assert_free(&assert);

  free(buf);

  return assert;
}

static void reset_pk(struct pk *pk) {
  if (pk->type == COSE_ES256) {
    es256_pk_free((es256_pk_t **) &pk->ptr);
  } else if (pk->type == COSE_RS256) {
    rs256_pk_free((rs256_pk_t **) &pk->ptr);
  } else if (pk->type == COSE_EDDSA) {
    eddsa_pk_free((eddsa_pk_t **) &pk->ptr);
  }
  memset(pk, 0, sizeof(*pk));
}

int cose_type(const char *str, int *type) {
  if (strcasecmp(str, "es256") == 0) {
    *type = COSE_ES256;
  } else if (strcasecmp(str, "rs256") == 0) {
    *type = COSE_RS256;
  } else if (strcasecmp(str, "eddsa") == 0) {
    *type = COSE_EDDSA;
  } else {
    *type = 0;
    return 0;
  }

  return 1;
}

const char *cose_string(int type) {
  switch (type) {
    case COSE_ES256:
      return "es256";
    case COSE_RS256:
      return "rs256";
    case COSE_EDDSA:
      return "eddsa";
    default:
      return "unknown";
  }
}

static int parse_pk(const cfg_t *cfg, int old, const char *type, const char *pk,
                    struct pk *out) {
  unsigned char *buf = NULL;
  size_t buf_len;
  int ok = 0;
  int r;

  reset_pk(out);

  if (old) {
    if (!hex_decode(pk, &buf, &buf_len)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to decode public key");
      goto err;
    }
  } else {
    if (!b64_decode(pk, (void **) &buf, &buf_len)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to decode public key");
      goto err;
    }
  }

  if (!cose_type(type, &out->type)) {
    if (cfg->debug)
      D(cfg->debug_file, "Unknown COSE type '%s'", type);
    goto err;
  }

  // For backwards compatibility, failure to pack the public key is not
  // returned as an error.  Instead, it is handled by fido_verify_assert().
  if (out->type == COSE_ES256) {
    if ((out->ptr = es256_pk_new()) == NULL) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to allocate ES256 public key");
      goto err;
    }
    if (old) {
      r = translate_old_format_pubkey(out->ptr, buf, buf_len);
    } else {
      r = es256_pk_from_ptr(out->ptr, buf, buf_len);
    }
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to convert ES256 public key");
    }
  } else if (out->type == COSE_RS256) {
    if ((out->ptr = rs256_pk_new()) == NULL) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to allocate RS256 public key");
      goto err;
    }
    r = rs256_pk_from_ptr(out->ptr, buf, buf_len);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to convert RS256 public key");
    }
  } else if (out->type == COSE_EDDSA) {
    if ((out->ptr = eddsa_pk_new()) == NULL) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to allocate EDDSA public key");
      goto err;
    }
    r = eddsa_pk_from_ptr(out->ptr, buf, buf_len);
    if (r != FIDO_OK) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to convert EDDSA public key");
    }
  } else {
    if (cfg->debug)
      D(cfg->debug_file, "COSE type '%s' not handled", type);
    goto err;
  }

  ok = 1;
err:
  free(buf);

  return ok;
}

int do_authentication(const cfg_t *cfg, const device_t *devices,
                      const unsigned n_devs, pam_handle_t *pamh) {
  fido_assert_t *assert = NULL;
  fido_dev_info_t *devlist = NULL;
  fido_dev_t **authlist = NULL;
  int cued = 0;
  int r;
  int retval = -2;
  size_t ndevs = 0;
  size_t ndevs_prev = 0;
  unsigned i = 0;
  struct opts opts;
  struct pk pk;
  char *pin = NULL;

  init_opts(&opts);
#ifndef WITH_FUZZING
  fido_init(cfg->debug ? FIDO_DEBUG : 0);
#else
  fido_init(0);
#endif
  memset(&pk, 0, sizeof(pk));

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
    D(cfg->debug_file, "Device max index is %zu", ndevs);

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

    init_opts(&opts); /* used during authenticator discovery */
    assert = prepare_assert(cfg, &devices[i], &opts);
    if (assert == NULL) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to prepare assert");
      goto out;
    }

    if (!parse_pk(cfg, devices[i].old_format, devices[i].coseType,
                  devices[i].publicKey, &pk)) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to parse public key");
      goto out;
    }

    if (get_authenticators(cfg, devlist, ndevs, assert,
                           is_resident(devices[i].keyHandle), authlist)) {
      for (size_t j = 0; authlist[j] != NULL; j++) {
        /* options used during authentication */
        parse_opts(cfg, devices[i].attributes, &opts);

        r = match_device_opts(authlist[j], &opts);
        if (r != 1) {
          if (cfg->debug) {
            D(cfg->debug_file, "%s, skipping authenticator",
              r < 0 ? "Failed to query supported options"
                    : "Unsupported options");
          }
          continue;
        }

        if (!set_opts(cfg, &opts, assert)) {
          if (cfg->debug)
            D(cfg->debug_file, "Failed to set assert options");
          goto out;
        }

        if (!set_cdh(cfg, assert)) {
          if (cfg->debug)
            D(cfg->debug_file, "Failed to reset client data hash");
          goto out;
        }

        if (opts.pin == FIDO_OPT_TRUE) {
          pin = converse(pamh, PAM_PROMPT_ECHO_OFF, "Please enter the PIN: ");
          if (pin == NULL) {
            D(cfg->debug_file, "converse() returned NULL");
            goto out;
          }
        }
        if (opts.up == FIDO_OPT_TRUE || opts.uv == FIDO_OPT_TRUE) {
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
          if (opts.pin == FIDO_OPT_TRUE || opts.uv == FIDO_OPT_TRUE) {
            r = fido_assert_set_uv(assert, FIDO_OPT_TRUE);
            if (r != FIDO_OK) {
              D(cfg->debug_file, "Failed to set UV");
              goto out;
            }
          }
          r = fido_assert_verify(assert, 0, pk.type, pk.ptr);
          if (r == FIDO_OK) {
            retval = 1;
            goto out;
          }
        }
      }
    } else {
      if (cfg->debug)
        D(cfg->debug_file, "Device for this keyhandle is not present");
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

    for (size_t j = 0; authlist[j] != NULL; j++) {
      fido_dev_close(authlist[j]);
      fido_dev_free(&authlist[j]);
    }

    fido_assert_free(&assert);
  }

out:
  reset_pk(&pk);
  fido_assert_free(&assert);
  fido_dev_info_free(&devlist, ndevs);

  if (authlist) {
    for (size_t j = 0; authlist[j] != NULL; j++) {
      fido_dev_close(authlist[j]);
      fido_dev_free(&authlist[j]);
    }
    free(authlist);
  }

  return retval;
}

#define MAX_PROMPT_LEN (1024)

static int manual_get_assert(const cfg_t *cfg, const char *prompt,
                             pam_handle_t *pamh, fido_assert_t *assert) {
  char *b64_cdh = NULL;
  char *b64_rpid = NULL;
  char *b64_authdata = NULL;
  char *b64_sig = NULL;
  unsigned char *authdata = NULL;
  unsigned char *sig = NULL;
  size_t authdata_len;
  size_t sig_len;
  int r;
  int ok = 0;

  b64_cdh = converse(pamh, PAM_PROMPT_ECHO_ON, prompt);
  b64_rpid = converse(pamh, PAM_PROMPT_ECHO_ON, prompt);
  b64_authdata = converse(pamh, PAM_PROMPT_ECHO_ON, prompt);
  b64_sig = converse(pamh, PAM_PROMPT_ECHO_ON, prompt);

  if (!b64_decode(b64_authdata, (void **) &authdata, &authdata_len)) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to decode authenticator data");
    goto err;
  }

  if (!b64_decode(b64_sig, (void **) &sig, &sig_len)) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to decode signature");
    goto err;
  }

  r = fido_assert_set_count(assert, 1);
  if (r != FIDO_OK) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to set signature count of assertion");
    goto err;
  }

  r = fido_assert_set_authdata(assert, 0, authdata, authdata_len);
  if (r != FIDO_OK) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to set authdata of assertion");
    goto err;
  }

  r = fido_assert_set_sig(assert, 0, sig, sig_len);
  if (r != FIDO_OK) {
    if (cfg->debug)
      D(cfg->debug_file, "Failed to set signature of assertion");
    goto err;
  }

  ok = 1;
err:
  free(b64_cdh);
  free(b64_rpid);
  free(b64_authdata);
  free(b64_sig);
  free(authdata);
  free(sig);

  return ok;
}

int do_manual_authentication(const cfg_t *cfg, const device_t *devices,
                             const unsigned n_devs, pam_handle_t *pamh) {
  fido_assert_t **assert = NULL;
  struct pk *pk = NULL;
  char *b64_challenge = NULL;
  char prompt[MAX_PROMPT_LEN];
  char buf[MAX_PROMPT_LEN];
  int retval = -2;
  int n;
  int r;
  unsigned i = 0;
  struct opts opts;

  init_opts(&opts);
  assert = calloc(n_devs, sizeof(*assert));
  if (assert == NULL)
	goto out;
  pk = calloc(n_devs, sizeof(*pk));
  if (pk == NULL)
	goto out;

#ifndef WITH_FUZZING
  fido_init(cfg->debug ? FIDO_DEBUG : 0);
#else
  fido_init(0);
#endif

  for (i = 0; i < n_devs; ++i) {
    /* options used during authentication */
    parse_opts(cfg, devices[i].attributes, &opts);
    assert[i] = prepare_assert(cfg, &devices[i], &opts);
    if (assert[i] == NULL) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to prepare assert");
      goto out;
    }

    if (cfg->debug)
      D(cfg->debug_file, "Attempting authentication with device number %d",
        i + 1);

    if (!parse_pk(cfg, devices[i].old_format, devices[i].coseType,
                  devices[i].publicKey, &pk[i])) {
      if (cfg->debug)
        D(cfg->debug_file, "Unable to parse public key %u", i);
      goto out;
    }

    if (!b64_encode(fido_assert_clientdata_hash_ptr(assert[i]),
                    fido_assert_clientdata_hash_len(assert[i]),
                    &b64_challenge)) {
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

    if (!manual_get_assert(cfg, prompt, pamh, assert[i])) {
      if (cfg->debug)
        D(cfg->debug_file, "Failed to get assert %u", i);
      goto out;
    }

    r = fido_assert_verify(assert[i], 0, pk[i].type, pk[i].ptr);
    if (r == FIDO_OK) {
      retval = 1;
      break;
    }
  }

out:
  for (i = 0; i < n_devs; i++) {
    fido_assert_free(&assert[i]);
    reset_pk(&pk[i]);
  }
  free(assert);
  free(pk);
  free(b64_challenge);

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
                                  .msg = (char *) (uintptr_t) prompt};
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

#if defined(WITH_FUZZING)
  (void) debug_file;
  snprintf(NULL, 0, DEBUG_STR, file, line, func);
  vsnprintf(NULL, 0, fmt, ap);
#elif defined(LOG_DEBUG)
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
