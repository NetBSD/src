/*
 * Copyright (C) 2020-2022 Yubico AB - See COPYING
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fuzz/fuzz.h"
#include "util.h"

static void cleanup(device_t *devs, unsigned int n_devs) {
  for (unsigned int i = 0; i < n_devs; i++) {
    free(devs[i].keyHandle);
    free(devs[i].publicKey);
    free(devs[i].coseType);
    free(devs[i].attributes);
    devs[i].keyHandle = NULL;
    devs[i].publicKey = NULL;
    devs[i].coseType = NULL;
    devs[i].attributes = NULL;
  }
}

#define DEV_MAX_SIZE 10

int LLVMFuzzerTestOneInput(const uint8_t *, size_t);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  device_t devs[12] = {0};
  unsigned int n_devs = 12;
  FILE *fp = NULL;
  size_t fp_len = 0;
  size_t offset = 0;
  char username[256] = "user";
  size_t username_len = 0;
  cfg_t cfg = {0};

  cfg.max_devs = DEV_MAX_SIZE;
  cfg.auth_file = "/path/to/authfile"; /* XXX: any path works, file mocked */
  cfg.sshformat = 1;

  /* first 6 byte decides which parser we should call, if
   * we want to run with debug and also sets the initial seed */
  if (size < 6) {
    return -1;
  }
  /* do not always run with debug, only 8/255 times */
  if (data[offset++] < 9) {
    cfg.debug = 1;
  }

  /* predictable random for this seed */
  prng_init((uint32_t) data[offset] << 24 | (uint32_t) data[offset + 1] << 16 |
            (uint32_t) data[offset + 2] << 8 | (uint32_t) data[offset + 3]);
  offset += 4;

  /* choose which format parser to run, even == native, odd == ssh */
  if (data[offset++] % 2) {
    cfg.sshformat = 0;
    /* native format, get a random username first */
    if (size < 7) {
      return -1;
    }
    username_len = data[offset++];
    if (username_len > (size - offset)) {
      username_len = (size - offset);
    }
    memset(username, 0, sizeof(username));
    memcpy(username, &data[offset], username_len);
    offset += username_len;
  }

  fp_len = size - offset;
  fp = tmpfile();
  if (fp == NULL || (fwrite(&data[offset], 1, fp_len, fp)) != fp_len) {
    fprintf(stderr, "failed to create file for parser: %s\n", strerror(errno));
    if (fp != NULL) {
      fclose(fp);
    }
    return -1;
  }
  (void) fseek(fp, 0L, SEEK_SET);

  set_user(username);
  set_authfile(fileno(fp));

  get_devices_from_authfile(&cfg, username, devs, &n_devs);

  cleanup(devs, n_devs);
  fclose(fp);

  return 0;
}
