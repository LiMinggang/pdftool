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

/*Id: zcspixel.c  */
/* DevicePixel color space support */
#include "ghost.h"
#include "oper.h"
#include "igstate.h"
#include "gscspace.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gscolor2.h"
#include "gscpixel.h"

/* <array> .setdevicepixelspace - */
private int
zsetdevicepixelspace(register os_ptr op)
{
    ref depth;
    gs_color_space cs;
    int code;

    check_read_type(*op, t_array);
    if (r_size(op) != 2)
	return_error(e_rangecheck);
    array_get(op, 1L, &depth);
    check_type_only(depth, t_integer);
    switch (depth.value.intval) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
	case 24:
	case 32:
	    break;
	default:
	    return_error(e_rangecheck);
    }
    gs_cs_init_DevicePixel(&cs, (int)depth.value.intval);
    code = gs_setcolorspace(igs, &cs);
    if (code >= 0)
	pop(1);
    return code;
}

/* ------ Initialization procedure ------ */

const op_def zcspixel_op_defs[] =
{
    {"1.setdevicepixelspace", zsetdevicepixelspace},
    op_def_end(0)
};
