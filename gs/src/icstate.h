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

/*Id: icstate.h  */
/* Externally visible context state */
/* Requires iref.h */

#ifndef icstate_INCLUDED
#  define icstate_INCLUDED

#include "imemory.h"

/*
 * Define the externally visible state of an interpreter context.
 * If we aren't supporting Display PostScript features, there is only
 * a single context.
 */
#ifndef gs_context_state_t_DEFINED
#  define gs_context_state_t_DEFINED
typedef struct gs_context_state_s gs_context_state_t;

#endif
#ifndef ref_stack_DEFINED
#  define ref_stack_DEFINED
typedef struct ref_stack_s ref_stack;

#endif
struct gs_context_state_s {
    ref_stack *dstack;
    ref_stack *estack;
    ref_stack *ostack;
    gs_state *pgs;
    gs_dual_memory_t memory;
    ref array_packing;		/* t_boolean */
    ref binary_object_format;	/* t_integer */
    long rand_state;		/* (not in Red Book) */
    long usertime_total;	/* total accumulated usertime, */
    /* not counting current time if running */
    bool keep_usertime;		/* true if context ever executed usertime */
    /* View clipping is handled in the graphics state. */
    ref userparams;		/* t_dictionary */
    ref stdio[2];		/* t_file */
};

/*
 * We make st_context_state public because interp.c must allocate one,
 * and zcontext.c must subclass it.
 */
				  /*extern_st(st_context_state); *//* in icontext.h */
#define public_st_context_state()	/* in icontext.c */\
  gs_public_st_complex_only(st_context_state, gs_context_state_t,\
    "gs_context_state_t", context_state_clear_marks,\
    context_state_enum_ptrs, context_state_reloc_ptrs, 0)

#endif /* icstate_INCLUDED */
