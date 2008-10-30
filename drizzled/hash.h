/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2008 Sun Microsystems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* Dynamic hashing of record with different key-length */

#ifndef DRIZZLED_HASH_H
#define DRIZZLED_HASH_H

#include <mysys/my_sys.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  Overhead to store an element in hash
  Can be used to approximate memory consumption for a hash
 */
#define HASH_OVERHEAD (sizeof(char*)*2)

/* flags for hash_init */
#define HASH_UNIQUE     1       /* hash_insert fails on duplicate key */

typedef unsigned char *(*hash_get_key)(const unsigned char *,size_t*,bool);
typedef void (*hash_free_key)(void *);

typedef struct st_hash {
  /* Length of key if const length */
  size_t key_offset,key_length;
  size_t blength;
  uint32_t records;
  uint32_t flags;
  /* Place for hash_keys */
  DYNAMIC_ARRAY array;
  hash_get_key get_key;
  void (*free)(void *);
  const CHARSET_INFO *charset;
} HASH;

/* A search iterator state */
typedef uint32_t HASH_SEARCH_STATE;

#define hash_init(A,B,C,D,E,F,G,H) _hash_init(A,0,B,C,D,E,F,G,H CALLER_INFO)
#define hash_init2(A,B,C,D,E,F,G,H,I) _hash_init(A,B,C,D,E,F,G,H,I CALLER_INFO)
bool _hash_init(HASH *hash, uint32_t growth_size,
                const CHARSET_INFO * const charset,
                uint32_t default_array_elements, size_t key_offset,
                size_t key_length, hash_get_key get_key,
                void (*free_element)(void*), uint32_t flags CALLER_INFO_PROTO);
void hash_free(HASH *tree);
void my_hash_reset(HASH *hash);
unsigned char *hash_element(HASH *hash,uint32_t idx);
unsigned char *hash_search(const HASH *info, const unsigned char *key,
                           size_t length);
unsigned char *hash_first(const HASH *info, const unsigned char *key,
                          size_t length, HASH_SEARCH_STATE *state);
unsigned char *hash_next(const HASH *info, const unsigned char *key,
                         size_t length, HASH_SEARCH_STATE *state);
bool my_hash_insert(HASH *info,const unsigned char *data);
bool hash_delete(HASH *hash,unsigned char *record);
bool hash_update(HASH *hash,unsigned char *record, unsigned char *old_key,
                 size_t old_key_length);
void hash_replace(HASH *hash, HASH_SEARCH_STATE *state, unsigned char *new_row);

#define hash_clear(H) memset((H), 0, sizeof(*(H)))
#define hash_inited(H) ((H)->array.buffer != 0)
#define hash_init_opt(A,B,C,D,E,F,G,H)                                  \
  (!hash_inited(A) && _hash_init(A,0,B,C,D,E,F,G, H CALLER_INFO))

#ifdef __cplusplus
}
#endif
#endif
