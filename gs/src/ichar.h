/* Copyright (C) 1994, 1996, 1997, 1999 Aladdin Enterprises.  All rights reserved.
 * This software is licensed to a single customer by Artifex Software Inc.
 * under the terms of a specific OEM agreement.
 */

/*$RCSfile$ $Revision$ */
/* Character rendering operator definitions and support procedures */
/* Requires gstext.h */

#ifndef ichar_INCLUDED
#  define ichar_INCLUDED

/*
 * All the character rendering operators use the execution stack
 * for loop control -- see estack.h for details.
 * The information pushed by these operators is as follows:
 *      the enumerator (t_struct, a gs_text_enum_t);
 *      a slot for the procedure for kshow or cshow (probably t_array);
 *      a slot for the saved o-stack depth for cshow or stringwidth,
 *              and for error recovery (t_integer);
 *      a slot for the saved d-stack depth ditto (t_integer);
 *      a slot for the saved gstate level ditto (t_integer);
 *	a slot for saving the font during the cshow proc (t_struct);
 *	a slot for saving the root font during the cshow proc (t_struct);
 *      the procedure to be called at the end of the enumeration
 *              (t_operator, but called directly, not by the interpreter);
 *      the usual e-stack mark (t_null).
 */
#define snumpush 9
#define esenum(ep) r_ptr(ep, gs_text_enum_t)
#define senum esenum(esp)
#define esslot(ep) ((ep)[-1])
#define sslot esslot(esp)
#define esodepth(ep) ((ep)[-2])
#define sodepth esodepth(esp)
#define esddepth(ep) ((ep)[-3])
#define sddepth esddepth(esp)
#define esgslevel(ep) ((ep)[-4])
#define sgslevel esgslevel(esp)
#define essfont(ep) ((ep)[-5])
#define ssfont essfont(esp)
#define esrfont(ep) ((ep)[-6])
#define srfont esrfont(esp)
#define eseproc(ep) ((ep)[-7])
#define seproc eseproc(esp)

/* Procedures exported by zchar.c for zchar*.c. */
gs_text_enum_t *op_show_find(P1(i_ctx_t *));
int op_show_setup(P2(i_ctx_t *, os_ptr));
int op_show_enum_setup(P1(i_ctx_t *));
int op_show_finish_setup(P4(i_ctx_t *, gs_text_enum_t *, int, op_proc_t));
int op_show_continue(P1(i_ctx_t *));
int op_show_continue_pop(P2(i_ctx_t *, int));
int op_show_continue_dispatch(P3(i_ctx_t *, int, int));
int op_show_free(P2(i_ctx_t *, int));
void glyph_ref(P2(gs_glyph, ref *));

/* Exported by zchar.c for zcharout.c */
bool zchar_show_width_only(P1(const gs_text_enum_t *));
int zsetcachedevice(P1(i_ctx_t *));
int zsetcachedevice2(P1(i_ctx_t *));

#endif /* ichar_INCLUDED */
