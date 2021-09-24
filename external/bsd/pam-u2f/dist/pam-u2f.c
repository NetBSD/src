/*
 *  Copyright (C) 2014-2019 Yubico AB - See COPYING
 */

/* Define which PAM interfaces we provide */
#define PAM_SM_AUTH

/* Include PAM headers */
#include <security/pam_appl.h>
#include <security/pam_modules.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "drop_privs.h"

/* If secure_getenv is not defined, define it here */
#ifndef HAVE_SECURE_GETENV
char *secure_getenv(const char *);
char *secure_getenv(const char *name) {
  (void) name;
  return NULL;
}
#endif

static void parse_cfg(int flags __unused, int argc, const char **argv, cfg_t *cfg) {
#ifndef WITH_FUZZING
  struct stat st;
#endif
  FILE *file = NULL;
  int fd = -1;
  int i;

  memset(cfg, 0, sizeof(cfg_t));
  cfg->debug_file = stderr;
  cfg->userpresence = -1;
  cfg->userverification = -1;
  cfg->pinverification = -1;

  for (i = 0; i < argc; i++) {
    if (strncmp(argv[i], "max_devices=", 12) == 0)
      sscanf(argv[i], "max_devices=%u", &cfg->max_devs);
    if (strcmp(argv[i], "manual") == 0)
      cfg->manual = 1;
    if (strcmp(argv[i], "debug") == 0)
      cfg->debug = 1;
    if (strcmp(argv[i], "nouserok") == 0)
      cfg->nouserok = 1;
    if (strcmp(argv[i], "openasuser") == 0)
      cfg->openasuser = 1;
    if (strcmp(argv[i], "alwaysok") == 0)
      cfg->alwaysok = 1;
    if (strcmp(argv[i], "interactive") == 0)
      cfg->interactive = 1;
    if (strcmp(argv[i], "cue") == 0)
      cfg->cue = 1;
    if (strcmp(argv[i], "nodetect") == 0)
      cfg->nodetect = 1;
    if (strncmp(argv[i], "userpresence=", 13) == 0)
      sscanf(argv[i], "userpresence=%d", &cfg->userpresence);
    if (strncmp(argv[i], "userverification=", 17) == 0)
      sscanf(argv[i], "userverification=%d", &cfg->userverification);
    if (strncmp(argv[i], "pinverification=", 16) == 0)
      sscanf(argv[i], "pinverification=%d", &cfg->pinverification);
    if (strncmp(argv[i], "authfile=", 9) == 0)
      cfg->auth_file = argv[i] + 9;
    if (strncmp(argv[i], "sshformat", 9) == 0)
      cfg->sshformat = 1;
    if (strncmp(argv[i], "authpending_file=", 17) == 0)
      cfg->authpending_file = argv[i] + 17;
    if (strncmp(argv[i], "origin=", 7) == 0)
      cfg->origin = argv[i] + 7;
    if (strncmp(argv[i], "appid=", 6) == 0)
      cfg->appid = argv[i] + 6;
    if (strncmp(argv[i], "prompt=", 7) == 0)
      cfg->prompt = argv[i] + 7;
    if (strncmp(argv[i], "cue_prompt=", 11) == 0)
      cfg->cue_prompt = argv[i] + 11;
    if (strncmp(argv[i], "debug_file=", 11) == 0) {
      if (cfg->is_custom_debug_file)
        fclose(cfg->debug_file);
      cfg->debug_file = stderr;
      cfg->is_custom_debug_file = 0;
      const char *filename = argv[i] + 11;
      if (strncmp(filename, "stdout", 6) == 0) {
        cfg->debug_file = stdout;
      } else if (strncmp(filename, "stderr", 6) == 0) {
        cfg->debug_file = stderr;
      } else if (strncmp(filename, "syslog", 6) == 0) {
        cfg->debug_file = (FILE *) -1;
      } else {
        fd = open(filename,
                  O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW | O_NOCTTY);
#ifndef WITH_FUZZING
        if (fd >= 0 && (fstat(fd, &st) == 0) && S_ISREG(st.st_mode)) {
#else
        if (fd >= 0) {
#endif
          file = fdopen(fd, "a");
          if (file != NULL) {
            cfg->debug_file = file;
            cfg->is_custom_debug_file = 1;
            file = NULL;
            fd = -1;
          }
        }
      }
    }
  }

  if (cfg->debug) {
    D(cfg->debug_file, "called.");
    D(cfg->debug_file, "flags %d argc %d", flags, argc);
    for (i = 0; i < argc; i++) {
      D(cfg->debug_file, "argv[%d]=%s", i, argv[i]);
    }
    D(cfg->debug_file, "max_devices=%d", cfg->max_devs);
    D(cfg->debug_file, "debug=%d", cfg->debug);
    D(cfg->debug_file, "interactive=%d", cfg->interactive);
    D(cfg->debug_file, "cue=%d", cfg->cue);
    D(cfg->debug_file, "nodetect=%d", cfg->nodetect);
    D(cfg->debug_file, "userpresence=%d", cfg->userpresence);
    D(cfg->debug_file, "userverification=%d", cfg->userverification);
    D(cfg->debug_file, "pinverification=%d", cfg->pinverification);
    D(cfg->debug_file, "manual=%d", cfg->manual);
    D(cfg->debug_file, "nouserok=%d", cfg->nouserok);
    D(cfg->debug_file, "openasuser=%d", cfg->openasuser);
    D(cfg->debug_file, "alwaysok=%d", cfg->alwaysok);
    D(cfg->debug_file, "sshformat=%d", cfg->sshformat);
    D(cfg->debug_file, "authfile=%s",
      cfg->auth_file ? cfg->auth_file : "(null)");
    D(cfg->debug_file, "authpending_file=%s",
      cfg->authpending_file ? cfg->authpending_file : "(null)");
    D(cfg->debug_file, "origin=%s", cfg->origin ? cfg->origin : "(null)");
    D(cfg->debug_file, "appid=%s", cfg->appid ? cfg->appid : "(null)");
    D(cfg->debug_file, "prompt=%s", cfg->prompt ? cfg->prompt : "(null)");
  }

  if (fd != -1)
    close(fd);

  if (file != NULL)
    fclose(file);
}

#ifdef DBG
#undef DBG
#endif
#define DBG(...)                                                               \
  if (cfg->debug) {                                                            \
    D(cfg->debug_file, __VA_ARGS__);                                           \
  }

/* PAM entry point for authentication verification */
int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc,
                        const char **argv) {

  struct passwd *pw = NULL, pw_s;
  const char *user = NULL;

  cfg_t cfg_st;
  cfg_t *cfg = &cfg_st;
  char buffer[BUFSIZE];
  char *buf = NULL;
  char *authfile_dir;
  size_t authfile_dir_len;
  const char *default_authfile;
  const char *default_authfile_dir;
  int pgu_ret, gpn_ret;
  int retval = PAM_IGNORE;
  device_t *devices = NULL;
  unsigned n_devices = 0;
  int openasuser = 0;
  int should_free_origin = 0;
  int should_free_appid = 0;
  int should_free_auth_file = 0;
  int should_free_authpending_file = 0;

  parse_cfg(flags, argc, argv, cfg);

  PAM_MODUTIL_DEF_PRIVS(privs);

  if (!cfg->origin) {
    if (!cfg->sshformat) {
      strcpy(buffer, DEFAULT_ORIGIN_PREFIX);

      if (gethostname(buffer + strlen(DEFAULT_ORIGIN_PREFIX),
                      BUFSIZE - strlen(DEFAULT_ORIGIN_PREFIX)) == -1) {
        DBG("Unable to get host name");
        goto done;
      }
    } else {
      strcpy(buffer, SSH_ORIGIN);
    }
    DBG("Origin not specified, using \"%s\"", buffer);
    cfg->origin = strdup(buffer);
    if (!cfg->origin) {
      DBG("Unable to allocate memory");
      goto done;
    } else {
      should_free_origin = 1;
    }
  }

  if (!cfg->appid) {
    DBG("Appid not specified, using the same value of origin (%s)",
        cfg->origin);
    cfg->appid = strdup(cfg->origin);
    if (!cfg->appid) {
      DBG("Unable to allocate memory")
      goto done;
    } else {
      should_free_appid = 1;
    }
  }

  if (cfg->max_devs == 0) {
    DBG("Maximum devices number not set. Using default (%d)", MAX_DEVS);
    cfg->max_devs = MAX_DEVS;
  }
#if WITH_FUZZING
  if (cfg->max_devs > 256)
    cfg->max_devs = 256;
#endif

  devices = calloc(cfg->max_devs, sizeof(device_t));
  if (!devices) {
    DBG("Unable to allocate memory");
    retval = PAM_IGNORE;
    goto done;
  }

  pgu_ret = pam_get_user(pamh, &user, NULL);
  if (pgu_ret != PAM_SUCCESS || user == NULL) {
    DBG("Unable to access user %s", user);
    retval = PAM_CONV_ERR;
    goto done;
  }

  DBG("Requesting authentication for user %s", user);

  gpn_ret = getpwnam_r(user, &pw_s, buffer, sizeof(buffer), &pw);
  if (gpn_ret != 0 || pw == NULL || pw->pw_dir == NULL ||
      pw->pw_dir[0] != '/') {
    DBG("Unable to retrieve credentials for user %s, (%s)", user,
        strerror(errno));
    retval = PAM_USER_UNKNOWN;
    goto done;
  }

  DBG("Found user %s", user);
  DBG("Home directory for %s is %s", user, pw->pw_dir);

  if (!cfg->sshformat) {
    default_authfile = DEFAULT_AUTHFILE;
    default_authfile_dir = DEFAULT_AUTHFILE_DIR;
  } else {
    default_authfile = DEFAULT_AUTHFILE_SSH;
    default_authfile_dir = DEFAULT_AUTHFILE_DIR_SSH;
  }

  if (!cfg->auth_file) {
    buf = NULL;
    authfile_dir = secure_getenv(DEFAULT_AUTHFILE_DIR_VAR);
    if (!authfile_dir) {
      DBG("Variable %s is not set. Using default value ($HOME%s/)",
          DEFAULT_AUTHFILE_DIR_VAR, default_authfile_dir);
      authfile_dir_len = strlen(pw->pw_dir) + strlen(default_authfile_dir) +
                         strlen(default_authfile) + 1;
      buf = malloc(sizeof(char) * (authfile_dir_len));

      if (!buf) {
        DBG("Unable to allocate memory");
        retval = PAM_IGNORE;
        goto done;
      }

      /* Opening a file in a users $HOME, need to drop privs for security */
      openasuser = geteuid() == 0 ? 1 : 0;

      snprintf(buf, authfile_dir_len, "%s%s%s", pw->pw_dir,
               default_authfile_dir, default_authfile);
    } else {
      DBG("Variable %s set to %s", DEFAULT_AUTHFILE_DIR_VAR, authfile_dir);
      authfile_dir_len = strlen(authfile_dir) + strlen(default_authfile) + 1;
      buf = malloc(sizeof(char) * (authfile_dir_len));

      if (!buf) {
        DBG("Unable to allocate memory");
        retval = PAM_IGNORE;
        goto done;
      }

      snprintf(buf, authfile_dir_len, "%s%s", authfile_dir, default_authfile);

      if (!cfg->openasuser) {
        DBG("WARNING: not dropping privileges when reading %s, please "
            "consider setting openasuser=1 in the module configuration",
            buf);
      }
    }

    DBG("Using authentication file %s", buf);

    cfg->auth_file = buf; /* cfg takes ownership */
    should_free_auth_file = 1;
    buf = NULL;
  } else {
    if (cfg->auth_file[0] != '/') {
      /* Individual authorization mapping by user: auth_file is not
          absolute path, so prepend user home dir. */
      openasuser = geteuid() == 0 ? 1 : 0;

      authfile_dir_len =
        strlen(pw->pw_dir) + strlen("/") + strlen(cfg->auth_file) + 1;
      buf = malloc(sizeof(char) * (authfile_dir_len));

      if (!buf) {
        DBG("Unable to allocate memory");
        retval = PAM_IGNORE;
        goto done;
      }

      snprintf(buf, authfile_dir_len, "%s/%s", pw->pw_dir, cfg->auth_file);

      cfg->auth_file = buf; /* update cfg */
      should_free_auth_file = 1;
      buf = NULL;
    }

    DBG("Using authentication file %s", cfg->auth_file);
  }

  if (!openasuser) {
    openasuser = geteuid() == 0 && cfg->openasuser;
  }
  if (openasuser) {
    DBG("Dropping privileges");
    if (pam_modutil_drop_priv(pamh, &privs, pw)) {
      DBG("Unable to switch user to uid %i", pw->pw_uid);
      retval = PAM_IGNORE;
      goto done;
    }
    DBG("Switched to uid %i", pw->pw_uid);
  }
  retval = get_devices_from_authfile(cfg, user, devices, &n_devices);

  if (openasuser) {
    if (pam_modutil_regain_priv(pamh, &privs)) {
      DBG("could not restore privileges");
      retval = PAM_IGNORE;
      goto done;
    }
    DBG("Restored privileges");
  }

  if (retval != 1) {
    // for nouserok; make sure errors in get_devices_from_authfile don't
    // result in valid devices
    n_devices = 0;
  }

  if (n_devices == 0) {
    if (cfg->nouserok) {
      DBG("Found no devices but nouserok specified. Skipping authentication");
      retval = PAM_SUCCESS;
      goto done;
    } else if (retval != 1) {
      DBG("Unable to get devices from file %s", cfg->auth_file);
      retval = PAM_AUTHINFO_UNAVAIL;
      goto done;
    } else {
      DBG("Found no devices. Aborting.");
      retval = PAM_AUTHINFO_UNAVAIL;
      goto done;
    }
  }

  // Determine the full path for authpending_file in order to emit touch request
  // notifications
  if (!cfg->authpending_file) {
    int actual_size =
      snprintf(buffer, BUFSIZE, DEFAULT_AUTHPENDING_FILE_PATH, getuid());
    if (actual_size >= 0 && actual_size < BUFSIZE) {
      cfg->authpending_file = strdup(buffer);
    }
    if (!cfg->authpending_file) {
      DBG("Unable to allocate memory for the authpending_file, touch request "
          "notifications will not be emitted");
    } else {
      should_free_authpending_file = 1;
    }
  } else {
    if (strlen(cfg->authpending_file) == 0) {
      DBG("authpending_file is set to an empty value, touch request "
          "notifications will be disabled");
      cfg->authpending_file = NULL;
    }
  }

  int authpending_file_descriptor = -1;
  if (cfg->authpending_file) {
    DBG("Using file '%s' for emitting touch request notifications",
        cfg->authpending_file);

    // Open (or create) the authpending_file to indicate that we start waiting
    // for a touch
    authpending_file_descriptor =
      open(cfg->authpending_file,
           O_RDONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NOCTTY, 0664);
    if (authpending_file_descriptor < 0) {
      DBG("Unable to emit 'authentication started' notification by opening the "
          "file '%s', (%s)",
          cfg->authpending_file, strerror(errno));
    }
  }

  if (cfg->manual == 0) {
    if (cfg->interactive) {
      buf = converse(pamh, PAM_PROMPT_ECHO_ON,
                     cfg->prompt != NULL ? cfg->prompt : DEFAULT_PROMPT);
      free(buf);
      buf = NULL;
    }

    retval = do_authentication(cfg, devices, n_devices, pamh);
  } else {
    retval = do_manual_authentication(cfg, devices, n_devices, pamh);
  }

  // Close the authpending_file to indicate that we stop waiting for a touch
  if (authpending_file_descriptor >= 0) {
    if (close(authpending_file_descriptor) < 0) {
      DBG("Unable to emit 'authentication stopped' notification by closing the "
          "file '%s', (%s)",
          cfg->authpending_file, strerror(errno));
    }
  }

  if (retval != 1) {
    DBG("do_authentication returned %d", retval);
    retval = PAM_AUTH_ERR;
    goto done;
  }

  retval = PAM_SUCCESS;

done:
  free_devices(devices, n_devices);

  if (buf) {
    free(buf);
    buf = NULL;
  }
#define free_const(a) free((void *) (uintptr_t)(a))
  if (should_free_origin) {
    free_const(cfg->origin);
    cfg->origin = NULL;
  }

  if (should_free_appid) {
    free_const(cfg->appid);
    cfg->appid = NULL;
  }

  if (should_free_auth_file) {
    free_const(cfg->auth_file);
    cfg->auth_file = NULL;
  }

  if (should_free_authpending_file) {
    free_const(cfg->authpending_file);
    cfg->authpending_file = NULL;
  }

  if (cfg->alwaysok && retval != PAM_SUCCESS) {
    DBG("alwaysok needed (otherwise return with %d)", retval);
    retval = PAM_SUCCESS;
  }
  DBG("done. [%s]", pam_strerror(pamh, retval));

  if (cfg->is_custom_debug_file) {
    fclose(cfg->debug_file);
  }

  return retval;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc,
                              const char **argv) {
  (void) pamh;
  (void) flags;
  (void) argc;
  (void) argv;

  return PAM_SUCCESS;
}

#ifdef PAM_MODULE_ENTRY
PAM_MODULE_ENTRY("pam_u2f");
#endif
