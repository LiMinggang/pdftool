/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.

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

/*Id: gxshade4.h  */
/* Internal definitions for triangle shading rendering */

#ifndef gxshade4_INCLUDED
#  define gxshade4_INCLUDED

/*
 * Define the fill state structure for triangle shadings.  This is used
 * both for the Gouraud triangle shading types and for the Coons and
 * tensor patch types.
 *
 * The shading pointer is named pshm rather than psh in case subclasses
 * also want to store a pointer of a more specific type.
 */
#define mesh_fill_state_common\
  shading_fill_state_common;\
  const gs_shading_mesh_t *pshm;\
  gs_fixed_rect rect
typedef struct mesh_fill_state_s {
    mesh_fill_state_common;
} mesh_fill_state_t;

/* Initialize the fill state for triangle shading. */
void mesh_init_fill_state(P5(mesh_fill_state_t * pfs,
			const gs_shading_mesh_t * psh, const gs_rect * rect,
			     gx_device * dev, gs_imager_state * pis));

/* Fill one triangle in a mesh. */
int mesh_fill_triangle(P8(const mesh_fill_state_t * pfs,
			  const gs_fixed_point * pa, const float *pca,
			  const gs_fixed_point * pb, const float *pcb,
			  const gs_fixed_point * pc, const float *pcc,
			  bool check_clipping));

#endif /* gxshade4_INCLUDED */
