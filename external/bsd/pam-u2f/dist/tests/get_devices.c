/*
 *  Copyright (C) 2014-2018 Yubico AB - See COPYING
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* These #defines must be present according to PAM documentation. */
#define PAM_SM_AUTH

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#ifdef HAVE_SECURITY_PAM_MODULES_H
#include <security/pam_modules.h>
#endif

#include <string.h>
#include "../util.h"

int main(int argc, const char **argv) {
  int rc;

  cfg_t cfg;

  memset(&cfg, 0, sizeof(cfg_t));
  cfg.auth_file = "credentials/ssh_credential.cred";
  cfg.debug = 1;
  cfg.debug_file = stderr;
  cfg.max_devs = 24;
  cfg.sshformat = 1;

  device_t dev[24];
  memset(dev, 0, sizeof(dev));
  unsigned n_devs;
  char *username;

  username = secure_getenv("USER");
  if (username == NULL) {
    username = secure_getenv("LOGNAME");
  }
  assert(username != NULL);

  rc = get_devices_from_authfile(&cfg, username /* not used for SSH format */,
                                 dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);
  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].attributes, "+presence") == 0);
  assert(strcmp(dev[0].keyHandle,
                "Li4NkUKcvFym8V6aGagSAI11MXPuKSu6kqdWhdxNmQo3i25Ab"
                "1Lkun2I2H2bz4EjuwLD1UQpJjLG5vjbKG8efg==") == 0);
  assert(strcmp(dev[0].publicKey,
                "439pGle7126d1YORADduke347N2t2XyKzOSv8M4naCUjlFYDt"
                "TVhP/MXO41wzHFUIzrrzfEzzCGWoOH5FU5Adw==") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);
  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(
    strcmp(dev[0].keyHandle,
           "vlcWFQFik8gJySuxMTlRwSDvnq9u/"
           "mlMXRIqv4rd7Kq2CJj1V9Uh9PqbTF8UkY3EcQfHeS0G3nY0ibyxXE0pdw==") == 0);
  assert(strcmp(dev[0].publicKey, "CTTRrHrqQmqfyI7/"
                                  "bhtAknx9TGCqhd936JdcoekUxUa6PNA6uYzsvFN0qaE+"
                                  "j2LchLPU4vajQPdAOcvvvNfWCA==") == 0);
  assert(strcmp(dev[0].attributes, "+presence") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-V.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(
    strcmp(dev[0].keyHandle,
           "qf/qcQqFloToNoUMnp2cWg8pUPKoJ0CJFyP0wqpbpOgcD+hzEOJEBaHFbnnYP9d/"
           "zLKuwTsQ1nRpSc/aDJTEeQ==") == 0);
  assert(strcmp(dev[0].publicKey,
                "kwca39tt8HI+MV7skKO1W1K4y0ptbXv6lFW/nwxZ0GSVeMAwTZgf/"
                "XP1O7O0i9+D227F/Ppo5eIc6gquvjiXdA==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+verification") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(
    strcmp(dev[0].keyHandle,
           "IPbgFVDLguVOr5GzdV7C5MH4Ec+"
           "bWfG2hifOy0IWWvNsHUZyN5x0rqbAoGWQPgxbAuQTKfk/n+3U9h4AWf8QXg==") ==
    0);
  assert(strcmp(dev[0].publicKey,
                "5KfKYcZofwqflFbr+d+df0e9b8nfLulducJ1WMyTBO00Rf3rL3JInYeccS2+"
                "xvI+eYNsZmJ3RR6zFAPkkBUhzA==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+pin") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-V-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "HftI6IHewEFB4OhBMeT9WjnG097GYvpE4dTxSS33JTRzRP6V/"
                "oBPyj3vurnTRJwif98V8YhceMAH8lDePA1dxQ==") == 0);
  assert(strcmp(dev[0].publicKey,
                "7h0f9+"
                "MuzG087QC8zjLK9UoEksAXHmmGmoHGPvWwfkfSsH2cqq"
                "p7Qyi4LO7Y58OxlEq79gbWqNYEP0H56zvZ4Q==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+verification+pin") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-P.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "yvFPHZBdPoBcdhF86mImwNQm2DUgfPw0s26QCpm4XQO0is4ql"
                "x3nIdyVP9WHszpJ5uFV/1mjd09L3P6ton1fAw==") == 0);
  assert(
    strcmp(dev[0].publicKey,
           "JTP+Uu9VE/79hD1H+Uzf9yqSCi9HgeMLeuc3jQ43TCxg5o+GeFL7Q6e63p3Dn4/"
           "uch2YJ8iNNJmDlktrLouWSg==") == 0);
  assert(strcmp(dev[0].attributes, "") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-P-V.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "WSSDFwB8Bv4wg5pOLzYNRsqyJYi6/rbuxL6nzuvPOkpSslyNX/"
                "8lcZSsPfBmuWkRE1CNh7xvalAlBUz1/LUcbg==") == 0);
  assert(strcmp(dev[0].publicKey, "XUIVb5hwOunVJvtwDXAWr5/"
                                  "FTd7tkbYN6ahTzrSwmLtd8ISBJq9LBJ9v1NwfHBMakdC"
                                  "OBbl8LMVc7bDBSrMoZw==") == 0);
  assert(strcmp(dev[0].attributes, "+verification") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-P-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "+/"
                "l9LJ6dwbnDLff0PqkDhMEOWsruM+aYP+"
                "bzQdaCq3QmTGnh0dbcblfLaYs86XgcirS9OEoEkohB5pd8mhwSMQ==") == 0);
  assert(strcmp(dev[0].publicKey,
                "d7sKBe6vgaoYTEXcyovikiB/7IZXLyUPv8qfdaxwWvy7WaGYhwkMvr2H/"
                "q6YBBHJmRl0OCU3WTD/hfeAo2RknA==") == 0);
  assert(strcmp(dev[0].attributes, "+pin") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-P-V-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "vw9z9n3ndQkTKPY3+LDy1Fd2otIsV5LgcYE+"
                "dR0buViSZnKcLJ1kav46mQ47jtelw82/6q3Z2/VKQ44F763tVg==") == 0);
  assert(strcmp(dev[0].publicKey,
                "X+"
                "GY5K9BSG24K9uVnaWgE8wlRhElIPp526M0Xw8H7zqVkGJm2OF"
                "T1ZQeowjxqEx4agArzPTT5WvukpERNLe81Q==") == 0);
  assert(strcmp(dev[0].attributes, "+verification+pin") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-r.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "b9G0ZYtAB4TQQBnpUfptgTzDArSqLMP3/"
                "LxtHYZQrIpXrUnGsqi0gYrKa8ThJoKRlj6f3EJdsJMRdnOr6323+w==") ==
         0);
  assert(strcmp(dev[0].attributes, "+presence") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-r-V.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "D/"
                "ZqT9AuR83CV07njO9NKFuzREbmec3Da+"
                "NS2HMG346rh8Jq2zd9rbB35tedrUC4fZiRa3yRXlWYz1L9GXku7Q==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+verification") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-r-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey, "+rrhFmn3HrQXi+TMvQbide4/"
                                  "rE24y3feLU9wJfhHsEgmaJiLTwAfiBw5z0ASlyZu3vPU"
                                  "5/MaNuJdAZqvz/zEJQ==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+pin") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-r-V-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "sDQr9MGvetCg0dfPJ3fW804stpJC5VDsPld+zv3C1k6e4I6uyOg9I2lfaZU0/"
                "sp83CaODgmGsMd7O3Zo80c64Q==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+verification+pin") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-r-P.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "yBQxzNyU+UEP7nQtlyuwzLmWXslkYCAHFSl5Fq2GxKIz9V0ocqbG7vRqbU+"
                "RGT73M4e8OLrBoX1gAZO7/2Q82Q==") == 0);
  assert(strcmp(dev[0].attributes, "") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-r-P-V.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey, "GhUPBL+KXG7F7PwhO+"
                                  "F3WDZx4KfxoxYwh7h5rqAzsIqkFESR21CqE7vkCvoWm2"
                                  "dFTU51aJd2qdw/VmxJ0N/vRQ==") == 0);
  assert(strcmp(dev[0].attributes, "+verification") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-r-P-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "9cthNaXFY62096qpe7OF45+KKFMqPb2csGkfa1q35J/"
                "3l8Re7SS9gkgSwvQOOAkxaqWYIWKAP1foPr58eerF0A==") == 0);
  assert(strcmp(dev[0].attributes, "+pin") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_-r-P-V-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "oO3z71rtDS86RH3EdZFxr/y6ZUOZ7/"
                "csyhia9UhiDWWpq7oUoxqvN0qmky9R14Clm6RovaOThX89oIbI84BqxA==") ==
         0);
  assert(strcmp(dev[0].attributes, "+verification+pin") == 0);
  assert(dev[0].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "THwoppI4JkuHWwQsSvsH6E987xAokX4MjB8Vh/"
                "lVghzW3iBtMglBw1epdwjbVEpKMVNqwYq6h71p3sQqnaTgLQ==") == 0);
  assert(strcmp(dev[0].publicKey,
                "CB2xx1o7OBmX27Ph6wiqFUodmAiSiz2EuYg3UV/"
                "yEE0Fe9zeMYrk3k2+Una+O9m1P2uzuU3UypOqszVG1WNvYQ==") == 0);
  assert(strcmp(dev[0].attributes, "+presence") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "i1grPL1cYGGda7VDTA5C4eqaLZXaW7u8LdIIz2QR8f0L07myF"
                "DVWFpHmdhEzFAPGtL2kgwdXwx4NvC8VfEKwjA==") == 0);
  assert(strcmp(dev[1].publicKey,
                "14+UmD2jiBtceZTsshDPl3rKvHFOWeLdNx9nfq4gTHwi+4GmzUvA+"
                "XwCohusQsjWocfoyTejYWKL/ZKc5wRuYQ==") == 0);
  assert(strcmp(dev[1].attributes, "+presence") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-V.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "oBQ1hIWiYfhJ8g6DFWawe0xOAlKtcPiBDKyoS8ydd/"
                "zwXbIEU+fHfnzjh46gLjV67+rt1ycCTTMj+P/7EsLNhg==") == 0);
  assert(
    strcmp(dev[0].publicKey,
           "exBDguUdnZhG4+sXOnKPJtrMvn+Rb7pn2E52wyEieitaLY3Yhb2mSFth5sxNjuwl7/"
           "n+0mMN6gQtmzVxCNvTXw==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+verification") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "/8NBWj19H4Wr+/"
                                  "avzu9Qg5PGwE7fmdrFWGqlNega38eV1nnajviBOS6Aba"
                                  "HmQwqsmVcE+DPNrP7KDFI3ZqjPew==") == 0);
  assert(strcmp(dev[1].publicKey, "+sW8XEp5RJe/"
                                  "ZyPykO6AP2Wm5ySTuLshZ13ohwl0VsypepsyhJxfPmEQ"
                                  "GIXysn47uK5egh4eWMvNyMA4Ww0fPg==") == 0);
  assert(strcmp(dev[1].attributes, "+presence+verification") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "WWJqEWaCASU+nsp2bTFh4LbJVOnf1ZRgNxmDcBuThynSTxDgO1GxGcTYg0Ilo/"
                "RF4YXvVCur7gfALYZA69lDTg==") == 0);
  assert(strcmp(dev[0].publicKey, "ZN+ud1nR+"
                                  "Lk5B6CzcbhvdJztDzgaK0MRLn7MOKPbOWfYpr8bLsYRY"
                                  "IfnVUFfSwnGPF6iMK3/FjHRe1mGhOddkg==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+pin") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "auU99KPIIvKGbRcVmsiEyGp/rPx1RNruXI2qS8+JgX1e7nWPczLvmlkx8/"
                "0Z8ZBNqy69aocwQgGHRWKEbDdwlw==") == 0);
  assert(strcmp(dev[1].publicKey,
                "oG+"
                "oN40QezgwX3S6xFk2sR3jiQnobXxxFQy7Mo5vv9hryeIHX13z"
                "G0OZK0KJuhj4A71OAeNXd065P9tVHeQtOQ==") == 0);
  assert(strcmp(dev[1].attributes, "+presence+pin") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-V-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "5sVKkhoc+afHBtAp7csIg/Sq4RFi1arnr/"
                "Qi9quwpNZ4gPhlI6FFBP4CmH8HLw/n5xt8iQxUD83aue23WbrDVA==") == 0);
  assert(strcmp(dev[0].publicKey,
                "K1oB5vq8XezU8NCA9jEuuxtLjbNS8bTAFEZXeNWvCQ5vF6viE"
                "7hvjBPfTrf2KoLz1JtYxHAngZMW+XOZIloVzw==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+verification+pin") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "RQWf8kjjCXCNrMhUHHHIeWvQVlft96SShOsfTylA0QUO8UzuS"
                "Y1mQQFaOPGde1wSX9b2P7tpfTlhYflfgAwkuQ==") == 0);
  assert(strcmp(dev[1].publicKey,
                "SGFYgZZ0rJoAPhj7KzDKSpm2a7y4lE8PIZ6T8WYeDqrsZxrrx"
                "Shc2sx2cAu+r70c8sb6etjab3m9CxobV8ADcA==") == 0);
  assert(strcmp(dev[1].attributes, "+presence+verification+pin") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-P.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "ACoC1fhEYhdOstzkaCb1PqcU4T6xMrXxe5GEQjPDsheOxJzWG"
                "XTpaA3abmHZ3khcJ8Off/ecyPq2kMMqh3l7Xg==") == 0);
  assert(
    strcmp(dev[0].publicKey,
           "c79BTe8BahuDUaeBAATyT8NKq+"
           "mwV87aaor4s79WI5g9gn7BQDjnyUd1C7aaQMGGtu88h/YOGvDVKMVDal6OJQ==") ==
    0);
  assert(strcmp(dev[0].attributes, "") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "0BdgF8gbsYuFfUrpI3K01LcEwnWBxZ6Ewj61GXZJPQh3IcK4H"
                "+QMMG6nqzBhfLipVwGqUugZxCZP2eR9151kog==") == 0);
  assert(strcmp(dev[1].publicKey,
                "X0vskPE+AKWmGrp3ZGhUJVXeAm+sN6nCbMeC30IpItVhMdSosP9I0jOMmsQeF+"
                "rKh+00K30iNucHdXguLPYL7g==") == 0);
  assert(strcmp(dev[1].attributes, "") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-P-V.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "7jPjHZzm/"
                "Ec6oKy6gpq+XXI3P435OLJFO4o3iGH8KUQlEw+"
                "1Zv0FmUtguJ2HIZifRsIyMILdu2rwCDgcqmuj9Q==") == 0);
  assert(strcmp(dev[0].publicKey,
                "xzrbCZKe8sNdrE0F3dkRwsfkwInYUrKHEAMeeHkNrRLbQqlJH"
                "n9C2j5puty3FDVKMV5y1MCrwyJ8IEZHtX2H+Q==") == 0);
  assert(strcmp(dev[0].attributes, "+verification") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "ghKvm1MLkHIWwr6qjzenROIPfoJCUfrFTlqOXLrMktBSqHaYh"
                "oA7NpqwVa3jJ86hpJFekWDOqoV1sLz+GZ9UtQ==") == 0);
  assert(strcmp(dev[1].publicKey,
                "SyrD8BKIReOUHLII642tgpA+i1S8d+6MOcnfGapk32blq0/"
                "qYWmgzJ5lqv+BsO0nBoOG6uXLqLqMkKt3/zLj1w==") == 0);
  assert(strcmp(dev[1].attributes, "+verification") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-P-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(
    strcmp(dev[0].keyHandle,
           "USgDNJZ9Z8GXzQgWdrkFJ5S+WsqKhdg9zHmoMifow3xBd8Rn0ZH2udPuRs6Q8Y/"
           "13BOCL9lEhdxc+1JAoP0j8w==") == 0);
  assert(strcmp(dev[0].publicKey,
                "Is31uMHcVRQYiCxCe3E1tJfKSA92Q47JoppIfLcRLd9sh+e7QR9Gix4LrA2+"
                "RPw24eKI0iqpMm5ayvPMx2nmxA==") == 0);
  assert(strcmp(dev[0].attributes, "+pin") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "uBTQIj0EUe2YoeLfVXksAo9gXTJJ2cXMyPqOMNvE2g9pDwetJ"
                "IdPTR9oUorMiuRZiXALAlfaayc4vMgQvWXdxw==") == 0);
  assert(strcmp(dev[1].publicKey,
                "cToPi0zc8+U6g1kpqJ2pHXCKQyxyNrJAvuLqBmknwxhciBj0+"
                "iTDFaut0Vc1MSu/r6yrw2mHSnuYXTmPx3mhmw==") == 0);
  assert(strcmp(dev[1].attributes, "+pin") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-P-V-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "Ypw0/"
                "A5KEPshXH0zO72Qlgt1uHvB4VnVRBpObzVGDeS8LxR9s"
                "mealISARIOo3rlOLgjqj6dkJxqu1LoLm22UpA==") == 0);
  assert(strcmp(dev[0].publicKey,
                "dFnZLWVzEvbSw6O4ld9Fjb1Pki4NptNpvASGEthr5GsaWRp6p"
                "Le1Uqwm/IrVrOgwO2Q6sB0SXsQKdAIWbMrPHw==") == 0);
  assert(strcmp(dev[0].attributes, "+verification+pin") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "IMaY3yG6NuO4oVjrKUrCArluNfimT+5pnxB2jA0CXD7xjmhUO+"
                "90lILLwxxnGYKvbq2X5wlxLNnuQLm5gpt7ig==") == 0);
  assert(strcmp(dev[1].publicKey,
                "bDTCB4xWqBY9gh6BLP8b4gJmUIYIQbckvrSMDX/8lyftL/"
                "uesJBxblHkDVzfCIatAzqKZ6kltokEHE8saX8phA==") == 0);
  assert(strcmp(dev[1].attributes, "+verification+pin") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-r.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "JvWtmu8JaeUNYklKkii5KflmS9vVXtTGcqLdWNXcRHza5qCuB"
                "oYX/QNWlKoZklPfsmjTVkXcnBh+B4DSZM55fw==") == 0);
  assert(strcmp(dev[0].attributes, "+presence") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "*") == 0);
  assert(strcmp(dev[1].publicKey,
                "4AXGf9eUWUXpfLNJ+2uySBvz2FmkK2EZP+wyKuTf73UDS8/"
                "vi+DZVllXuhrXmQA36NLwqS8YPEIq6pRLYE6m2A==") == 0);
  assert(strcmp(dev[1].attributes, "+presence") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-r-V.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "qZywZ2yedeQu4bPAy6rk7pPcHPprUd+"
                "DOxGf10MgwteNYKyAWuyPd7tREc0X3ZzoDejzmM3+X0dWALnBBSVWyA==") ==
         0);
  assert(strcmp(dev[0].attributes, "+presence+verification") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "*") == 0);
  assert(strcmp(dev[1].publicKey, "IcQsmgW/Y5UQUW/"
                                  "Bz7eYU1azOfyhQWTr8R6mk0gpBJ4l5qq4BstimedubRF"
                                  "voIAanumNrrqgvo1CA+9rzHG6Hg==") == 0);
  assert(strcmp(dev[1].attributes, "+presence+verification") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-r-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(
    strcmp(dev[0].publicKey,
           "BEOf/KU74hsdWbvfUn7yIXH2aktz/"
           "DY2ChTLpljbntz5xpwsOv+4x9r6LySuVYWuoWH11fgKv4Bqt57MHiPYUg==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+pin") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "*") == 0);
  assert(strcmp(dev[1].publicKey,
                "4vbnuZSSrYJ/qzOpnVgy8cqm7yq6m9+GQlpNnMbPN2kXr+B0vL91O6d7/"
                "0VitOqW8GX2FFQaXvV3mHETtsUYAg==") == 0);
  assert(strcmp(dev[1].attributes, "+presence+pin") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-r-V-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "5zx2nk/ANhsncQV0np0kDYT+jf5w3dQ8rvVM5fqwDcHbh8AzBHbcGiRcNfPE/"
                "6v09cEomfVrIAT+IvyAcZnuow==") == 0);
  assert(strcmp(dev[0].attributes, "+presence+verification+pin") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "*") == 0);
  assert(strcmp(dev[1].publicKey,
                "FJ6553yOZoAJKnR2Ysai/5k1i6PpHz/8HusKkFjOqBSIsAK9vALAb/"
                "M223hz8remwuqPyNXczq1WgBcN4P9wKw==") == 0);
  assert(strcmp(dev[1].attributes, "+presence+verification+pin") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-r-P.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "kuY0RmjxQfbzMREZM8l++bbKTFHSGTYK+"
                "OI0owggVKCXSaD5YHsk2EONGtwWoafg8KypNQIYhxxxT2RlWWVcGw==") ==
         0);
  assert(strcmp(dev[0].attributes, "") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "*") == 0);
  assert(strcmp(dev[1].publicKey,
                "t94+brQxTF62oQw642Pq6qDFFfPx1A7GbjU/"
                "2i+H7OiHLeIALTzm9AwLVZuyofXURgiIrmLAG26ww2KVv6ji+A==") == 0);
  assert(strcmp(dev[1].attributes, "") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-r-P-V.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "KBBozy1XYywARLB1AcY5hYvUN2hYHpGY2YyxAIczZ7GXrfWeZ"
                "8RGOW7+Z34DaozgLFeHMQSCXJuNYK+fw8khEw==") == 0);
  assert(strcmp(dev[0].attributes, "+verification") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "*") == 0);
  assert(strcmp(dev[1].publicKey, "LABCxfeOxfctKo8HmDA/"
                                  "PYyPlwTYj3i2tQu2QWEmi7tytaPQA8ZZZP7hddzpkUQV"
                                  "HRu2oASGigS7oBwt38WFCw==") == 0);
  assert(strcmp(dev[1].attributes, "+verification") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-r-P-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(strcmp(dev[0].publicKey,
                "se1ih71yYTjlvF6p3Kc+wB0PtXv+5RM8O/0/"
                "yFy5RcvEXCvirQPbxhmSIhh5QYP17fkAFGLfJYQtmV7RNU0xDg==") == 0);
  assert(strcmp(dev[0].attributes, "+pin") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "*") == 0);
  assert(strcmp(dev[1].publicKey,
                "v6hgPffPopqHTh19Y0Wf11cF/lChqwlT0f5/"
                "6K+Dsdzq1OPZxKBqTaW6jCU0x5Pr9HgntWyTtQ1TS7EM22uhyQ==") == 0);
  assert(strcmp(dev[1].attributes, "+pin") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_double_-r-P-V-N.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "*") == 0);
  assert(
    strcmp(dev[0].publicKey,
           "+zfrwyH/M5/"
           "tEVfijRKzRqNh+"
           "QoC3JBweJFa0heINIDkCjLAYqUb2hSTecTxoKh2bzpxSqeg6nJJPJNBqDD2aA==") ==
    0);
  assert(strcmp(dev[0].attributes, "+verification+pin") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle, "*") == 0);
  assert(strcmp(dev[1].publicKey,
                "W1vVZhmkt3KG16sraGayBP8elRXSl5UGMl+"
                "oojLI15yyIAVUUzoEKNUQtf3j3s3sVtjD009nLxHOpkf2zjIpQQ==") == 0);
  assert(strcmp(dev[1].attributes, "+verification+pin") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_mixed_12.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "ooq2bCWeHFXzWqKwWFRliREQjOtUWKtWJbr7KwSh3FLNiCFgB"
                "uie4tqq3Pee86o7ew32u1+ITLsCBEYPrTQMAg==") == 0);
  assert(strcmp(dev[0].publicKey,
                "39hCGEGO7kqz3Pig/bL0ycZxLfcpWPtX8fKxb/"
                "S8xx2BdSUs6HXTzIDmifuFv6pabpy3DxUvcA0yIygMAO1ZQw==") == 0);
  assert(strcmp(dev[0].attributes, "+presence") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "2O2vxjSMeMl6obzQCkIE3VL2Qtija5sLJuJkMrP+/"
                "bAFeoLp7m2SPKKRUFFXsO8Z44HTL7PKoFmY4+r5Qwt00w==") == 0);
  assert(strcmp(dev[1].publicKey,
                "qZIaqR+"
                "mGxEnvo04LtsX4krKV5r5PBVBfJYDXi2zS7uXSHgRr7K"
                "OQHaNgx70E2IBrVmUlaFAH4QhDnDAeishBA==") == 0);
  assert(strcmp(dev[1].attributes, "+presence") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_mixed_1-P2.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle, "9HY72OR/"
                                  "kQECy5PbwfJwSaWZFlLL1CHamlm1LMZFozCBj6hzq4V9"
                                  "BpkkkMObxNL9gFd8yOXKDflFiVVoGq7sWQ==") == 0);
  assert(strcmp(dev[0].publicKey,
                "sn+"
                "cAxAvdlnwwwvLCLoEjiza2G0aPniyqgomxmm1aLisMl1z9Vpv"
                "dlGgO4nOPLYZSoRkW6nKvOBzztGYq/knfQ==") == 0);
  assert(strcmp(dev[0].attributes, "+presence") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "43JAMt5EnG72Sx/4C9ptEd+6/f1RMOKWBSQj4U6pnQyVvPFI/"
                "nX2jvN4EYqMQrdGYaszRbh0AL0an3hKZJNqLw==") == 0);
  assert(strcmp(dev[1].publicKey, "aPlEf4vR+SUrOykB1tk+"
                                  "H1XKsEiSIBMK252bPz7kLHusnAgqgPZLqcruFEegChmY"
                                  "yhytWDPluPrw1o16FFyf5Q==") == 0);
  assert(strcmp(dev[1].attributes, "") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_mixed_-P12.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "kNfZ8Uot7TcImjCXhji32Apur3172TYc4XLA0uDQsdW1lrIRe"
                "cyZP5chyPrkNxIrRIZ58UgiMxD72fiaCiQghw==") == 0);
  assert(strcmp(dev[0].publicKey,
                "QiscDH8fpvC9imwd7UiQ8n2XeqNRWW5sUxmboMbiNLUXWrvuG"
                "7pEBvWYQA3yLdmOvvb/3MijCh6AZr/3fpwZKQ==") == 0);
  assert(strcmp(dev[0].attributes, "") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "4ICSXy6FCp7NUErnJAOoyqGOnszEMmnIxjgH2NmiC9cyn0XqB"
                "xmr1+YHt9fv6yhbrPrnn9/QLvysS+VZBc9twQ==") == 0);
  assert(strcmp(dev[1].publicKey,
                "IJMQOa1WrUkBwZKKviNxkMlvKGkiIbXcIdWf+"
                "Rv1BPWI9Xo1edi1LF7ux8sZs6mbQEn3z+v+UCSgO13ZtFzI/w==") == 0);
  assert(strcmp(dev[1].attributes, "+presence") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/new_mixed_-P1-P2.cred";
  cfg.sshformat = 0;
  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 2);

  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].keyHandle,
                "gqCuXGhiA9P4PhXPgrMjQCdgBPkLHHmQcDF/"
                "AMOp9vMuCoreRgwWlckMvCdHnsRTohdGqKZgVT/M3HVu4/UiXA==") == 0);
  assert(strcmp(dev[0].publicKey,
                "DJaEFTDU5XMq5+KwhEwj69zo5KthqvPRcrCE8Rhu6v1FkgFww/"
                "gHYeCZi8s6IRbzmmkxSANXuBAGcpVAM6Zo3A==") == 0);
  assert(strcmp(dev[0].attributes, "") == 0);
  assert(dev[0].old_format == 0);

  assert(strcmp(dev[1].coseType, "es256") == 0);
  assert(strcmp(dev[1].keyHandle,
                "RFgUluPS2nAXHy3++1dyyu5k0Rnr9KHUccYSH2oMdpw+"
                "QWOu5lvHki3lyAxhKm7HTu8wfMK86qIHakTMYDiYSA==") == 0);
  assert(strcmp(dev[1].publicKey,
                "sgoB52Vmw6fqQMDsBHKMsSeft6AfXoULH+"
                "mHNi3nOS6KHnvobo82LFGjvQqxrbSNfIul/cpD3MSdz8R0Tfhl3w==") == 0);
  assert(strcmp(dev[1].attributes, "") == 0);
  assert(dev[1].old_format == 0);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  free(dev[1].coseType);
  free(dev[1].attributes);
  free(dev[1].keyHandle);
  free(dev[1].publicKey);

  memset(dev, 0, sizeof(dev));

  cfg.auth_file = "credentials/old_credential.cred";
  cfg.sshformat = 0;

  rc = get_devices_from_authfile(&cfg, username, dev, &n_devs);
  assert(rc == 1);
  assert(n_devs == 1);
  assert(strcmp(dev[0].coseType, "es256") == 0);
  assert(strcmp(dev[0].attributes, "+presence") == 0);
  printf("kh %s\n", dev[0].publicKey);
  assert(strcmp(dev[0].keyHandle,
                "mGvXxDqTMSVkSlDnDRNTVsP5Ij9cceCkdZkSJYeaJCHCOpBtM"
                "IFGQXKBBkvZpV5bWuEuJkoElIiMKirhCPAU8Q==") == 0);
  assert(
    strcmp(dev[0].publicKey,
           "0405a35641a6f5b63e2ef4449393e7e1cb2b96711e797fc74dbd63e99dbf410ffe7"
           "425e79f8c41d8f049c8f7241a803563a43c139f923f0ab9007fbd0dcc722927") ==
    0);
  assert(dev[0].old_format == 1);

  free(dev[0].coseType);
  free(dev[0].attributes);
  free(dev[0].keyHandle);
  free(dev[0].publicKey);

  return 0;
}
