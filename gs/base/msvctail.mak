#  Copyright (C) 2001-2006 Artifex Software, Inc.
#  All Rights Reserved.
#
#  This software is provided AS-IS with no warranty, either express or
#  implied.
#
#  This software is distributed under license and may not be copied, modified
#  or distributed except as expressly authorized under the terms of that
#  license.  Refer to licensing information at http://www.artifex.com/
#  or contact Artifex Software, Inc.,  7 Mt. Lassen Drive - Suite A-134,
#  San Rafael, CA  94903, U.S.A., +1(415)492-9861, for further information.
#
# $Id$
# Common tail section for Microsoft Visual C++ 4.x/5.x,
# Windows NT or Windows 95 platform.
# Created 1997-05-22 by L. Peter Deutsch from msvc4/5 makefiles.
# edited 1997-06-xx by JD to factor out interpreter-specific sections
# edited 2000-06-05 by lpd to handle empty MSINCDIR specially.


# -------------------------- Auxiliary programs --------------------------- #

# This also creates the subdirectories since this (hopefully) will be the
# first need. Too bad nmake doesn't have .BEFORE symbolic target.
$(GLGENDIR)\ccf32.tr: $(TOP_MAKEFILES)
	-if not exist $(PSOBJDIR) mkdir $(PSOBJDIR)
	-if not exist $(PSGENDIR) mkdir $(PSGENDIR)
	-if not exist $(PSGENDIR)$(D)cups mkdir $(PSGENDIR)$(D)cups
	-if not exist $(GLOBJDIR) mkdir $(GLOBJDIR)
	-if not exist $(GLGENDIR) mkdir $(GLGENDIR)
	-if not exist $(AUXDIR) mkdir $(AUXDIR)
	-if not exist $(BINDIR) mkdir $(BINDIR)
	echo $(GENOPT) -DCHECK_INTERRUPTS -D_Windows -D__WIN32__ > $(GLGENDIR)\ccf32.tr

$(ECHOGS_XE): $(GLSRC)echogs.c
	$(CCAUX_) $(GLSRC)echogs.c /Fo$(GLOBJ)echogs.obj /Fe$(ECHOGS_XE) $(CCAUX_TAIL)

# Don't create genarch if it's not needed
!ifdef GENARCH_XE
!ifdef WIN64
# The genarch.exe that is generated is 64-bit, so the OS must be able to run it
$(GENARCH_XE): $(GLSRC)genarch.c $(GENARCH_DEPS) $(GLGENDIR)\ccf32.tr
	$(CCAUX_) @$(GLGENDIR)\ccf32.tr /Fo$(AUXDIR)genarch.obj $(GLSRC)genarch.c
	$(LINK) $(LCT) $(LINKLIBPATH) $(AUX)genarch.obj /OUT:$(GENARCH_XE)
!else
$(GENARCH_XE): $(GLSRC)genarch.c $(GENARCH_DEPS) $(GLGENDIR)\ccf32.tr
	$(CCAUX_) @$(GLGENDIR)\ccf32.tr /Fo$(AUX)genarch.obj /Fe$(GENARCH_XE) $(GLSRC)genarch.c $(CCAUX_TAIL)
!endif
!endif

$(GENCONF_XE): $(GLSRC)genconf.c $(GENCONF_DEPS)
	$(CCAUX_) $(GLSRC)genconf.c /Fo$(AUX)genconf.obj /Fe$(GENCONF_XE) $(CCAUX_TAIL)

$(GENDEV_XE): $(GLSRC)gendev.c $(GENDEV_DEPS)
	$(CCAUX_) $(GLSRC)gendev.c /Fo$(AUX)gendev.obj /Fe$(GENDEV_XE) $(CCAUX_TAIL)

$(GENHT_XE): $(GLSRC)genht.c $(GENHT_DEPS)
	$(CCAUX_) $(GENHT_CFLAGS) $(GLSRC)genht.c /Fo$(AUX)genht.obj /Fe$(GENHT_XE) $(CCAUX_TAIL)

MKROMFS_OBJS=$(MKROMFS_ZLIB_OBJS) $(winplat_) $(GLOBJ)gpmisc.$(OBJ) $(GLOBJ)gp_getnv.$(OBJ) $(GLOBJ)gp_wutf8.$(OBJ)
$(MKROMFS_XE): $(GLSRC)mkromfs.c $(MKROMFS_COMMON_DEPS) $(MKROMFS_OBJS)
	$(CCAUX_) -I$(GLOBJ) -I$(ZSRCDIR) @$(GLGENDIR)\ccf32.tr $(GLSRC)mkromfs.c /Fo$(AUX)mkromfs.obj /Fe$(MKROMFS_XE) $(MKROMFS_OBJS) $(CCAUX_TAIL) /DEBUG

# -------------------------------- Library -------------------------------- #

# See winlib.mak

# ----------------------------- Main program ------------------------------ #

LIBCTR=$(GLGEN)libc32.tr

$(LIBCTR): $(TOP_MAKEFILES)
	echo shell32.lib >$(LIBCTR)
	echo comdlg32.lib >>$(LIBCTR)
	echo gdi32.lib >>$(LIBCTR)
	echo user32.lib >>$(LIBCTR)
	echo winspool.lib >>$(LIBCTR)
	echo advapi32.lib >>$(LIBCTR)
	echo ws2_32.lib >>$(LIBCTR)

# end of msvctail.mak
