/* Copyright (C) 1992, 1993, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/*Id: gscolor2.h */
/* Client interface to Level 2 color facilities */
/* (requires gscspace.h, gsmatrix.h) */

#ifndef gscolor2_INCLUDED
#  define gscolor2_INCLUDED

#include "gsptype1.h"

/* Note: clients should use rc_alloc_struct_0 (in gsrefct.h) to allocate */
/* CIE color spaces or rendering structures. */

/* ---------------- Graphics state ---------------- */

/* General color routines */
const gs_color_space *gs_currentcolorspace(P1(const gs_state *));
int gs_setcolorspace(P2(gs_state *, gs_color_space *));
const gs_client_color *gs_currentcolor(P1(const gs_state *));
int gs_setcolor(P2(gs_state *, const gs_client_color *));

/* CIE-specific routines */
#ifndef gs_cie_render_DEFINED
#  define gs_cie_render_DEFINED
typedef struct gs_cie_render_s gs_cie_render;
#endif
const gs_cie_render *gs_currentcolorrendering(P1(const gs_state *));
int gs_setcolorrendering(P2(gs_state *, gs_cie_render *));

/* ---------------- Indexed color space ---------------- */

/*
 * Indexed color spaces.
 *
 * If the color space will use a procedure rather than a byte table,
 * ptbl should be set to 0.
 *
 * Unlike most of the other color space constructors, this one initializes
 * some of the fields of the colorspace. In the case in which a string table
 * is used for mapping, it initializes the entire structure. Note that the
 * client is responsible for the table memory in that case; the color space
 * will not free it when the color space itself is released.
 *
 * For the case of an indexed color space based on a procedure, a default
 * procedure will be provided that simply echoes the color values already in
 * the palette; the client may override these procedures by use of
 * gs_cspace_indexed_set_proc. If the client wishes to insert values into
 * the palette, it should do so by using gs_cspace_indexed_value_array, and
 * directly inserting the desired values into the array.
 *
 * If the client does insert values into the palette directly, the default
 * procedures provided by the client are fairly efficient, and there are
 * few instances in which the client would need to replace them.
 */
extern  int     gs_cspace_build_Indexed(
    gs_color_space **       ppcspace,
    const gs_color_space *  pbase_cspace,
    uint                    num_entries,
    const gs_const_string * ptbl,
    gs_memory_t *           pmem
);

/* Return the number of entries in the palette of an indexed color space. */
extern int gs_cspace_indexed_num_entries(P1(
    const gs_color_space *  pcspace
));

/* In the case of a procedure-based indexed color space, get a pointer to */
/* the array of cached values. */
extern float * gs_cspace_indexed_value_array(P1(
    const gs_color_space *  pcspace
));

/* Set the lookup procedure to be used for an Indexed color space. */
extern int gs_cspace_indexed_set_proc(P2(
    gs_color_space *    pcspace,
    int                 (*proc)(P3( const gs_indexed_params *, int, float * ))
));

#endif					/* gscolor2_INCLUDED */
