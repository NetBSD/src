/*
 *  Copyright (C) 2014-2023 Yubico AB - See COPYING
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

#include "debug.h"
#include "drop_privs.h"
#include "util.h"

#define free_const(a) free((void *) (uintptr_t) (a))

/* If secure_getenv is not defined, define it here */
#ifndef HAVE_SECURE_GETENV
char *secure_getenv(const char *);
char *secure_getenv(const char *name) {
  (void) name;
  return NULL;
}
#endif

static void parse_cfg(int flags __unused, int argc, const char **argv, cfg_t *cfg) {
  int i;

  memset(cfg, 0, sizeof(cfg_t));
  cfg->debug_file = DEFAULT_DEBUG_FILE;
  cfg->userpresence = -1;
  cfg->userverification = -1;
  cfg->pinverification = -1;

  for (i = 0; i < argc; i++) {
    if (strncmp(argv[i], "max_devices=", 12) == 0) {
      sscanf(argv[i], "max_devices=%u", &cfg->max_devs);
    } else if (strcmp(argv[i], "manual") == 0) {
      cfg->manual = 1;
    } else if (strcmp(argv[i], "debug") == 0) {
      cfg->debug = 1;
    } else if (strcmp(argv[i], "nouserok") == 0) {
      cfg->nouserok = 1;
    } else if (strcmp(argv[i], "openasuser") == 0) {
      cfg->openasuser = 1;
    } else if (strcmp(argv[i], "alwaysok") == 0) {
      cfg->alwaysok = 1;
    } else if (strcmp(argv[i], "interactive") == 0) {
      cfg->interactive = 1;
    } else if (strcmp(argv[i], "cue") == 0) {
      cfg->cue = 1;
    } else if (strcmp(argv[i], "nodetect") == 0) {
      cfg->nodetect = 1;
    } else if (strcmp(argv[i], "expand") == 0) {
      cfg->expand = 1;
    } else if (strncmp(argv[i], "userpresence=", 13) == 0) {
      sscanf(argv[i], "userpresence=%d", &cfg->userpresence);
    } else if (strncmp(argv[i], "userverification=", 17) == 0) {
      sscanf(argv[i], "userverification=%d", &cfg->userverification);
    } else if (strncmp(argv[i], "pinverification=", 16) == 0) {
      sscanf(argv[i], "pinverification=%d", &cfg->pinverification);
    } else if (strncmp(argv[i], "authfile=", 9) == 0) {
      cfg->auth_file = argv[i] + 9;
    } else if (strcmp(argv[i], "sshformat") == 0) {
      cfg->sshformat = 1;
    } else if (strncmp(argv[i], "authpending_file=", 17) == 0) {
      cfg->authpending_file = argv[i] + 17;
    } else if (strncmp(argv[i], "origin=", 7) == 0) {
      cfg->origin = argv[i] + 7;
    } else if (strncmp(argv[i], "appid=", 6) == 0) {
      cfg->appid = argv[i] + 6;
    } else if (strncmp(argv[i], "prompt=", 7) == 0) {
      cfg->prompt = argv[i] + 7;
    } else if (strncmp(argv[i], "cue_prompt=", 11) == 0) {
      cfg->cue_prompt = argv[i] + 11;
    } else if (strncmp(argv[i], "debug_file=", 11) == 0) {
      const char *filename = argv[i] + 11;
      debug_close(cfg->debug_file);
      cfg->debug_file = debug_open(filename);
    }
  }

  debug_dbg(cfg, "called.");
  debug_dbg(cfg, "flags %d argc %d", flags, argc);
  for (i = 0; i < argc; i++) {
    debug_dbg(cfg, "argv[%d]=%s", i, argv[i]);
  }
  debug_dbg(cfg, "max_devices=%d", cfg->max_devs);
  debug_dbg(cfg, "debug=%d", cfg->debug);
  debug_dbg(cfg, "interactive=%d", cfg->interactive);
  debug_dbg(cfg, "cue=%d", cfg->cue);
  debug_dbg(cfg, "nodetect=%d", cfg->nodetect);
  debug_dbg(cfg, "userpresence=%d", cfg->userpresence);
  debug_dbg(cfg, "userverification=%d", cfg->userverification);
  debug_dbg(cfg, "pinverification=%d", cfg->pinverification);
  debug_dbg(cfg, "manual=%d", cfg->manual);
  debug_dbg(cfg, "nouserok=%d", cfg->nouserok);
  debug_dbg(cfg, "openasuser=%d", cfg->openasuser);
  debug_dbg(cfg, "alwaysok=%d", cfg->alwaysok);
  debug_dbg(cfg, "sshformat=%d", cfg->sshformat);
  debug_dbg(cfg, "expand=%d", cfg->expand);
  debug_dbg(cfg, "authfile=%s", cfg->auth_file ? cfg->auth_file : "(null)");
  debug_dbg(cfg, "authpending_file=%s",
            cfg->authpending_file ? cfg->authpending_file : "(null)");
  debug_dbg(cfg, "origin=%s", cfg->origin ? cfg->origin : "(null)");
  debug_dbg(cfg, "appid=%s", cfg->appid ? cfg->appid : "(null)");
  debug_dbg(cfg, "prompt=%s", cfg->prompt ? cfg->prompt : "(null)");
}

static void interactive_prompt(pam_handle_t *pamh, const cfg_t *cfg) {
  char *tmp = NULL;

  tmp = converse(pamh, PAM_PROMPT_ECHO_ON,
                 cfg->prompt != NULL ? cfg->prompt : DEFAULT_PROMPT);

  free(tmp);
}

static char *resolve_authfile_path(const cfg_t *cfg, const struct passwd *user,
                                   int *openasuser) {
  char *authfile = NULL;
  const char *dir = NULL;
  const char *path = NULL;

  *openasuser = geteuid() == 0; /* user files, drop privileges */

  if (cfg->auth_file == NULL) {
    if ((dir = secure_getenv(DEFAULT_AUTHFILE_DIR_VAR)) == NULL) {
      debug_dbg(cfg, "Variable %s is not set, using default",
                DEFAULT_AUTHFILE_DIR_VAR);
      dir = user->pw_dir;
      path = cfg->sshformat ? DEFAULT_AUTHFILE_DIR_SSH "/" DEFAULT_AUTHFILE_SSH
                            : DEFAULT_AUTHFILE_DIR "/" DEFAULT_AUTHFILE;
    } else {
      debug_dbg(cfg, "Variable %s set to %s", DEFAULT_AUTHFILE_DIR_VAR, dir);
      *openasuser = 0; /* documented exception, require explicit openasuser */
      path = cfg->sshformat ? DEFAULT_AUTHFILE_SSH : DEFAULT_AUTHFILE;
      if (!cfg->openasuser) {
        debug_dbg(cfg, "WARNING: not dropping privileges when reading the "
                       "authentication file, please consider setting "
                       "openasuser=1 in the module configuration");
      }
    }
  } else {
    dir = user->pw_dir;
    path = cfg->auth_file;
  }

  if (dir == NULL || *dir != '/' || path == NULL ||
      asprintf(&authfile, "%s/%s", dir, path) == -1)
    authfile = NULL;

  return authfile;
}

/* PAM entry point for authentication verification */
int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc,
                        const char **argv) {

  struct passwd *pw = NULL, pw_s;
  const char *user = NULL;

  cfg_t cfg_st;
  cfg_t *cfg = &cfg_st;
  char buffer[BUFSIZE];
  int pgu_ret, gpn_ret;
  int retval = PAM_ABORT;
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
        debug_dbg(cfg, "Unable to get host name");
        retval = PAM_SYSTEM_ERR;
        goto done;
      }
    } else {
      strcpy(buffer, SSH_ORIGIN);
    }
    debug_dbg(cfg, "Origin not specified, using \"%s\"", buffer);
    cfg->origin = strdup(buffer);
    if (!cfg->origin) {
      debug_dbg(cfg, "Unable to allocate memory");
      retval = PAM_BUF_ERR;
      goto done;
    } else {
      should_free_origin = 1;
    }
  }

  if (!cfg->appid) {
    debug_dbg(cfg, "Appid not specified, using the value of origin (%s)",
              cfg->origin);
    cfg->appid = strdup(cfg->origin);
    if (!cfg->appid) {
      debug_dbg(cfg, "Unable to allocate memory");
      retval = PAM_BUF_ERR;
      goto done;
    } else {
      should_free_appid = 1;
    }
  }

  if (cfg->max_devs == 0) {
    debug_dbg(cfg, "Maximum number of devices not set. Using default (%d)",
              MAX_DEVS);
    cfg->max_devs = MAX_DEVS;
  }
#if WITH_FUZZING
  if (cfg->max_devs > 256)
    cfg->max_devs = 256;
#endif

  devices = calloc(cfg->max_devs, sizeof(device_t));
  if (!devices) {
    debug_dbg(cfg, "Unable to allocate memory");
    retval = PAM_BUF_ERR;
    goto done;
  }

  pgu_ret = pam_get_user(pamh, &user, NULL);
  if (pgu_ret != PAM_SUCCESS || user == NULL) {
    debug_dbg(cfg, "Unable to get username from PAM");
    retval = PAM_CONV_ERR;
    goto done;
  }

  debug_dbg(cfg, "Requesting authentication for user %s", user);

  gpn_ret = getpwnam_r(user, &pw_s, buffer, sizeof(buffer), &pw);
  if (gpn_ret != 0 || pw == NULL || pw->pw_dir == NULL ||
      pw->pw_dir[0] != '/') {
    debug_dbg(cfg, "Unable to retrieve credentials for user %s, (%s)", user,
              strerror(errno));
    retval = PAM_SYSTEM_ERR;
    goto done;
  }

  debug_dbg(cfg, "Found user %s", user);
  debug_dbg(cfg, "Home directory for %s is %s", user, pw->pw_dir);

  // Perform variable expansion.
  if (cfg->expand && cfg->auth_file) {
    if ((cfg->auth_file = expand_variables(cfg->auth_file, user)) == NULL) {
      debug_dbg(cfg, "Failed to perform variable expansion");
      retval = PAM_BUF_ERR;
      goto done;
    }
    should_free_auth_file = 1;
  }
  // Resolve default or relative paths.
  if (!cfg->auth_file || cfg->auth_file[0] != '/') {
    char *tmp = resolve_authfile_path(cfg, pw, &openasuser);
    if (tmp == NULL) {
      debug_dbg(cfg, "Could not resolve authfile path");
      retval = PAM_BUF_ERR;
      goto done;
    }
    if (should_free_auth_file) {
      free_const(cfg->auth_file);
    }
    cfg->auth_file = tmp;
    should_free_auth_file = 1;
  }

  debug_dbg(cfg, "Using authentication file %s", cfg->auth_file);

  if (!openasuser) {
    openasuser = geteuid() == 0 && cfg->openasuser;
  }
  if (openasuser) {
    debug_dbg(cfg, "Dropping privileges");
    if (pam_modutil_drop_priv(pamh, &privs, pw)) {
      debug_dbg(cfg, "Unable to switch user to uid %i", pw->pw_uid);
      retval = PAM_SYSTEM_ERR;
      goto done;
    }
    debug_dbg(cfg, "Switched to uid %i", pw->pw_uid);
  }
  retval = get_devices_from_authfile(cfg, user, devices, &n_devices);

  if (openasuser) {
    if (pam_modutil_regain_priv(pamh, &privs)) {
      debug_dbg(cfg, "could not restore privileges");
      retval = PAM_SYSTEM_ERR;
      goto done;
    }
    debug_dbg(cfg, "Restored privileges");
  }

  if (retval != PAM_SUCCESS) {
    goto done;
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
      debug_dbg(cfg, "Unable to allocate memory for the authpending_file, "
                     "touch request notifications will not be emitted");
    } else {
      should_free_authpending_file = 1;
    }
  } else {
    if (strlen(cfg->authpending_file) == 0) {
      debug_dbg(cfg, "authpending_file is set to an empty value, touch request "
                     "notifications will be disabled");
      cfg->authpending_file = NULL;
    }
  }

  int authpending_file_descriptor = -1;
  if (cfg->authpending_file) {
    debug_dbg(cfg, "Touch request notifications will be emitted via '%s'",
              cfg->authpending_file);

    // Open (or create) the authpending_file to indicate that we start waiting
    // for a touch
    authpending_file_descriptor =
      open(cfg->authpending_file,
           O_RDONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NOCTTY, 0664);
    if (authpending_file_descriptor < 0) {
      debug_dbg(cfg, "Unable to emit 'authentication started' notification: %s",
                strerror(errno));
    }
  }

  if (cfg->manual == 0) {
    if (cfg->interactive) {
      interactive_prompt(pamh, cfg);
    }
    retval = do_authentication(cfg, devices, n_devices, pamh);
  } else {
    retval = do_manual_authentication(cfg, devices, n_devices, pamh);
  }

  // Close the authpending_file to indicate that we stop waiting for a touch
  if (authpending_file_descriptor >= 0) {
    if (close(authpending_file_descriptor) < 0) {
      debug_dbg(cfg, "Unable to emit 'authentication stopped' notification: %s",
                strerror(errno));
    }
  }

done:
  free_devices(devices, n_devices);

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
    debug_dbg(cfg, "alwaysok needed (otherwise return with %d)", retval);
    retval = PAM_SUCCESS;
  }
  debug_dbg(cfg, "done. [%s]", pam_strerror(pamh, retval));

  debug_close(cfg->debug_file);
  cfg->debug_file = DEFAULT_DEBUG_FILE;

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
