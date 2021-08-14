#!/bin/bash

# Launch unitary tests
# 


CONFIG_FILE="ppm.example"

LDAP_SRC="${LDAP_SRC:-../../..}"
LDAP_BUILD=${LDAP_BUILD:-${LDAP_SRC}}
CURRENT_DIR=$( dirname $0 )
LIB_PATH="${LD_LIBRARY_PATH}:${CURRENT_DIR}:${LDAP_BUILD}/libraries/liblber/.libs:${LDAP_BUILD}/libraries/libldap/.libs"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

RESULT=0

PPM_CONF_1='minQuality 3
checkRDN 0
forbiddenChars 
maxConsecutivePerClass 0
useCracklib 0
cracklibDict /var/cache/cracklib/cracklib_dict
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1
class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1
class-digit 0123456789 0 1
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'\''{([-|è`_\ç^à@)]°=}+ 0 1'

PPM_CONF_2='minQuality 3
checkRDN 0
forbiddenChars à
maxConsecutivePerClass 5
useCracklib 0
cracklibDict /var/cache/cracklib/cracklib_dict
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 2 4
class-lowerCase abcdefghijklmnopqrstuvwxyz 3 4
class-digit 0123456789 2 4
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'\''{([-|è`_\ç^à@)]°=}+ 0 4'

PPM_CONF_3='minQuality 3
checkRDN 1
forbiddenChars 
maxConsecutivePerClass 0
useCracklib 0
cracklibDict /var/cache/cracklib/cracklib_dict
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1
class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1
class-digit 0123456789 0 1
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'\''{([-|è`_\ç^à@)]°=}+ 0 1'


echo "$PPM_CONF_1" > ppm1.conf
echo "$PPM_CONF_2" > ppm2.conf
echo "$PPM_CONF_3" > ppm3.conf


launch_test()
{
  # launch tests
  # FORMAT: launch_test [conf_file] [password] [expected_result]
  # [expected_result] = [PASS|FAIL]

  local CONF="$1"
  local USER="$2"
  local PASS="$3"
  local EXPECT="$4"

  [[ $EXPECT == "PASS" ]] && EXP="0" || EXP="1"

  LD_LIBRARY_PATH="${LIB_PATH}" ./ppm_test "${USER}" "${PASS}" "${CONF}"
  RES="$?"

  if [ "$RES" -eq "$EXP" ] ; then
    echo -e "conf=${CONF} user=${USER} pass=${PASS} expect=${EXPECT}... ${GREEN}PASS${NC}"
  else
    echo -e "conf=${CONF} user=${USER} pass=${PASS} expect=${EXPECT}... ${RED}FAIL${NC}"
    ((RESULT+=1))
  fi

  echo
}




launch_test "ppm1.conf" "uid=test,ou=users,dc=my-domain,dc=com" "azerty" "FAIL"
launch_test "ppm1.conf" "uid=test,ou=users,dc=my-domain,dc=com" "azeRTY" "FAIL"
launch_test "ppm1.conf" "uid=test,ou=users,dc=my-domain,dc=com" "azeRTY123" "PASS"
launch_test "ppm1.conf" "uid=test,ou=users,dc=my-domain,dc=com" "azeRTY." "PASS"


launch_test "ppm2.conf" "uid=test,ou=users,dc=my-domain,dc=com" "AAaaa01AAaaa01AAaaa0" "PASS"
# forbidden char
launch_test "ppm2.conf" "uid=test,ou=users,dc=my-domain,dc=com" "AAaaa01AAaaa01AAaaaà" "FAIL"
# too much consecutive for upper
launch_test "ppm2.conf" "uid=test,ou=users,dc=my-domain,dc=com" "AAaaa01AAaaa01AAAAAA" "FAIL"
# not enough upper
launch_test "ppm2.conf" "uid=test,ou=users,dc=my-domain,dc=com" "Aaaaa01aaaaa01aa.;.;" "FAIL"
# not enough lower
launch_test "ppm2.conf" "uid=test,ou=users,dc=my-domain,dc=com" "aaAAA01BB0123AAA.;.;" "FAIL"
# not enough digit
launch_test "ppm2.conf" "uid=test,ou=users,dc=my-domain,dc=com" "1AAAA.;BBB.;.;AA.;.;" "FAIL"
# not enough points (no point for digit)
launch_test "ppm2.conf" "uid=test,ou=users,dc=my-domain,dc=com" "AAaaaBBBBaaa01AAaaaa" "FAIL"

# password in RDN
launch_test "ppm3.conf" "uid=User_Password10-test,ou=users,dc=my-domain,dc=com" "Password10" "FAIL"
launch_test "ppm3.conf" "uid=User_Passw0rd-test,ou=users,dc=my-domain,dc=com" "Password10" "PASS"
launch_test "ppm3.conf" "uid=User-Pw-Test,ou=users,dc=my-domain,dc=com" "Password10" "PASS"


echo "${RESULT} error(s) encountered"

rm ppm1.conf ppm2.conf ppm3.conf
exit ${RESULT}

