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

/* gsmemlok.c */
/* Monitor-locked heap memory allocator */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */

#include "gx.h"
#include "gsmemlok.h"
#include "gserrors.h"

private gs_memory_proc_alloc_bytes(gs_locked_alloc_bytes);
private gs_memory_proc_alloc_bytes(gs_locked_alloc_bytes_immovable);
private gs_memory_proc_alloc_struct(gs_locked_alloc_struct);
private gs_memory_proc_alloc_struct(gs_locked_alloc_struct_immovable);
private gs_memory_proc_alloc_byte_array(gs_locked_alloc_byte_array);
private gs_memory_proc_alloc_byte_array(gs_locked_alloc_byte_array_immovable);
private gs_memory_proc_alloc_struct_array(gs_locked_alloc_struct_array);
private gs_memory_proc_alloc_struct_array(gs_locked_alloc_struct_array_immovable);
private gs_memory_proc_resize_object(gs_locked_resize_object);
private gs_memory_proc_object_size(gs_locked_object_size);
private gs_memory_proc_object_type(gs_locked_object_type);
private gs_memory_proc_free_object(gs_locked_free_object);
private gs_memory_proc_alloc_string(gs_locked_alloc_string);
private gs_memory_proc_alloc_string(gs_locked_alloc_string_immovable);
private gs_memory_proc_resize_string(gs_locked_resize_string);
private gs_memory_proc_free_string(gs_locked_free_string);
private gs_memory_proc_register_root(gs_locked_register_root);
private gs_memory_proc_unregister_root(gs_locked_unregister_root);
private gs_memory_proc_status(gs_locked_status);
private gs_memory_proc_enable_free(gs_locked_enable_free);
private gs_memory_procs_t locked_procs =
{
    gs_locked_alloc_bytes,
    gs_locked_alloc_bytes_immovable,
    gs_locked_alloc_struct,
    gs_locked_alloc_struct_immovable,
    gs_locked_alloc_byte_array,
    gs_locked_alloc_byte_array_immovable,
    gs_locked_alloc_struct_array,
    gs_locked_alloc_struct_array_immovable,
    gs_locked_resize_object,
    gs_locked_object_size,
    gs_locked_object_type,
    gs_locked_free_object,
    gs_locked_alloc_string,
    gs_locked_alloc_string_immovable,
    gs_locked_resize_string,
    gs_locked_free_string,
    gs_locked_register_root,
    gs_locked_unregister_root,
    gs_locked_status,
    gs_locked_enable_free
};

/* ---------- Public constructors/destructors ---------- */

/* Init some raw memory into a gs_memory_locked_t */
int				/* -ve error code or 0 */
gs_memory_locked_construct(
			      gs_memory_locked_t * lmem,	/* allocator to init */
			      gs_memory_t * target	/* allocator to monitor lock */
)
{
    /* Init the procedure vector from template */
    lmem->procs = locked_procs;

    lmem->target = target;

    /* Allocate a monitor to serialize access to structures within */
    lmem->monitor = gx_monitor_alloc(target);
    return (lmem->monitor) ? 0 : gs_error_VMerror;
}

/* Release resources held by a gs_memory_locked_t */
void
gs_memory_locked_destruct(
			     gs_memory_locked_t * lmem	/* allocator to dnit */
)
{
    gx_monitor_free(lmem->monitor);
    lmem->monitor = 0;
    lmem->target = 0;
}

/* ---------- Accessors ------------- */

/* Retrieve this allocator's target */
gs_memory_t *			/* returns target of this allocator */
gs_memory_locked_query_target(
				 gs_memory_locked_t * lmem	/* allocator to query */
)
{
    return lmem->target;
}

/* -------- Private members just wrap a monitor around a gs_memory_heap --- */

/* Allocate various kinds of blocks. */
private byte *
gs_locked_alloc_bytes(gs_memory_t * mem, uint size, client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    byte *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_bytes) (lmem->target, size, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private byte *
gs_locked_alloc_bytes_immovable(gs_memory_t * mem, uint size,
				client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    byte *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_bytes_immovable)
	(lmem->target, size, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private void *
gs_locked_alloc_struct(gs_memory_t * mem, gs_memory_type_ptr_t pstype,
		       client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    void *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_struct) (lmem->target, pstype, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private void *
gs_locked_alloc_struct_immovable(gs_memory_t * mem,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    void *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_struct_immovable)
	(lmem->target, pstype, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private byte *
gs_locked_alloc_byte_array(gs_memory_t * mem, uint num_elements, uint elt_size,
			   client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    byte *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_byte_array)
	(lmem->target, num_elements, elt_size, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private byte *
gs_locked_alloc_byte_array_immovable(gs_memory_t * mem, uint num_elements,
				     uint elt_size, client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    byte *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_byte_array_immovable)
	(lmem->target, num_elements, elt_size, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private void *
gs_locked_alloc_struct_array(gs_memory_t * mem, uint num_elements,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    void *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_struct_array)
	(lmem->target, num_elements, pstype, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private void *
gs_locked_alloc_struct_array_immovable(gs_memory_t * mem, uint num_elements,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    void *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_struct_array_immovable)
	(lmem->target, num_elements, pstype, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private void *
gs_locked_resize_object(gs_memory_t * mem, void *obj, uint new_num_elements,
			client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    void *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.resize_object) (lmem->target, obj,
						 new_num_elements, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private uint
gs_locked_object_size(gs_memory_t * mem, const void *ptr)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    uint temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.object_size) (lmem->target, ptr);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private gs_memory_type_ptr_t
gs_locked_object_type(gs_memory_t * mem, const void *ptr)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    gs_memory_type_ptr_t temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.object_type) (lmem->target, ptr);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private void
gs_locked_free_object(gs_memory_t * mem, void *ptr, client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;

    gx_monitor_enter(lmem->monitor);
    (*lmem->target->procs.free_object) (lmem->target, ptr, cname);
    gx_monitor_leave(lmem->monitor);
}
private byte *
gs_locked_alloc_string(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    byte *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_string) (lmem->target, nbytes, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private byte *
gs_locked_alloc_string_immovable(gs_memory_t * mem, uint nbytes,
				 client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    byte *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.alloc_string_immovable)
	(lmem->target, nbytes, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private byte *
gs_locked_resize_string(gs_memory_t * mem, byte * data, uint old_num,
			uint new_num,
			client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;
    byte *temp;

    gx_monitor_enter(lmem->monitor);
    temp = (*lmem->target->procs.resize_string) (lmem->target, data,
						 old_num, new_num, cname);
    gx_monitor_leave(lmem->monitor);
    return temp;
}
private void
gs_locked_free_string(gs_memory_t * mem, byte * data, uint nbytes,
		      client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;

    gx_monitor_enter(lmem->monitor);
    (*lmem->target->procs.free_string) (lmem->target, data, nbytes, cname);
    gx_monitor_leave(lmem->monitor);
}
private void
gs_locked_register_root(gs_memory_t * mem, gs_gc_root_t * rp, gs_ptr_type_t ptype,
			void **up, client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;

    gx_monitor_enter(lmem->monitor);
    (*lmem->target->procs.register_root) (lmem->target, rp, ptype, up, cname);
    gx_monitor_leave(lmem->monitor);
}
private void
gs_locked_unregister_root(gs_memory_t * mem, gs_gc_root_t * rp,
			  client_name_t cname)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;

    gx_monitor_enter(lmem->monitor);
    (*lmem->target->procs.unregister_root) (lmem->target, rp, cname);
    gx_monitor_leave(lmem->monitor);
}
private void
gs_locked_status(gs_memory_t * mem, gs_memory_status_t * pstat)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;

    gx_monitor_enter(lmem->monitor);
    (*lmem->target->procs.status) (lmem->target, pstat);
    gx_monitor_leave(lmem->monitor);
}
private void
gs_locked_enable_free(gs_memory_t * mem, bool enable)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *) mem;

    gx_monitor_enter(lmem->monitor);
    (*lmem->target->procs.enable_free) (lmem->target, enable);
    gx_monitor_leave(lmem->monitor);
}
