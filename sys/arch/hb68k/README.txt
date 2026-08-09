$NetBSD: README.txt,v 1.2 2026/08/09 04:00:12 thorpej Exp $

Greetings programs!  This is a space for your homebrew 680x0 projects!
I know where are a few of them out there, so port NetBSD to your board
and contact me to get it into the tree!

Ok, a couple of caveats... I **strongly** encourage all of you to do
your port using Flattened Device Tree (FDT).  I've done the boilerplate
glue in the m68k-specific parts of the kernel for you, and FDT is a
great way to describe your system in a very generalized way that makes it
really easy to get the kernel going with a minimal amount of work.  Even if
you don't have a boot ROM or firmware that understands FDT, it's possible
to embed a device tree blob into a machine-specific kernel binary or work
it into your bootloader, so don't let fear of the unknown hold you back...
If you need help, advice, or emotional support, drop me a line!

Homebrew systems supported by this port:

- Phaethon 1: thorpej's 68010-based system with a custom MMU.
- Wrap030: techav's 68030-based system that started life as a wire-wrapped
  prototype, now lives on an ATX form-factor board.

Other systems that I'd like to see supported by this port:

- crmaykish's Mackerel-30
- KISS-68030 (you can find this on https://www.retrobrewcomputers.org/)
- MAXI030 (https://www.aslak.net/index.php/category/68k/)

...plus whatever goodness you can cook up!

OK!  Let's go, 68k hackers!

Love and rockets...
-- thorpej@netbsd.org
