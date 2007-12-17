// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */


#ifndef __EBOFS_TYPES_H
#define __EBOFS_TYPES_H

#include <cassert>
#include "include/buffer.h"
#include "include/Context.h"
#include "include/pobject.h"
#include "common/Cond.h"

#include <ext/hash_map>
#include <set>
#include <list>
#include <vector>
using namespace std;
using namespace __gnu_cxx;


#include "include/object.h"

#include "csum.h"

#ifndef MIN
# define MIN(a,b)  ((a)<=(b) ? (a):(b))
#endif
#ifndef MAX
# define MAX(a,b)  ((a)>=(b) ? (a):(b))
#endif


// disk
typedef uint64_t block_t;        // disk location/sector/block

static const int EBOFS_BLOCK_SIZE = 4096;
static const int EBOFS_BLOCK_BITS = 12;    // 1<<12 == 4096

struct Extent {
  block_t start, length;

  Extent() : start(0), length(0) {}
  Extent(block_t s, block_t l) : start(s), length(l) {}

  block_t last() const { return start + length - 1; }
  block_t end() const { return start + length; }
};

inline ostream& operator<<(ostream& out, const Extent& ex)
{
  return out << ex.start << "~" << ex.length;
}


// objects

typedef uint64_t coll_t;

struct ebofs_onode {
  csum_t data_csum;

  Extent onode_loc;       /* this is actually the block we live in */
  pobject_t object_id;       /* for kicks */
  __u8 readonly;

  __s64 object_size;     /* file size in bytes.  should this be 64-bit? */
  __u32 alloc_blocks;   // allocated
  
  __u16 inline_bytes;
  __u16 num_collections;
  __u32 num_attr;        // num attr in onode
  __u32 num_extents;     /* number of extents used.  if 0, data is in the onode */
  __u32 num_bad_byte_extents; // undefined partial byte extents over partial blocks; block checksums reflect zeroed data beneath these.
};

struct ebofs_cnode {
  Extent     cnode_loc;       /* this is actually the block we live in */
  coll_t     coll_id;
  __u32      num_attr;        // num attr in cnode
};

struct ebofs_onode_ptr {
  Extent loc;
  csum_t csum;
};


// tree/set nodes
//typedef int    nodeid_t;
typedef __s64 nodeid_t;     // actually, a block number.  FIXME.

static const unsigned EBOFS_NODE_BLOCKS = 1;
static const unsigned EBOFS_NODE_BYTES = EBOFS_NODE_BLOCKS * EBOFS_BLOCK_SIZE;
static const unsigned EBOFS_MAX_NODE_REGIONS = 10;   // pick a better value!
static const unsigned EBOFS_NODE_DUP = 3;

struct ebofs_nodepool {
  Extent node_usemap_even;   // for even sb versions
  Extent node_usemap_odd;    // for odd sb versions
  
  __u32  num_regions;
  Extent region_loc[EBOFS_MAX_NODE_REGIONS];
};

// table

struct ebofs_node_ptr {
  nodeid_t nodeid;
  //__u64 start[EBOFS_NODE_DUP];
  //__u64 length;
  csum_t csum;
};

struct ebofs_table {
  ebofs_node_ptr root;
  __u32    num_keys;
  __u32    depth;
};


// super
typedef uint64_t version_t;

static const __u64 EBOFS_MAGIC = 0x000EB0F5;

static const int EBOFS_NUM_FREE_BUCKETS = 5;   /* see alloc.h for bucket constraints */
static const int EBOFS_FREE_BUCKET_BITS = 2;

struct ebofs_super {
  __u64 s_magic;
  __u64 fsid;   /* _ebofs_ fsid, mind you, not ceph_fsid_t. */

  epoch_t epoch;             // version of this superblock.

  uint64_t num_blocks;        /* # blocks in filesystem */

  // some basic stats, for kicks
  uint64_t free_blocks;       /* unused blocks */
  uint64_t limbo_blocks;      /* limbo blocks */
  //unsigned num_objects;
  //unsigned num_fragmented;
  
  struct ebofs_nodepool nodepool;
  
  // tables
  struct ebofs_table free_tab[EBOFS_NUM_FREE_BUCKETS];  
  struct ebofs_table limbo_tab;
  struct ebofs_table alloc_tab;
  struct ebofs_table object_tab;      // object directory
  struct ebofs_table collection_tab;  // collection directory
  struct ebofs_table co_tab;

  csum_t super_csum;

  csum_t calc_csum() {
    return ::calc_csum((char*)this, (unsigned long)&super_csum-(unsigned long)this);
  }
  bool is_corrupt() { 
    csum_t actual = calc_csum();
    if (actual != super_csum) 
      return true;
    else 
      return false;
  }
  bool is_valid_magic() { return s_magic == EBOFS_MAGIC; }
  bool is_valid() { return is_valid_magic() && !is_corrupt(); }
};


#endif
