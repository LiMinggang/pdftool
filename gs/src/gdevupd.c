/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/* gdevupd.c Revision: 1.77  */
/* "uniprint" -- Ugly Printer Driver by Gunther Hess (gunther@elmos.de) */

/* Revision-History:
   23-Mar-1997 -  1.43: First published version
   24-Mar-1997 -  1.44: gs4.03 compatible version on the web
   31-Mar-1997 -  1.53: First Version inside gs-fileset (limited)
   28-Apr-1997 -  1.54: Version intended for public gs-release
   4-May-1997 -  1.55: Deactivated an accidentially active Debug-Option
   14-Jun-1997 -  1.56: Bug-Workaround for White on White Printing (gs5.0)
   17-Jun-1997 -  1.57: More reasonable Fix for the above Bug
   ...
   7-Jul-1997 -  1.68: NULL-Param-BUG, HR's BJC, Pwidth/-height BUG, YFlip
   25-Jul-1997 -  1.69: Bug-Fix: incomplete Change of PHEIGHT-Treatment
   4-Aug-1997 -  1.70: Arrgh: still incomplete Change of PHEIGHT-Treatment
   17-AUG-1997 -  1.71: Fix of BSD-sprintf bug. (returns char * there)
   ...
   28-Sep-1977 -  1.77: Fixed the byte<>char and casted-lvalue Problems
 */

/* Canon BJC 610 additions from (hr)
   Helmut Riegler <helmut-riegler@net4you.co.at>

   The BJC-4000 can be supported very easily, only by creating the right .upp
   parameter file. If you have this printer and you are willing to do this,
   contact me, I'll give you the technical details (ESC codes).
 */

/* ------------------------------------------------------------------- */
/* Compile-Time-Options                                                */
/* ------------------------------------------------------------------- */

/**
There are two compile-time options for this driver:
   1. UPD_SIGNAL   enables interrupt detection, that aborts printing and
   2. UPD_MESSAGES controls the amount of messages generated by the driver
*/

#ifndef   UPD_SIGNAL
#ifdef      __unix__
#define       UPD_SIGNAL 1
/** Activated, if undefined, on UNIX-Systems */
