
SQZh v1.01 - A C header file compressor.

If you write OS/2 programs you probably copy the OS/2 & PM headers to a RAM
disk.  SQZh can reduce the amount of space that header files use.  A 40 - 50%
reduction is normal.  The reduction is acheived by removing comments and white
space only.

This is a FAPI app and I have used it successfully on OS/2 1.x and 2.0 headers
as well as Windows 3.x header files.  I have included the source code, and I
would not feel insulted at all if the program was improved or modified. There
is at least one optimization not done (a blank after an id but before an
operator) and there may be others.  The file search algorithm is weak and the
command line interface could be improved/enhanced.

The code is optimized at the expense of reuseability/reentrancy.  For such a
small program that is constantly used, I believe that this is a valid trade
off.


Enjoy: Nick Bethmann
You can reach me at: OS/2 Shareware BBS - 703-385-4325 (open access) 14.4K bps

SQZh v1.02, 6 Jan 1993

Bug fix: char immediately after '/' was dupped!

SQZh v1.01, 9 Jun 1992

Changed command line to take target first and then any number of source
directories.  This is more efficient than multiple calls.

Fixed 1 bug. Was forcing '#' char to be first char on line, bad idea given
token pasting macros like assert.  I didn't really need that force anyways,
it was left over from trying to rip out most cr/lf's.

SQZh v1.00 - first version, 3 May 1992


