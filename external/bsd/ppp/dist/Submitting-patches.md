How to contribute patches to the PPP project.
=============================================

The PPP project source code is maintained in a Git repository, which
is publicly available at

git://git.ozlabs.org/~paulus/ppp.git

and

https://github.com/paulusmack/ppp.git

The linux-ppp@vger.kernel.org mailing list is a suitable place to
discuss issues relating to the PPP code and to post patches.

Although there is a copy of the repository on github.com, the PPP
project is not a "github project", in the sense that the development
of the code does not depend on github infrastructure.  In particular,
patch descriptions should be understandable without reference to any
github.com web page.  Thus, patches or commits whose description
consists only of something like "Closes #123" will be rejected.  See
below for the minimum standards for patch descriptions.

There are two ways in which patches can be submitted for review:

1. Post the patch on the linux-ppp@vger.kernel.org mailing list.  This
   is my preferred way to receive patches, because it provides more
   opportunity for other interested people to review the patches.

2. Create a pull request on github.com.  However, please don't make
   the mistake of creating a commit with a minimal commit message and
   then explaining the rationale for the change in the pull request.
   Put the rationale in the commit message.

Requirements for patch/commit description
-----------------------------------------

The description on a patch, which becomes the commit message in the
resulting commit, should describe the reason why the change is being
made.  If it fixes a bug, it should describe the bug in enough detail
for the reader to understand why and how the change being made fixes
the bug.  If it adds a feature, it should describe the feature and how
it might be used and why it would be desirable to have the feature.

Normally the patch description should be a few paragraphs in length --
it can be longer for a really subtle bug or complex feature, or
shorter for obvious or trivial changes such as fixing spelling
mistakes.

The first line of the commit message is the "headline", corresponding
to the subject line of an emailed patch.  If the patch is concerned
with one particular area of the package, it is helpful to put that at
the beginning, followed by a colon (':'), for example, "pppd: Fix bug
in vslprintf".  The remainder of the headline should be a sentence and
should start with a capital letter.

Note that as maintainer I will edit the headline or the commit message
if necessary to make it clearer or to fix spelling or grammatical
errors.  For a github pull request I may cherry-pick the commits and
modify their commit messages.

References to information on web sites are permitted provided that the
full URL is given, and that reference to the web site is not essential
for understanding the change being made.  For example, you can refer
to a github issue provided that you also put the essential details of
the issue in the commit message, and put the full URL for the issue,
not just the issue number.

Signoff
-------

In order to forestall possible (though unlikely) future legal
problems, this project requires a "Signed-off-by" line on all
non-trivial patches, with a real name (not just initials, and no
pseudonyms).  Signing off indicates that you certify that your patch
meets the 'Developer's Certificate of Origin' below (taken from the
DCO 1.1 in the Linux kernel source tree).

Developer's Certificate of Origin
---------------------------------

By making a contribution to this project, I certify that:

 (a) The contribution was created in whole or in part by me and I
     have the right to submit it under the open source license
     indicated in the file; or

 (b) The contribution is based upon previous work that, to the best
     of my knowledge, is covered under an appropriate open source
     license and I have the right under that license to submit that
     work with modifications, whether created in whole or in part
     by me, under the same open source license (unless I am
     permitted to submit under a different license), as indicated
     in the file; or

 (c) The contribution was provided directly to me by some other
     person who certified (a), (b) or (c) and I have not modified
     it.

 (d) I understand and agree that this project and the contribution
     are public and that a record of the contribution (including all
     personal information I submit with it, including my sign-off) is
     maintained indefinitely and may be redistributed consistent with
     this project or the open source license(s) involved.

