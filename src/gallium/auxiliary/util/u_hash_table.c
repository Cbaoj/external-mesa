/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * General purpose hash table implementation.
 * 
 * Just uses the cso_hash for now, but it might be better switch to a linear 
 * probing hash table implementation at some point -- as it is said they have 
 * better lookup and cache performance and it appears to be possible to write 
 * a lock-free implementation of such hash tables . 
 * 
 * @author José Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "pipe/p_compiler.h"
#include "pipe/p_debug.h"
#include "pipe/p_util.h"

#include "cso_cache/cso_hash.h"
#include "u_hash_table.h"


struct hash_table
{
   struct cso_hash *cso;   
   
   /** Hash function */
   unsigned (*hash)(void *key);
   
   /** Compare two keys */
   int (*compare)(void *key1, void *key2);
   
   /* TODO: key, value destructors? */
};


struct hash_table_item
{
   void *key;
   void *value;
};


struct hash_table *
hash_table_create(unsigned (*hash)(void *key),
                  int (*compare)(void *key1, void *key2))
{
   struct hash_table *ht;
   
   ht = MALLOC_STRUCT(hash_table);
   if(!ht)
      return NULL;
   
   ht->cso = cso_hash_create();
   if(!ht->cso) {
      FREE(ht);
      return NULL;
   }
   
   ht->hash = hash;
   ht->compare = compare;
   
   return ht;
}


static struct hash_table_item *
hash_table_find_item(struct hash_table *ht,
                     void *key, 
                     unsigned key_hash)
{
   struct cso_hash_iter iter;
   struct hash_table_item *item;
   
   iter = cso_hash_find(ht->cso, key_hash);
   while (!cso_hash_iter_is_null(iter)) {
      item = (struct hash_table_item *)cso_hash_iter_data(iter);
      if (!ht->compare(item->key, key))
         return item;
      iter = cso_hash_iter_next(iter);
   }
   
   return NULL;
}


enum pipe_error
hash_table_set(struct hash_table *ht,
               void *key,
               void *value)
{
   unsigned key_hash;
   struct hash_table_item *item;

   assert(ht);

   key_hash = ht->hash(key);

   item = hash_table_find_item(ht, key, key_hash);
   if(item) {
      /* TODO: key/value destruction? */
      item->value = value;
      return PIPE_OK;
   }
   
   item = MALLOC_STRUCT(hash_table_item);
   if(!item)
      return PIPE_ERROR_OUT_OF_MEMORY;
   
   item->key = key;
   item->value = value;
   
   cso_hash_insert(ht->cso, key_hash, item);
   /* FIXME: there is no OOM propagation in cso_hash */
   if(0) {
      FREE(item);
      return PIPE_ERROR_OUT_OF_MEMORY;
   }

   return PIPE_OK;
}


void *
hash_table_get(struct hash_table *ht, 
               void *key)
{
   unsigned key_hash;
   struct hash_table_item *item;

   assert(ht);

   key_hash = ht->hash(key);

   item = hash_table_find_item(ht, key, key_hash);
   if(!item)
      return NULL;
   
   return item->value;
}


void
hash_table_remove(struct hash_table *ht, 
                  void *key)
{
   unsigned key_hash;
   struct hash_table_item *item;

   assert(ht);

   key_hash = ht->hash(key);

   item = hash_table_find_item(ht, key, key_hash);
   if(!item)
      return;
   
   /* FIXME: cso_hash_take takes the first element of the collision list 
    * indiscriminately, so we can not take the item down. */
   item->value = NULL;
}


void
hash_table_destroy(struct hash_table *ht)
{
   assert(ht);

   cso_hash_delete(ht->cso);
   
   FREE(ht);
}

