/* pager.h - Generic pager interface.
   Copyright (C) 2007 Free Software Foundation, Inc.
   Written by Neal H. Walfield <neal@gnu.org>.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _HURD_PAGER_H
#define _HURD_PAGER_H

#include <hurd/btree.h>
#include <hurd/addr.h>
#include <hurd/exceptions.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward.  */
struct pager;

/* A fault occured at address ADDR.  If an access occurred, it is
   described by IP and INFO.  Return true if the fault was handler
   (and an appropriate mapping installed), otherwise, false.  */
typedef bool (*pager_fault_t) (struct pager *pager,
			       addr_t addr, uintptr_t ip,
			       struct exception_info info);

/* If not NULL, then the pager is able to free memory.  */
typedef int (*pager_evict_t) (struct pager *pager);

struct pager_region
{
  addr_t start;
  int count;
};

enum
  {
    PAGER_COST_LOW,
    PAGER_COST_MEDIUM,
    PAGER_COST_HIGH
  };

struct pager
{
  hurd_btree_node_t node;

  /* The virtual addresses this pager covers.  If this changes (e.g.,
     if it grows or shrinks), then must be changed using the
     pager_relocate function.  */
  struct pager_region region;

  pager_fault_t fault;

  pager_evict_t evict;
  /* The effort required to reconstruct a freed page.  */
  int reconstruction_effort;
  /* The effort required to scan and free a page.  */
  int eviction_effort;
};

/* Compare two regions.  Two regions are considered equal if there is
   any overlap at all.  */
static int
pager_region_compare (const struct pager_region *a,
		      const struct pager_region *b)
{
  l4_uint64_t a_end = addr_prefix (a->start)
    + (a->count << (ADDR_BITS - addr_depth (a->start)));
  l4_uint64_t b_end = addr_prefix (b->start)
    + (b->count << (ADDR_BITS - addr_depth (b->start)));

  if (a_end < addr_prefix (b->start))
    return -1;
  if (addr_prefix (a->start) > b_end)
    return 1;
  /* Overlap.  */
  return 0;
}

BTREE_CLASS (pager, struct pager, struct pager_region, region, node,
	     pager_region_compare)

/* Install the pager PAGER.  Pagers may not overlap.  Returns true on
   success, false otherwise.  */
extern bool pager_install (struct pager *pager);

/* Change pager PAGER to pager the region REGION.  */
extern bool pager_relocate (struct pager *pager, struct pager_region region);

/* Deinstall an installed pager.  Results are undefined if PAGER is
   not installed.  */
extern void pager_deinstall (struct pager *pager);

/* Raise a fault at address ADDR.  Returns true if the fault was
   handled, false otherwise.  */
extern bool pager_fault (addr_t addr,
			 uintptr_t ip, struct exception_info info);

#endif