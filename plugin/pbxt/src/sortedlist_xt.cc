/* Copyright (c) 2005 PrimeBase Technologies GmbH
 *
 * PrimeBase XT
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * 2005-02-04	Paul McCullagh
 *
 * H&G2JCtL
 */

#include "xt_config.h"

#include "pthread_xt.h"
#include "thread_xt.h"
#include "sortedlist_xt.h"

XTSortedListPtr xt_new_sortedlist_ns(u_int item_size, u_int grow_size, XTCompareFunc comp_func, void *thunk, XTFreeFunc free_func)
{
	XTSortedListPtr sl;

	if (!(sl = (XTSortedListPtr) xt_calloc_ns(sizeof(XTSortedListRec))))
		return NULL;
	sl->sl_item_size = item_size;
	sl->sl_grow_size = grow_size;
	sl->sl_comp_func = comp_func;
	sl->sl_thunk = thunk;
	sl->sl_free_func = free_func;
	sl->sl_current_size = 0;
	return sl;
}

XTSortedListPtr xt_new_sortedlist(XTThreadPtr self, u_int item_size, u_int initial_size, u_int grow_size, XTCompareFunc comp_func, void *thunk, XTFreeFunc free_func, xtBool with_lock, xtBool with_cond)
{
	XTSortedListPtr sl;

	sl = (XTSortedListPtr) xt_calloc(self, sizeof(XTSortedListRec));
	xt_init_sortedlist(self, sl, item_size, initial_size, grow_size, comp_func, thunk, free_func, with_lock, with_cond);
	return sl;
}

xtPublic void xt_init_sortedlist(XTThreadPtr self, XTSortedListPtr sl, u_int item_size, u_int initial_size, u_int grow_size, XTCompareFunc comp_func, void *thunk, XTFreeFunc free_func, xtBool with_lock, xtBool with_cond)
{
	sl->sl_item_size = item_size;
	sl->sl_grow_size = grow_size;
	sl->sl_comp_func = comp_func;
	sl->sl_thunk = thunk;
	sl->sl_free_func = free_func;
	sl->sl_current_size = initial_size;

	if (initial_size) {
		try_(a) {
			sl->sl_data = (char *) xt_malloc(self, initial_size * item_size);
		}
		catch_(a) {
			xt_free(self, sl);
			throw_();
		}
		cont_(a);
	}

	if (with_lock || with_cond) {
		sl->sl_lock = (xt_mutex_type *) xt_calloc(self, sizeof(xt_mutex_type));
		try_(b) {
			xt_init_mutex_with_autoname(self, sl->sl_lock);
		}
		catch_(b) {
			xt_free(self, sl->sl_lock);
			sl->sl_lock = NULL;
			xt_free_sortedlist(self, sl);
			throw_();
		}
		cont_(b);
	}

	if (with_cond) {
		sl->sl_cond = (xt_cond_type *) xt_calloc(self, sizeof(xt_cond_type));
		try_(c) {
			xt_init_cond(self, sl->sl_cond);
		}
		catch_(c) {
			xt_free(self, sl->sl_cond);
			sl->sl_cond = NULL;
			xt_free_sortedlist(self, sl);
			throw_();
		}
		cont_(c);
	}
}

xtPublic xtBool xt_init_sortedlist_ns(XTSortedListPtr sl, u_int item_size, u_int initial_size, u_int grow_size, XTCompareFunc comp_func, void *thunk, XTFreeFunc free_func, xtBool with_lock, xtBool with_cond)
{
	memset(sl, 0, sizeof(XTSortedListRec));
	sl->sl_item_size = item_size;
	sl->sl_grow_size = grow_size;
	sl->sl_comp_func = comp_func;
	sl->sl_thunk = thunk;
	sl->sl_free_func = free_func;
	sl->sl_current_size = initial_size;

	if (initial_size) {
		if (!(sl->sl_data = (char *) xt_malloc_ns(initial_size * item_size)))
			goto failed;
	}

	if (with_lock || with_cond) {
		if (!(sl->sl_lock = (xt_mutex_type *) xt_calloc_ns(sizeof(xt_mutex_type))))
			goto failed;
		if (!xt_init_mutex_with_autoname(NULL, sl->sl_lock))
			goto failed;
	}

	if (with_cond) {
		if (!(sl->sl_cond = (xt_cond_type *) xt_calloc_ns(sizeof(xt_cond_type))))
			goto failed;
		if (!xt_init_cond(NULL, sl->sl_cond))
			goto failed;
	}

	return OK;

	failed:
	xt_free_sortedlist(NULL, sl);
	return FAILED;
}

xtPublic void xt_empty_sortedlist(XTThreadPtr self, XTSortedListPtr sl)
{
	if (sl->sl_lock)
		xt_lock_mutex(self, sl->sl_lock);
	if (sl->sl_data) {
		if (sl->sl_free_func) {
			while (sl->sl_usage_count > 0) {
				sl->sl_usage_count--;
				if (sl->sl_free_func)
					(*sl->sl_free_func)(self, sl->sl_thunk, &sl->sl_data[sl->sl_usage_count * sl->sl_item_size]);
			}
		}
		else
			sl->sl_usage_count = 0;
	}
	if (sl->sl_lock)
		xt_unlock_mutex(self, sl->sl_lock);
}

xtPublic void xt_clear_sortedlist(XTSortedListPtr sl)
{
	if (sl->sl_data) {
		xt_free_ns(sl->sl_data);
		sl->sl_data = NULL;
	}
	if (sl->sl_lock) {
		xt_free_mutex(sl->sl_lock);
		xt_free_ns(sl->sl_lock);
	}
	if (sl->sl_cond) {
		xt_free_cond(sl->sl_cond);
		xt_free_ns(sl->sl_cond);
	}
}

xtPublic void xt_free_sortedlist(XTThreadPtr self, XTSortedListPtr sl)
{
	xt_empty_sortedlist(self, sl);
	xt_clear_sortedlist(sl);
	xt_free(self, sl);
}

xtPublic void *xt_sl_find(XTThreadPtr self, XTSortedListPtr sl, void *key)
{
	void	*result;
	size_t	idx;

	if (sl->sl_usage_count == 0)
		return NULL;
	else if (sl->sl_usage_count == 1) {
		if ((*sl->sl_comp_func)(self, sl->sl_thunk, key, sl->sl_data) == 0)
			return sl->sl_data;
		return NULL;
	}
	result = xt_bsearch(self, key, sl->sl_data, sl->sl_usage_count, sl->sl_item_size, &idx, sl->sl_thunk, sl->sl_comp_func);
	return result;
}

/* This function is the same as find, but it also returns the
 * index position for the item.
 *
 * If the item is not in the list, then it returns the index at which the item
 * should be inserted.
 */
xtPublic void *xt_sl_search(XTThreadPtr self, size_t *idx, XTSortedListPtr sl, void *key)
{
	if (sl->sl_usage_count == 0) {
		*idx = 0;
		return NULL;
	}
	else if (sl->sl_usage_count == 1) {
		int r;

		if ((r = (*sl->sl_comp_func)(self, sl->sl_thunk, key, sl->sl_data)) == 0) {
			*idx = 0;
			return sl->sl_data;
		}
		if (r > 0)
			*idx = 1;
		else
			*idx = 0;
		return NULL;
	}
	return xt_bsearch(self, key, sl->sl_data, sl->sl_usage_count, sl->sl_item_size, idx, sl->sl_thunk, sl->sl_comp_func);
}

/*
 * Returns:
 * 1 = Value inserted.
 * 2 = Value not inserted, already in the list (returned value is index + 2).
 * 0 = An error occurred.
 */
xtPublic int xt_sl_insert(XTThreadPtr self, XTSortedListPtr sl, void *key, void *data)
{
	size_t idx;

	if (sl->sl_usage_count == 0)
		idx = 0;
	else if (sl->sl_usage_count == 1) {
		int r;

		if ((r = (*sl->sl_comp_func)(self, sl->sl_thunk, key, sl->sl_data)) == 0) {
			if (sl->sl_free_func)
				(*sl->sl_free_func)(self, sl->sl_thunk, data);
			return 2;
		}
		if (r < 0)
			idx = 0;
		else
			idx = 1;
	}
	else {
		if (xt_bsearch(self, key, sl->sl_data, sl->sl_usage_count, sl->sl_item_size, &idx, sl->sl_thunk, sl->sl_comp_func)) {
			if (sl->sl_free_func)
				(*sl->sl_free_func)(self, sl->sl_thunk, data);
			return 2;
		}
	}
	if (sl->sl_usage_count == sl->sl_current_size) {		
		if (!xt_realloc_ns((void **) &sl->sl_data, (sl->sl_current_size + sl->sl_grow_size) * sl->sl_item_size)) {
			if (sl->sl_free_func)
				(*sl->sl_free_func)(self, sl->sl_thunk, data);
			if (self)
				xt_throw(self);
			return 0;
		}
		sl->sl_current_size = sl->sl_current_size + sl->sl_grow_size;
	}
	XT_MEMMOVE(sl->sl_data, &sl->sl_data[(idx+1) * sl->sl_item_size], &sl->sl_data[idx * sl->sl_item_size], (sl->sl_usage_count-idx) * sl->sl_item_size);
	XT_MEMCPY(sl->sl_data, &sl->sl_data[idx * sl->sl_item_size], data, sl->sl_item_size);
	sl->sl_usage_count++;
	return 1;
}

xtPublic xtBool xt_sl_insert_item_at(XTThreadPtr self, XTSortedListPtr sl, size_t idx, void *data)
{
	ASSERT(idx <= sl->sl_usage_count);
	if (sl->sl_usage_count == sl->sl_current_size) {		
		if (!xt_realloc_ns((void **) &sl->sl_data, (sl->sl_current_size + sl->sl_grow_size) * sl->sl_item_size)) {
			if (sl->sl_free_func)
				(*sl->sl_free_func)(self, sl->sl_thunk, data);
			if (self)
				xt_throw(self);
			return FAILED;
		}
		sl->sl_current_size = sl->sl_current_size + sl->sl_grow_size;
	}
	XT_MEMMOVE(sl->sl_data, &sl->sl_data[(idx+1) * sl->sl_item_size], &sl->sl_data[idx * sl->sl_item_size], (sl->sl_usage_count-idx) * sl->sl_item_size);
	XT_MEMCPY(sl->sl_data, &sl->sl_data[idx * sl->sl_item_size], data, sl->sl_item_size);
	sl->sl_usage_count++;
	return OK;
}

xtPublic xtBool xt_sl_delete(XTThreadPtr self, XTSortedListPtr sl, void *key)
{
	void	*result;
	size_t	idx;
	
	if (sl->sl_usage_count == 0)
		return FALSE;
	if (sl->sl_usage_count == 1) {
		if ((*sl->sl_comp_func)(self, sl->sl_thunk, key, sl->sl_data) != 0)
			return FALSE;
		idx = 0;
		result = sl->sl_data;
	}
	else {
		if (!(result = xt_bsearch(self, key, sl->sl_data, sl->sl_usage_count, sl->sl_item_size, &idx, sl->sl_thunk, sl->sl_comp_func)))
			return FALSE;
	}
	if (sl->sl_free_func)
		(*sl->sl_free_func)(self, sl->sl_thunk, result);
	sl->sl_usage_count--;
	XT_MEMMOVE(sl->sl_data, &sl->sl_data[idx * sl->sl_item_size], &sl->sl_data[(idx+1) * sl->sl_item_size], (sl->sl_usage_count-idx) * sl->sl_item_size);
	return TRUE;
}

xtPublic void xt_sl_delete_item_at(XTThreadPtr self, XTSortedListPtr sl, size_t idx)
{
	void *result;

	if (idx >= sl->sl_usage_count)
		return;
	result = &sl->sl_data[idx * sl->sl_item_size];
	if (sl->sl_free_func)
		(*sl->sl_free_func)(self, sl->sl_thunk, result);
	sl->sl_usage_count--;
	XT_MEMMOVE(sl->sl_data, &sl->sl_data[idx * sl->sl_item_size], &sl->sl_data[(idx+1) * sl->sl_item_size], (sl->sl_usage_count-idx) * sl->sl_item_size);
}

xtPublic void xt_sl_remove_from_front(struct XTThread *XT_UNUSED(self), XTSortedListPtr sl, size_t items)
{
	if (sl->sl_usage_count <= items)
		xt_sl_set_size(sl, 0);
	else {
		XT_MEMMOVE(sl->sl_data, sl->sl_data, &sl->sl_data[items * sl->sl_item_size], (sl->sl_usage_count-items) * sl->sl_item_size);
		sl->sl_usage_count -= items;
	}
}

xtPublic void xt_sl_delete_from_info(XTThreadPtr self, XTSortedListInfoPtr li_undo)
{	
	xt_sl_delete(self, li_undo->li_sl, li_undo->li_key);
}

xtPublic void xt_sl_set_size(XTSortedListPtr sl, size_t new_size)
{
	sl->sl_usage_count = new_size;
	if (sl->sl_usage_count + sl->sl_grow_size <= sl->sl_current_size) {
		size_t curr_size;

		curr_size = sl->sl_usage_count;
		if (curr_size < sl->sl_grow_size)
			curr_size = sl->sl_grow_size;

		if (xt_realloc(NULL, (void **) &sl->sl_data, curr_size * sl->sl_item_size))
			sl->sl_current_size = curr_size;
	}
}

xtPublic void *xt_sl_item_at(XTSortedListPtr sl, size_t idx)
{
	if (idx < sl->sl_usage_count)
		return &sl->sl_data[idx * sl->sl_item_size];
	return NULL;
}

xtPublic void *xt_sl_last_item(XTSortedListPtr sl)
{
	if (sl->sl_usage_count > 0)
		return xt_sl_item_at(sl, sl->sl_usage_count - 1);
	return NULL;
}

xtPublic void *xt_sl_first_item(XTSortedListPtr sl)
{
	if (sl->sl_usage_count > 0)
		return xt_sl_item_at(sl, 0);
	return NULL;
}

xtPublic xtBool xt_sl_lock(XTThreadPtr self, XTSortedListPtr sl)
{
	xtBool r = OK;
	
	if (sl->sl_locker != self)
		r = xt_lock_mutex(self, sl->sl_lock);
	if (r) {
		sl->sl_locker = self;
		sl->sl_lock_count++;
	}
	return r;
}

xtPublic void xt_sl_unlock(XTThreadPtr self, XTSortedListPtr sl)
{
	ASSERT(!self || sl->sl_locker == self);
	ASSERT(sl->sl_lock_count > 0);

	sl->sl_lock_count--;
	if (!sl->sl_lock_count) {
		sl->sl_locker = NULL;
		xt_unlock_mutex(self, sl->sl_lock);
	}
}

xtPublic void xt_sl_lock_ns(XTSortedListPtr sl, XTThreadPtr thread)
{
	if (!thread || sl->sl_locker != thread) {
		xt_lock_mutex_ns(sl->sl_lock);
		ASSERT_NS(sl->sl_locker == NULL);
		sl->sl_locker = thread;
	}
	sl->sl_lock_count++;
}

xtPublic void xt_sl_unlock_ns(XTSortedListPtr sl)
{
	ASSERT_NS(!sl->sl_locker || sl->sl_locker == xt_get_self());
	ASSERT_NS(sl->sl_lock_count > 0);

	sl->sl_lock_count--;
	if (!sl->sl_lock_count) {
		sl->sl_locker = NULL;
		xt_unlock_mutex_ns(sl->sl_lock);
	}
}

xtPublic void xt_sl_wait(XTThreadPtr self, XTSortedListPtr sl)
{
	xt_wait_cond(self, sl->sl_cond, sl->sl_lock);
}

xtPublic xtBool xt_sl_signal(XTThreadPtr self, XTSortedListPtr sl)
{
	return xt_signal_cond(self, sl->sl_cond);
}

xtPublic void xt_sl_broadcast(XTThreadPtr self, XTSortedListPtr sl)
{
	xt_broadcast_cond(self, sl->sl_cond);
}

/*
 * -----------------------------------------------------------------------
 * Pointer list
 */

void XTPointerList::pl_init(XTThreadPtr self)
{
	if (!pl_init_ns())
		xt_throw(self);
}

void XTPointerList::pl_exit()
{
	xt_clear_sortedlist(&pl_sortedlist);
}

static int sl_cmp_ptr(struct XTThread *XT_UNUSED(self), register const void *XT_UNUSED(thunk), register const void *a, register const void *b)
{
	void **b_ptr = (void **) b;

	if (a == *b_ptr)
		return 0;
	if (a < *b_ptr)
		return -1;
	return 1;
}

xtBool XTPointerList::pl_init_ns()
{
	return xt_init_sortedlist_ns(&pl_sortedlist, sizeof(void *), 2, 4, sl_cmp_ptr, NULL, NULL, FALSE, FALSE);
}

/*
 * Same as init, but cannot fail because nothing is allocated.
 */
void XTPointerList::pl_setup_ns()
{
	xt_init_sortedlist_ns(&pl_sortedlist, sizeof(void *), 0, 4, sl_cmp_ptr, NULL, NULL, FALSE, FALSE);
}

xtBool XTPointerList::pl_add_pointer(void *ptr)
{
	return xt_sl_insert(NULL, &pl_sortedlist, ptr, &ptr);
}

/*
 * Return TRUE of the pointer was on the list.
 */
xtBool XTPointerList::pl_remove_pointer(void *ptr)
{
	return xt_sl_delete(NULL, &pl_sortedlist, ptr);
}

/*
 * Return NULL if the index is out of range.
 */
void *XTPointerList::pl_get_pointer(u_int index)
{
	void **p_ptr;
	
	if ((p_ptr = (void **) xt_sl_item_at(&pl_sortedlist, index)))
		return *p_ptr;
	return NULL;
}

void XTPointerList::pl_clear()
{
	pl_sortedlist.sl_usage_count = 0;
}

u_int XTPointerList::pl_size()
{
	return pl_sortedlist.sl_usage_count;
}

xtBool XTPointerList::pl_is_empty()
{
	return pl_sortedlist.sl_usage_count == 0;
}

