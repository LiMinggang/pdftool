/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
 * This software is licensed to a single customer by Artifex Software Inc.
 * under the terms of a specific OEM agreement.
 */

/*$RCSfile$ $Revision$ */
/* ASCII85Decode filter interface */
/* Requires scommon.h; strimpl.h if any templates are referenced */

#ifndef sa85d_INCLUDED
#  define sa85d_INCLUDED

/* ASCII85Decode */
typedef struct stream_A85D_state_s {
    stream_state_common;
    int odd;			/* # of odd digits */
    ulong word;			/* word being accumulated */
} stream_A85D_state;

#define private_st_A85D_state()	/* in sfilter2.c */\
  gs_private_st_simple(st_A85D_state, stream_A85D_state,\
    "ASCII85Decode state")
/* We define the initialization procedure here, so that the scanner */
/* can avoid a procedure call. */
#define s_A85D_init_inline(ss)\
  ((ss)->word = 0, (ss)->odd = 0)
extern const stream_template s_A85D_template;

#endif /* sa85d_INCLUDED */
