/* machine description file template.
   Copyright (C) 1985, 1986, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
                 2008 Free Software Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.  */


/* The following line tells the configuration script what sort of
   operating system this machine is likely to run.
   USUAL-OPSYS="<name of system .h file here, without the s- or .h>"  */

/* Define WORDS_BIG_ENDIAN if lowest-numbered byte in a word
   is the most significant byte.  */

#define WORDS_BIG_ENDIAN

/* Define NO_ARG_ARRAY if you cannot take the address of the first of a
 * group of arguments and treat it as an array of the arguments.  */

#define NO_ARG_ARRAY

/* Define EXPLICIT_SIGN_EXTEND if XINT must explicitly sign-extend
   the 24-bit bit field into an int.  In other words, if bit fields
   are always unsigned.

   This flag only matters if you use USE_LISP_UNION_TYPE.  */

#define EXPLICIT_SIGN_EXTEND

/* Data type of load average, as read out of kmem.  */

#define LOAD_AVE_TYPE long

/* Convert that into an integer that is 100 for a load average of 1.0  */

#define LOAD_AVE_CVT(x) (int) (((double) (x)) * 100.0 / FSCALE)

/* Define VIRT_ADDR_VARIES if the virtual addresses of
   pure and impure space as loaded can vary, and even their
   relative order cannot be relied on.

   Otherwise Emacs assumes that text space precedes data space,
   numerically.  */

#define VIRT_ADDR_VARIES

/* Define NO_REMAP if memory segmentation makes it not work well
   to change the boundary between the text section and data section
   when Emacs is dumped.  If you define this, the preloaded Lisp
   code will not be sharable; but that's better than failing completely.  */

/*#define NO_REMAP */

/* Some really obscure 4.2-based systems (like Sequent DYNIX)
 * do not support asynchronous I/O (using SIGIO) on sockets,
 * even though it works fine on tty's.  If you have one of
 * these systems, define the following, and then use it in
 * config.h (or elsewhere) to decide when (not) to use SIGIO.
 *
 * You'd think this would go in an operating-system description file,
 * but since it only occurs on some, but not all, BSD systems, the
 * reasonable place to select for it is in the machine description
 * file.
 */

/*#define NO_SOCK_SIGIO*/


/* After adding support for a new system, modify the large case
   statement in the `configure' script to recognize reasonable
   configuration names, and add a description of the system to
   `etc/MACHINES'.

   If you've just fixed a problem in an existing configuration file,
   you should also check `etc/MACHINES' to make sure its descriptions
   of known problems in that configuration should be updated.  */

/* arch-tag: d8a0ffa4-a8f0-4736-90d3-7fd7b21b8314
   (do not change this comment) */
