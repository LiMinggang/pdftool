/* Copyright (C) 2001-2018 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  1305 Grant Avenue - Suite 200, Novato,
   CA 94945, U.S.A., +1(415)492-9861, for further information.
*/

/* Top level PDF access routines */
#include "ghostpdf.h"
#include "pdf_types.h"
#include "pdf_dict.h"
#include "pdf_array.h"
#include "pdf_int.h"
#include "pdf_stack.h"
#include "pdf_file.h"
#include "stream.h"
#include "strmio.h"

static int pdf_process_page_contents(pdf_context *ctx, pdf_dict *page_dict)
{
    int i, code = 0;
    pdf_obj *o, *o1;

    code = pdf_dict_get(page_dict, "Contents", &o);
    if (code == gs_error_undefined)
        /* Don't throw an error if there are no contents, just render nothing.... */
        return 0;
    if (code < 0)
        return code;

    if (o->type == PDF_INDIRECT) {
        if (((pdf_indirect_ref *)o)->ref_object_num == page_dict->object_num)
            return_error(gs_error_circular_reference);

        code = pdf_dereference(ctx, ((pdf_indirect_ref *)o)->ref_object_num, ((pdf_indirect_ref *)o)->ref_generation_num, &o1);
        pdf_countdown(o);
        if (code < 0)
            return code;
        o = o1;
    }

    if (o->type == PDF_ARRAY) {
        pdf_array *a = (pdf_array *)o;

        for (i=0;i < a->size; i++) {
            pdf_indirect_ref *r;
            code = pdf_array_get(a, i, (pdf_obj **)&r);
            if (code < 0) {
                pdf_countdown(o);
                return code;
            }
            if (r->type == PDF_DICT) {
                code = pdf_interpret_content_stream(ctx, (pdf_dict *)r);
                pdf_countdown(r);
                if (code < 0) {
                    pdf_countdown(o);
                    return(code);
                }
            } else {
                if (r->type != PDF_INDIRECT) {
                    pdf_countdown(o);
                    pdf_countdown(r);
                    return_error(gs_error_typecheck);
                } else {
                    if (r->ref_object_num == page_dict->object_num) {
                        pdf_countdown(o);
                        pdf_countdown(r);
                        return_error(gs_error_circular_reference);
                    }
                    code = pdf_dereference(ctx, r->ref_object_num, r->ref_generation_num, &o1);
                    pdf_countdown(r);
                    if (code < 0) {
                        pdf_countdown(o);
                        if (code == gs_error_VMerror || ctx->pdfstoponerror)
                            return code;
                        else
                            return 0;
                    }
                    if (o1->type != PDF_DICT) {
                        pdf_countdown(o);
                        return_error(gs_error_typecheck);
                    }
                    code = pdf_interpret_content_stream(ctx, (pdf_dict *)o1);
                    pdf_countdown(o1);
                    if (code < 0) {
                        if (code == gs_error_VMerror || ctx->pdfstoponerror == true) {
                            pdf_countdown(o);
                            return code;
                        }
                    }
                }
            }
        }
    } else {
        if (o->type == PDF_DICT) {
            code = pdf_interpret_content_stream(ctx, (pdf_dict *)o);
        } else {
            pdf_countdown(o);
            return_error(gs_error_typecheck);
        }
    }
    pdf_countdown(o);
    return code;
}

static int pdf_check_page_transparency(pdf_context *ctx, pdf_dict *page_dict, bool *transparent)
{
    *transparent = false;
    return 0;
}

static int pdf_set_media_size(pdf_context *ctx, pdf_dict *page_dict)
{
    gs_c_param_list list;
    gs_param_float_array fa;
    pdf_array *a = NULL, *default_media = NULL;
    float fv[2];
    double d[4];
    int code;
    uint64_t i;

    gs_c_param_list_write(&list, ctx->memory);

    code = pdf_dict_get_type(ctx, page_dict, "MediaBox", PDF_ARRAY, (pdf_obj **)&default_media);
    if (code < 0)
        return 0;

    if (ctx->usecropbox) {
        if (a != NULL)
            pdf_countdown(a);
        (void)pdf_dict_get_type(ctx, page_dict, "CropBox", PDF_ARRAY, (pdf_obj **)&a);
    }
    if (ctx->useartbox) {
        if (a != NULL)
            pdf_countdown(a);
        (void)pdf_dict_get_type(ctx, page_dict, "ArtBox", PDF_ARRAY, (pdf_obj **)&a);
    }
    if (ctx->usebleedbox) {
        if (a != NULL)
            pdf_countdown(a);
        (void)pdf_dict_get_type(ctx, page_dict, "BBox", PDF_ARRAY, (pdf_obj **)&a);
    }
    if (ctx->usetrimbox) {
        if (a != NULL)
            pdf_countdown(a);
        (void)pdf_dict_get_type(ctx, page_dict, "MediaBox", PDF_ARRAY, (pdf_obj **)&a);
    }
    if (a == NULL)
        a = default_media;

    for (i=0;i<4;i++) {
        code = pdf_array_get_number(ctx, a, i, &d[i]);
    }

    normalize_rectangle(d);

    fv[0] = (float)(d[2] - d[0]);
    fv[1] = (float)(d[3] - d[1]);
    fa.persistent = false;
    fa.data = fv;
    fa.size = 2;

    code = param_write_float_array((gs_param_list *)&list, ".MediaSize", &fa);
    if (code >= 0)
    {
        gx_device *dev = gs_currentdevice(ctx->pgs);

        gs_c_param_list_read(&list);
        code = gs_putdeviceparams(dev, (gs_param_list *)&list);
        if (code < 0) {
            gs_c_param_list_release(&list);
            return code;
        }
    }
    gs_c_param_list_release(&list);
    return 0;
}

static int pdf_render_page(pdf_context *ctx, uint64_t page_num)
{
    int code;
    uint64_t page_offset = 0;
    pdf_dict *page_dict = NULL;
    bool uses_transparency;

    if (page_num > ctx->num_pages)
        return_error(gs_error_rangecheck);

    if (ctx->pdfdebug)
        dmprintf1(ctx->memory, "%% Processing Page %"PRIi64" content stream\n", page_num + 1);

    code = pdf_init_loop_detector(ctx);
    if (code < 0)
        return code;

    code = pdf_loop_detector_add_object(ctx, ctx->Pages->object_num);
    if (code < 0) {
        pdf_free_loop_detector(ctx);
        return code;
    }

    code = pdf_get_page_dict(ctx, ctx->Pages, page_num, &page_offset, &page_dict, NULL);
    pdf_free_loop_detector(ctx);
    if (code < 0) {
        if (code == gs_error_VMerror || ctx->pdfstoponerror)
            return code;
        return 0;
    }

    if (code > 0)
        return_error(gs_error_unknownerror);

    code = pdf_check_page_transparency(ctx, page_dict, &uses_transparency);
    if (code < 0) {
        pdf_countdown(page_dict);
        return code;
    }

    code = pdf_set_media_size(ctx, page_dict);
    if (code < 0) {
        pdf_countdown(page_dict);
        return code;
    }

    code = pdf_process_page_contents(ctx, page_dict);
    if (code < 0) {
        pdf_countdown(page_dict);
        return code;
    }

    pdf_countdown(page_dict);

    return pl_finish_page(ctx->memory->gs_lib_ctx->top_of_system,
                          ctx->pgs, 1, true);
}

/* These functions are used by the 'PL' implementation, eventually we will */
/* need to have custom PostScript operators to process the file or at      */
/* (least pages from it).                                                  */

int pdf_close_pdf_file(pdf_context *ctx)
{
    if (ctx->main_stream) {
        if (ctx->main_stream->s) {
            sfclose(ctx->main_stream->s);
            ctx->main_stream = NULL;
        }
        gs_free_object(ctx->memory, ctx->main_stream, "Closing main PDF file");
    }
    return 0;
}

int pdf_process_pdf_file(pdf_context *ctx, char *filename)
{
    int code = 0, i;
    pdf_obj *o;

    ctx->filename = (char *)gs_alloc_bytes(ctx->memory, strlen(filename) + 1, "copy of filename");
    if (ctx->filename == NULL)
        return_error(gs_error_VMerror);
    strcpy(ctx->filename, filename);

    code = pdf_open_pdf_file(ctx, filename);
    if (code < 0) {
        return code;
    }

    code = pdf_read_xref(ctx);
    if (code < 0) {
        if (ctx->is_hybrid) {
            /* If its a hybrid file, and we failed to read the XrefStm, try
             * again, but this time read the xref table instead.
             */
            ctx->pdf_errors |= E_PDF_BADXREFSTREAM;
            pdf_countdown(ctx->xref_table);
            ctx->xref_table = NULL;
            ctx->prefer_xrefstm = false;
            code = pdf_read_xref(ctx);
            if (code < 0)
                return code;
        } else {
            ctx->pdf_errors |= E_PDF_BADXREF;
            return code;
        }
    }

    if (ctx->Trailer) {
        code = pdf_dict_get(ctx->Trailer, "Encrypt", &o);
        if (code < 0 && code != gs_error_undefined)
            return code;
        if (code == 0) {
            dmprintf(ctx->memory, "Encrypted PDF files not yet supported.\n");
            return 0;
        }
    }

read_root:
    if (ctx->Trailer) {
        code = pdf_read_Root(ctx);
        if (code < 0) {
            /* If we couldn#'t find the Root object, and we were using the XrefStm
             * from a hybrid file, then try again, but this time use the xref table
             */
            if (code == gs_error_undefined && ctx->is_hybrid && ctx->prefer_xrefstm) {
                ctx->pdf_errors |= E_PDF_BADXREFSTREAM;
                pdf_countdown(ctx->xref_table);
                ctx->xref_table = NULL;
                ctx->prefer_xrefstm = false;
                code = pdf_read_xref(ctx);
                if (code < 0) {
                    ctx->pdf_errors |= E_PDF_BADXREF;
                    return code;
                }
                code = pdf_read_Root(ctx);
                if (code < 0)
                    return code;
            } else
                if (!ctx->repaired) {
                    code = pdf_repair_file(ctx);
                    if (code < 0)
                        return code;
                    goto read_root;
                }
                return code;
        }
    }

    if (ctx->Trailer) {
        code = pdf_read_Info(ctx);
        if (code < 0 && code != gs_error_undefined) {
            if (ctx->pdfstoponerror)
                return code;
            pdf_clearstack(ctx);
        }
    }

    if (!ctx->Root) {
        dmprintf(ctx->memory, "Catalog dictionary not located in file, unable to proceed\n");
        return_error(gs_error_syntaxerror);
    }

    code = pdf_read_Pages(ctx);
    if (code < 0)
        return code;

    for (i=0;i < ctx->num_pages;i++) {
        code = pdf_render_page(ctx, i);
        if (code < 0)
            return code;
    }

    code = sfclose(ctx->main_stream->s);
    ctx->main_stream = NULL;
    return code;
}

static void cleanup_pdf_open_file(pdf_context *ctx, byte *Buffer)
{
    if (Buffer != NULL)
        gs_free_object(ctx->memory, Buffer, "PDF interpreter - cleanup working buffer for file validation");

    if (ctx->main_stream != NULL) {
        sfclose(ctx->main_stream->s);
        ctx->main_stream = NULL;
    }
    ctx->main_stream_length = 0;
}

int pdf_open_pdf_file(pdf_context *ctx, char *filename)
{
    byte *Buffer = NULL;
    char *s = NULL;
    float version = 0.0;
    gs_offset_t Offset = 0;
    int64_t bytes = 0;
    bool found = false;

//    if (ctx->pdfdebug)
        dmprintf1(ctx->memory, "%% Attempting to open %s as a PDF file\n", filename);

    ctx->main_stream = (pdf_stream *)gs_alloc_bytes(ctx->memory, sizeof(pdf_stream), "PDF interpreter allocate main PDF stream");
    if (ctx->main_stream == NULL)
        return_error(gs_error_VMerror);
    memset(ctx->main_stream, 0x00, sizeof(pdf_stream));

    ctx->main_stream->s = sfopen(filename, "r", ctx->memory);
    if (ctx->main_stream == NULL) {
        emprintf1(ctx->memory, "Failed to open file %s\n", filename);
        return_error(gs_error_ioerror);
    }

    Buffer = gs_alloc_bytes(ctx->memory, BUF_SIZE, "PDF interpreter - allocate working buffer for file validation");
    if (Buffer == NULL) {
        cleanup_pdf_open_file(ctx, Buffer);
        return_error(gs_error_VMerror);
    }

    /* Determine file size */
    pdf_seek(ctx, ctx->main_stream, 0, SEEK_END);
    ctx->main_stream_length = stell(ctx->main_stream->s);
    Offset = BUF_SIZE;
    bytes = BUF_SIZE;
    pdf_seek(ctx, ctx->main_stream, 0, SEEK_SET);

    bytes = Offset = min(BUF_SIZE, ctx->main_stream_length);

    if (ctx->pdfdebug)
        dmprintf(ctx->memory, "%% Reading header\n");

    bytes = pdf_read_bytes(ctx, Buffer, 1, Offset, ctx->main_stream);
    if (bytes <= 0) {
        emprintf(ctx->memory, "Failed to read any bytes from file\n");
        cleanup_pdf_open_file(ctx, Buffer);
        return_error(gs_error_ioerror);
    }

    /* First check for existence of header */
    s = strstr((char *)Buffer, "%PDF");
    if (s == NULL) {
        if (ctx->pdfdebug)
            dmprintf1(ctx->memory, "%% File %s does not appear to be a PDF file (no %%PDF in first 2Kb of file)\n", filename);
        ctx->pdf_errors |= E_PDF_NOHEADER;
    } else {
        /* Now extract header version (may be overridden later) */
        if (sscanf(s + 5, "%f", &version) != 1) {
            if (ctx->pdfdebug)
                dmprintf(ctx->memory, "%% Unable to read PDF version from header\n");
            ctx->HeaderVersion = 0;
            ctx->pdf_errors |= E_PDF_NOHEADERVERSION;
        }
        else {
            ctx->HeaderVersion = version;
        }
        if (ctx->pdfdebug)
            dmprintf1(ctx->memory, "%% Found header, PDF version is %f\n", ctx->HeaderVersion);
    }

    /* Jump to EOF and scan backwards looking for startxref */
    pdf_seek(ctx, ctx->main_stream, 0, SEEK_END);

    if (ctx->pdfdebug)
        dmprintf(ctx->memory, "%% Searching for 'startxerf' keyword\n");

    bytes = Offset;
    do {
        byte *last_lineend = NULL;
        uint32_t read;

        if (pdf_seek(ctx, ctx->main_stream, ctx->main_stream_length - Offset, SEEK_SET) != 0) {
            emprintf1(ctx->memory, "File is smaller than %"PRIi64" bytes\n", (int64_t)Offset);
            cleanup_pdf_open_file(ctx, Buffer);
            return_error(gs_error_ioerror);
        }
        read = pdf_read_bytes(ctx, Buffer, 1, bytes, ctx->main_stream);

        if (read <= 0) {
            emprintf1(ctx->memory, "Failed to read %"PRIi64" bytes from file\n", (int64_t)bytes);
            cleanup_pdf_open_file(ctx, Buffer);
            return_error(gs_error_ioerror);
        }

        read = bytes = read + (BUF_SIZE - bytes);

        while(read) {
            if (memcmp(Buffer + read - 9, "startxref", 9) == 0) {
                found = true;
                break;
            } else {
                if (Buffer[read] == 0x0a || Buffer[read] == 0x0d)
                    last_lineend = Buffer + read;
            }
            read--;
        }
        if (found) {
            byte *b = Buffer + read;

            if(sscanf((char *)b, " %ld", &ctx->startxref) != 1) {
                dmprintf(ctx->memory, "Unable to read offset of xref from PDF file\n");
            }
            break;
        } else {
            if (last_lineend) {
                uint32_t len = last_lineend - Buffer;
                memcpy(Buffer + bytes - len, last_lineend, len);
                bytes -= len;
            }
        }

        Offset += bytes;
    } while(Offset < ctx->main_stream_length);

    if (!found)
        ctx->pdf_errors |= E_PDF_NOSTARTXREF;

    gs_free_object(ctx->memory, Buffer, "PDF interpreter - allocate working buffer for file validation");
    return 0;
}

/***********************************************************************************/
/* Highest level functions. The context we create here is returned to the 'PL'     */
/* implementation, in future we plan to return it to PostScript by wrapping a      */
/* gargabe collected object 'ref' around it and returning that to the PostScript   */
/* world. custom PostScript operators will then be able to render pages, annots,   */
/* AcroForms etc by passing the opaque object back to functions here, allowing     */
/* the interpreter access to its context.                                          */

/* We start with routines for creating and destroying the interpreter context */
pdf_context *pdf_create_context(gs_memory_t *pmem)
{
    pdf_context *ctx = NULL;
    gs_gstate *pgs = NULL;
    int code = 0;

    ctx = (pdf_context *) gs_alloc_bytes(pmem->non_gc_memory,
            sizeof(pdf_context), "pdf_imp_allocate_interp_instance");

    pgs = gs_gstate_alloc(pmem);

    if (!ctx || !pgs)
    {
        if (ctx)
            gs_free_object(pmem->non_gc_memory, ctx, "pdf_imp_allocate_interp_instance");
        if (pgs)
            gs_gstate_free(pgs);
        return NULL;
    }

    memset(ctx, 0, sizeof(pdf_context));

    ctx->stack_bot = (pdf_obj **)gs_alloc_bytes(pmem->non_gc_memory, INITIAL_STACK_SIZE * sizeof (pdf_obj *), "pdf_imp_allocate_interp_stack");
    if (ctx->stack_bot == NULL) {
        gs_free_object(pmem->non_gc_memory, ctx, "pdf_imp_allocate_interp_instance");
        gs_gstate_free(pgs);
        return NULL;
    }
    ctx->stack_size = INITIAL_STACK_SIZE;
    ctx->stack_top = ctx->stack_bot - sizeof(pdf_obj *);
    code = sizeof(pdf_obj *);
    code *= ctx->stack_size;
    ctx->stack_limit = ctx->stack_bot + ctx->stack_size;

    code = gsicc_init_iccmanager(pgs);
    if (code < 0) {
        gs_free_object(pmem->non_gc_memory, ctx->stack_bot, "pdf_imp_allocate_interp_instance");
        gs_free_object(pmem->non_gc_memory, ctx, "pdf_imp_allocate_interp_instance");
        gs_gstate_free(pgs);
        return NULL;
    }

    ctx->memory = pmem->non_gc_memory;
    ctx->pgs = pgs;
    /* Declare PDL client support for high level patterns, for the benefit
     * of pdfwrite and other high-level devices
     */
    ctx->pgs->have_pattern_streams = true;
    ctx->fontdir = NULL;
    ctx->preserve_tr_mode = 0;

    ctx->main_stream = NULL;

    /* Gray, RGB and CMYK profiles set when color spaces installed in graphics lib */
    ctx->gray_lin = gs_cspace_new_ICC(ctx->memory, ctx->pgs, -1);
    ctx->gray = gs_cspace_new_ICC(ctx->memory, ctx->pgs, 1);
    ctx->cmyk = gs_cspace_new_ICC(ctx->memory, ctx->pgs, 4);
    ctx->srgb = gs_cspace_new_ICC(ctx->memory, ctx->pgs, 3);
    ctx->scrgb = gs_cspace_new_ICC(ctx->memory, ctx->pgs, 3);

    /* Initially, prefer the XrefStm in a hybrid file */
    ctx->prefer_xrefstm = true;

#if REFCNT_DEBUG
    ctx->UID = 1;
    ctx->freed_objects = 0;
#endif
    return ctx;
}

int pdf_free_context(gs_memory_t *pmem, pdf_context *ctx)
{
    if (ctx->cache_entries != 0) {
        pdf_obj_cache_entry *entry = ctx->cache_LRU, *next;

        while(entry) {
            next = entry->next;
            pdf_countdown(entry->o);
            ctx->cache_entries--;
            gs_free_object(ctx->memory, entry, "pdf_add_to_cache, free LRU");
            entry = next;
        }
        ctx->cache_LRU = ctx->cache_MRU = NULL;
        ctx->cache_entries = 0;
    }

    if (ctx->PDFPassword)
        gs_free_object(ctx->memory, ctx->PDFPassword, "pdf_free_context");

    if (ctx->PageList)
        gs_free_object(ctx->memory, ctx->PageList, "pdf_free_context");

    if (ctx->Trailer)
        pdf_countdown(ctx->Trailer);

    if(ctx->Root)
        pdf_countdown(ctx->Root);

    if (ctx->Info)
        pdf_countdown(ctx->Info);

    if (ctx->Pages)
        pdf_countdown(ctx->Pages);

    if (ctx->xref_table) {
        pdf_countdown(ctx->xref_table);
        ctx->xref_table = NULL;
    }

    if (ctx->stack_bot) {
        pdf_clearstack(ctx);
        gs_free_object(ctx->memory, ctx->stack_bot, "pdf_free_context");
    }

    if (ctx->filename) {
        gs_free_object(ctx->memory, ctx->filename, "free copy of filename");
        ctx->filename = NULL;
    }

    if (ctx->main_stream != NULL) {
        sfclose(ctx->main_stream->s);
        ctx->main_stream = NULL;
    }
    if(ctx->pgs != NULL) {
        gs_gstate_free(ctx->pgs);
        ctx->pgs = NULL;
    }

    gs_free_object(ctx->memory, ctx, "pdf_free_context");
    return 0;
}
