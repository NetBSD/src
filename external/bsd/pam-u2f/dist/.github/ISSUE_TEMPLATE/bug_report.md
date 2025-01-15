---
name: Bug report
about: Report a bug in pam-u2f
---

<!--
Please use the questions below as a template. Review your answers for
potentially sensitive information. Thank you!
-->

### What version of pam-u2f are you using?

<!--
Please also share how you installed pam-u2f.
-->

### What operating system are you using?

<!--
Please also provide other relevant information about your environment.
For example, what version of libfido2 is installed on your system.
-->

### What authenticator are you using?

<!--
Please include the output of `fido2-token -I`. This helps us understand
your authenticator's capabilities.

fido2-token is distributed with the fido2-tools package on Ubuntu and
Debian. Other distributions may package it directly with libfido2.
-->

### Problem description

<!--
Describe the problem you're facing. If you already know of a possible
solution, please also share whether you'd be open to submitting a pull
request.

Please also provide your PAM configuration and debug output. Debug
output can be enabled for the module itself using the `debug` option.
Debug output for pamu2fcfg can be toggled with the --debug flag on the
command line.

You are strongly encouraged to only capture debug output using test
credentials. Failure to do so may disclose sensitive information.
-->
