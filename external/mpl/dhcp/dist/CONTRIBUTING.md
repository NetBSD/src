# ISC DHCP Contributor's Guide

So you found a bug in ISC DHCP or plan to develop an extension and want to send us a patch? Great!
This page will explain how to contribute your changes smoothly.

We do not require a contributors agreement. By submitting a patch or merge request to this project,
you are agreeing that your code will be covered by the primary license for the project.
ISC DHCP is currently licensed under the MPL2.0 license.

**NOTE**: The client (dhclient) and relay (dhcrelay) component maintenance is coming to an end.
The 4.4.3 release is the last one that included those components and they are now considered EOL.
The 4.5.0 release will feature server (dhcpd) only. You may still submit bugs for a client or
relay, but those will be kept for informational purposes only. There will not be any action
taken by ISC team on those.

Here's are the steps in contributing a patch:

1. **create account** on [gitlab](https://gitlab.isc.org)
2. **open an issue** in [this project](https://gitlab.isc.org/isc-projects/dhcp/issues/new), make sure
   it describes what you want to fix and **why**. ISC DHCP is very mature code, with a large installed base.
   We are fairly conservative about making changes unless there is a very good reason.
3. **ask someone from the ISC team to give you a 'project allocation' so you can to fork ISC DHCP in our repo** (ask on the issue - mention @tomek, @vicky, @ondrej
   or @godfryd if it seems we haven't noticed your request)
4. **fork the DHCP master branch**: go to the DHCP project page, click the [Fork button](https://gitlab.isc.org/isc-projects/dhcp/forks/new).
   If you can't, you didn't complete step 3. It helps to include the issue number and subject in the branch name.
5. **Implement your fix or feature, in your branch**. Make sure it compiles, has unit-tests,
   is documented and does what it's supposed to do.
6. **Open Merge Request**: go to the DHCP project [merge requests page](https://gitlab.isc.org/isc-projects/dhcp/merge_requests), and
   click [New merge request](https://gitlab.isc.org/isc-projects/dhcp/merge_requests/new). If you
   don't see the button, you didn't complete step 3.
7. **Participate in the code review**: Once you submit the MR, someone from ISC will eventually get
   to the issue and will review your code. Please make sure you respond to comments. It's likely
   you'll be asked to update the code.

See the text below for more details.


## Create an issue

The first step in contributing to ISC DHCP is to [create an issue](https://gitlab.isc.org/isc-projects/dhcp/issues/new), describing the problem, deficiency
or missing feature you want to address.  It is important to make it very clear why the specific change
you are proposing should be made. ISC DHCP is very mature code, with a large and somewhat inert installed base.
We are very cautious about introducing changes that could break existing functionalty.  If you want to fix
multiple problems, or make multiple changes, please make separate issues for each.

## Plan your changes

Before you start working on a patch or a new feature, it is a good idea to discuss it first with
DHCP developers.  You may benefit from reading the [ISC DHCP Developer's Survival Guide](https://gitlab.isc.org/isc-projects/dhcp/wikis/home)
posted on the wiki page for this repo.

You can post questions about development on the [dhcp-workers](https://lists.isc.org/mailman/listinfo/dhcp-workers)
or [dhcp-users](https://lists.isc.org/mailman/listinfo/dhcp-users) mailing lists. Dhcp-users is
intended for users who are not interested in development details: it is appropriate to ask for
feedback regarding the best proposed solution to a certain problem. The internal details,
questions about the code and its internals are better asked  on dhcp-workers. The dhcp-workers
list is a very low traffic list.


## Create a branch for your work

These instructions assume you will be making your changes on a branch in the ISC DHCP Gitlab
repository. This is by far the easiest way for us to collaborate with you.  While we also maintain a presence
on [Github](https://github.com/isc-projects/dhcp), ISC developers rarely look at Github, which is
just a mirror of our Gitlab system.

ISC's Gitlab has been a target for spammers, so it is set up defensively. New users need permission
from ISC to create new projects.  We gladly do this for anyone who asks and provides a good reason.
"I'd like to fix bug X or develop feature Y" is an excellent reason. To request a project
allocation in ISC's Gitlab, just ask for it in a comment in your issue. Make sure
you tag someone at ISC (@tomek, @godfryd, @vicky or @ondrej). When you write a comment in an issue or
merge request and add a name tag on it, the user is automatically notified.

Once you are given a 'project allocation' in our Gitlab, you can fork ISC DHCP and create a branch.
This is your copy of ISC DHCP and is where you will make your changes. Go to the DHCP project page,
click the [Fork button](https://gitlab.isc.org/isc-projects/dhcp/forks/new) and you will be prompted
to name your branch. It helps to include the issue number and subject in the branch name. You can make
changes to this branch without worrying that you will impact the master branch - commit priviliges
are restricted so you cannot accidentally alter the master branch.

Please read the [Gitlab How-To](https://gitlab.isc.org/isc-projects/dhcp/wikis/processes/gitlab-howto) for ISC DHCP.


## Implement your change

Please try to conform to the project's coding standards. ISC DHCP uses the same [coding standards](https://gitlab.isc.org/isc-projects/bind9/blob/master/doc/dev/style.md) as the BIND 9 project. https://gitlab.isc.org/isc-projects/bind9/blob/master/doc/dev/style.md


## Compile your code

We don't yet have continuous integration set up for ISC DHCP, so you have to check the compilation manually.
ISC DHCP is used on a wide array of UNIX and Linux operating systems. Will your code compile and work there?
What about endianness? It is likely that you used a regular x86 architecture machine to write your
patch, but the software is expected to run on many other architectures. .

## Run unit-tests

One of the ground rules in all ISC projects is that every piece of code has to be tested. For newer
projects, we require a unit-test for almost every line of code. For older code, such as
ISC DHCP, that was not developed with testability in mind, it's unfortunately impractical to require
extensive unit-tests. Having said that, please think thoroughly if there is any way to develop
unit-tests. The long term goal is to improve the situation.

Where unit tests are not practical, supplying us with things like configuration file(s), lease file(s),
PCAPS, and step-by-step on how you tested the changes would be a big help.  This will aid us in
creating and adding system tests to the build farm.

You should have also conducted some sort of system testing to verify that your changes do what you
want. It would be extremely helpful if you can attach any configuration files (dhcpd and or
dhclient), along with a step-by-step procedure to carry out the test(s).  This will help us verify
your changes as extend our own system tests.

Make sure you have ATF (Automated Test Framework) installed in your system. For more information
about ATF, please refer to <dhcp source tree>/doc/devel/atf.dox.  Note, running "make devel" in this
directory will generate the documentation. To run the unit-tests, simply run:

```bash
./configure --with-atf
make
make check
```

If you happen to add new files or have modified any Makefile.am files, it is also a good idea to
check if you haven't broken the distribution process:

```bash
make distcheck
```

There are other useful switches which can be passed to configure. A complete list of all switches
can be obtained with the command:

```bash
./configure --help
```

## Create a Merge Request

Once you feel that your patch is ready, go to the DHCP project
and [submit a Merge Request](https://gitlab.isc.org/isc-projects/dhcp/merge_requests/new).

If you can't access this link or don't see New Merge Request button on the [merge requests
page](https://gitlab.isc.org/isc-projects/dhcp/merge_requests), please ask on dhcp-workers and someone
will help you out.

Once you submit it, someone from the DHCP development team will look at it and will get back to you.
The dev team is very small, so it may take a while...

## If you really can't do a merge request on ISC's Gitlab...

Well, you are out of luck. There are other ways, but those are really awkward and the chances of
your patch being ignored are really high. Anyway, here they are:

- Create a ticket in the DHCP Gitlab (https://gitlab.isc.org/isc-projects/dhcp) and attach your
  patch to it. Sending a patch has a number of disadvantages. First, if you don't specify the base
  version against which it was created, one of ISC engineers will have to guess that or go through
  a series of trials and errors to find that out. If the code doesn't compile, the reviewer will not
  know if the patch is broken or maybe it was applied to incorrect base code. Another frequent
  problem is that it may be possible that the patch didn't include any new files you have added.

- Send a patch to the dhcp-workers list. This is even worse, but still better than not getting the
  patch at all. The problem with this approach is that we don't know which version the patch was
  created against and there is no way to track it. So the chances of it being forgotten are high.
  Once a DHCP developer get to it, the first thing he/she will have to do is try to apply your
  patch, create a branch commit your changes and then open MR for it.

## Going through a review

Once the merge request (MR) is in the system, the action is on one of the core developers.

Sooner or later, one of these engineers will do the review. Unfortunately, we have a small team
and we have a lot of users to support, so it may take a while for us to get to your patch.
Having said that, we value external contributions very much and will do whatever we
can to review patches in a timely manner.

Don't get discouraged if your patch is not accepted on the first review. To keep the code
quality high, we use the same review processes for external patches as we do for internal code.
It may take some cycles of review/updated patch submissions before the code is finally accepted.
The nature of the review process is that it emphasizes areas that need improvement. If you are
not used to the review process, you may get the impression that the feedback is negative. It
is not: even the core developers seldom see reviews that say "All OK please merge".

If we happen to have any comments that you as submitter are expected to address (and in the
overwhelming majority of cases, we have), you will be asked to update your MR. It is common
to see several rounds of such reviews.

Once the process is almost complete, the developer will likely ask you how you would like to be
credited. The typical answers are by first and last name, by nickname, by company name or
anonymously. Typically we will add a note to the ChangeLog and also set you as the author of the
commit applying the patch and update the contributors section in the AUTHORS file. If the
contributed feature is big or critical for whatever reason, it may also be mentioned in release
notes.

Sadly, we sometimes see patches that are submitted and then the submitter never responds to our
comments or requests for an updated patch. Depending on the nature of the patch, we may either fix
the outstanding issues on our own and get another engineer to review them or the ticket may end
up in our Outstanding milestone. When a new release is started, we go through the tickets in
Outstanding, select a small number of them and move them to whatever the current milestone is. Keep
that in mind if you plan to submit a patch and forget about it. We may accept it eventually, but
it's a much, much faster process if you participate in it.

**Thank you for contributing your time and expertise to the ISC DHCP Project.**
