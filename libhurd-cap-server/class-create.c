/* class-create.c - Create a capability class.
   Copyright (C) 2004 Free Software Foundation, Inc.
   Written by Marcus Brinkmann <marcus@gnu.org>

   This file is part of the GNU Hurd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include <hurd/cap-server.h>


/* Initialize the slab object pointed to by BUFFER.  HOOK is as
   provided to hurd_slab_create.  */
static error_t
_hurd_cap_obj_constructor (void *hook, void *buffer)
{
  hurd_cap_class_t cap_class = (hurd_cap_class_t) hook;
  hurd_cap_obj_t obj = (hurd_cap_obj_t) buffer;
  error_t err;

  /* First do our own initialization.  */
  obj->refs = 1;
  err = pthread_mutex_init (&obj->lock, 0);
  if (err)
    return err;

  /* Then do the user part, if necessary.  */
  if (cap_class->obj_init)
    {
      err = (*cap_class->obj_init) (cap_class, obj);
      if (err)
	{
	  error_t pthread_err = pthread_mutex_destroy (&obj->lock);

	  assert (!pthread_err);
	  return err;
	}
    }
  return 0;
}


/* Destroy the slab object pointed to by BUFFER.  HOOK is as provided
   to hurd_slab_create.  This might be called with the */
static void
_hurd_cap_obj_destructor (void *hook, void *buffer)
{
  hurd_cap_class_t cap_class = (hurd_cap_class_t) hook;
  hurd_cap_obj_t obj = (hurd_cap_obj_t) buffer;
  error_t err;

  if (cap_class->obj_destroy)
    (*cap_class->obj_destroy) (cap_class, obj);

  err = pthread_mutex_destroy (&obj->lock);
  assert (!err);
}


/* Create a new capability class for objects with the size SIZE,
   including the struct hurd_cap_obj, which has to be placed at the
   beginning of each capability object.

   The callback OBJ_INIT is used whenever a capability object in this
   class is created.  The callback OBJ_REINIT is used whenever a
   capability object in this class is deallocated and returned to the
   slab.  OBJ_REINIT should return a capability object that is not
   used anymore into the same state as OBJ_INIT does for a freshly
   allocated object.  OBJ_DESTROY should deallocate all resources for
   this capablity object.  Note that if OBJ_INIT or OBJ_REINIT fails,
   the object is considered to be fully destroyed.  No extra call to
   OBJ_DESTROY will be made for such objects.

   The new capability class is returned in R_CLASS.  If the creation
   fails, an error value will be returned.  */
error_t
hurd_cap_class_create (size_t size, hurd_cap_obj_init_t obj_init,
		       hurd_cap_obj_alloc_t obj_alloc,
		       hurd_cap_obj_reinit_t obj_reinit,
		       hurd_cap_obj_destroy_t obj_destroy,
		       hurd_cap_class_t *r_class)
{
  error_t err;

  hurd_cap_class_t cap_class = malloc (sizeof (struct hurd_cap_class));

  if (!cap_class)
    return errno;

  assert (size >= sizeof (struct hurd_cap_obj));
  cap_class->obj_size = size;

  cap_class->obj_init = obj_init;
  cap_class->obj_alloc = obj_alloc;
  cap_class->obj_reinit = obj_reinit;
  cap_class->obj_destroy = obj_destroy;

  err = hurd_slab_create (size, 0, _hurd_cap_obj_constructor,
			  _hurd_cap_obj_destructor, cap_class,
			  &cap_class->slab);
  if (err)
    {
      free (cap_class);
      return err;
    }

  *r_class = cap_class;
  return 0;
}