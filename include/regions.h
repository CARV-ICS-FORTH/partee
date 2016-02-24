/*
 * Copyright (c) 1999-2001
 *      The Regents of the University of California.  All rights reserved.
 * Edits for Locksmith Copyright(c) 2006-2010
 *      University of Maryland.  All rights reserved.
 * Edits for 64bit port: Copyright (c) 2010
 *      FORTH-ICS. All rights reserved.
 * Edits for ENCORE: Copyright(c) 2010
 *      FORTH-ICS.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#ifndef REGIONS_H
#define REGIONS_H

#include "linkage.h"
#include <inttypes.h>

EXTERN_C_BEGIN
/* In a perfect world, this would be the definition of RMAXMEMLOG. */

#if defined (__ia64__) || defined (__x86_64__)
#define INT_PTR    unsigned long
#define RMAXMEMLOG ( 8 * sizeof(void*) )
#define RMAPLOG    26
#define RPAGELOG   14
#else /* if defined (__ia64__) || defined (__x86_64__) */
#define INT_PTR    unsigned int
#define RMAXMEMLOG 36
#define RMAPLOG    10
#define RPAGELOG   14
#endif /* if defined (__ia64__) || defined (__x86_64__) */

typedef struct region_ *region_t;

extern volatile region_t permanent;

#include <stdlib.h>

void region_t_init(void);

region_t __newregion(void);
region_t newsubregion(region_t parent);

typedef int type_t;

#define rctypeof(type) 0

/* Low-level alloc with dynamic type info */
void* __rc_typed_ralloc(region_t r, size_t size, type_t type);
void* __rc_typed_rarrayalloc(region_t r, size_t n, size_t size, type_t type);
void* __rc_typed_rarrayextend(region_t r, void *old, size_t n, size_t size,
                              type_t type);

void typed_rarraycopy(void *to, void *from, size_t n, size_t size, type_t type);

void* __rc_ralloc_small0(region_t r, size_t size);

/* In theory, the test at the start of qalloc should give the same benefit.
 *  In practice, it doesn't (gcc, at least, generates better code for
 *  __rcralloc_small0 than the equivalent path through typed_ralloc */
#define ralloc(r,                                                                \
	       type) ( sizeof(type) <                                            \
	               ( 1 <<                                                    \
	                 (RPAGELOG - 3) ) ? __rcralloc_small0( (r),              \
	                                                       sizeof(           \
	                                                               type) ) : \
	               typed_ralloc( (r), sizeof(type), rctypeof(type) ) )
#define rarrayalloc(r, n, type)                                           \
	             typed_rarrayalloc( (r), (n), sizeof(type), rctypeof( \
	                           type) )
#define rarrayextend(r, old, n, type)                     \
	             typed_rarrayextend( (r), (old), (n), \
	                    sizeof(type),                 \
	                    rctypeof(type) )
#define rarraycopy(to, from, n, type)                     \
	             typed_rarraycopy( (to), (from), (n), \
	                  sizeof(type),                   \
	                  rctypeof(type) )

char* __rc_rstralloc(region_t r, size_t size);
char* __rc_rstralloc0(region_t r, size_t size);
char* __rc_rstrdup(region_t r, const char *s);

/* rstrextend is used to extend an old string. The string MUST have been
 *  initially allocated by a call to rstrextend with old == NULL (you cannot
 *  initially allocate the string with rstralloc) */
char* __rc_rstrextend(region_t r, const char *old, size_t newsize);
char* __rc_rstrextend0(region_t r, const char *old, size_t newsize);

void     __deleteregion(region_t r);
void     __deleteregion_ptr(region_t *r);
void     __deleteregion_array(int n, region_t *regions);
region_t regionof(void *ptr);

typedef void (*nomem_handler)(void);

nomem_handler set_nomem_handler(nomem_handler newhandler);

/* Debugging support */
void findrefs(region_t r, void *from, void *to);
void findgrefs(region_t r);
void findrrefs(region_t r, region_t from);

#ifdef DEBUG_RALLOC

extern int  __rc_line;
extern char *__rc_file;

#define RCDEBUG (__rc_line = __LINE__, __rc_file = __FILE__)
#else /* ifdef DEBUG_RALLOC */
#define RCDEBUG ( (void)0 )
#endif /* ifdef DEBUG_RALLOC */

#ifndef REGION_PROFILE
#define typed_ralloc       (RCDEBUG, __rc_typed_ralloc)
#define typed_rarrayalloc  (RCDEBUG, __rc_typed_rarrayalloc)
#define typed_rarrayextend (RCDEBUG, __rc_typed_rarrayextend)
#define rstralloc          (RCDEBUG, __rc_rstralloc)
#define rstralloc0         (RCDEBUG, __rc_rstralloc0)
#define rstrdup            (RCDEBUG, __rc_rstrdup)
#define rstrextend         (RCDEBUG, __rc_rstrextend)
#define rstrextend0        (RCDEBUG, __rc_rstrextend0)
#define __rcralloc_small0  (RCDEBUG, __rc_ralloc_small0)
#define deleteregion(r)                __deleteregion(r)
#define deleteregion_ptr(r)            __deleteregion_ptr(r)
#define deleteregion_array(n, regions) __deleteregion_array(n, regions)
#define newregion()                    __newregion()
#else /* ifndef REGION_PROFILE */
#include "profile.h"
#endif /* ifndef REGION_PROFILE */

struct translation_ {
	region_t reg;
	void     **map;
};

/*
 *  The interface for serialization/deserialization.
 */
typedef struct translation_ *translation;
typedef int (*Updater)(translation, void*);

void delete_translation(translation);

extern int         serialize(region_t *r, char *datafile, char *statefile);
extern translation deserialize(char*, char*, Updater*, region_t);
extern void        update_pointer(translation, void**);
extern void        * translate_pointer(translation, void*);

void region_init(void);

#define TRANSLATEPOINTER(m,                                              \
	                 a)     ( ( *( m->map +                          \
	                               ( ( (INT_PTR)a ) >> SHIFT ) ) ) + \
	                          ( ( (INT_PTR)a ) &                     \
	                            0x00001FFF ) )
#define UPDATEPOINTER(map, loc) *(loc) = TRANSLATEPOINTER(map, loc)

EXTERN_C_END

#endif /* ifndef REGIONS_H */
