/* _exit.c - Exit implementation.
   Copyright (C) 2008 Free Software Foundation, Inc.
   Written by Neal H. Walfield <neal@gnu.org>.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <hurd/stddef.h>
#include <hurd/startup.h>
#include <hurd/folio.h>

int __global_zero;

void
_exit (int ret)
{
#if defined (RM_INTERN)
# if defined (__i386__)
  /* Try to invoke the debugger.  */
  asm ("int $3");
# endif
#else
  extern struct hurd_startup_data *__hurd_startup_data;

  addr_t folio = addr_chop (__hurd_startup_data->activity,
			    FOLIO_OBJECTS_LOG2);
  int index = addr_extract (__hurd_startup_data->activity,
			    FOLIO_OBJECTS_LOG2);

  error_t err;
  err = rm_folio_object_alloc (ADDR_VOID, folio, index,
			       cap_void, OBJECT_POLICY_VOID,
			       (uintptr_t) ret,
			       ADDR_VOID, ADDR_VOID);

  assert_perror (err);
#endif

  debug (0, "Failed to die gracefully; doing the ultra-violent.");

  volatile int j = ret / __global_zero;
  for (;;)
    {
      j --;
      l4_yield ();
    }
}