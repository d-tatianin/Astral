#ifndef _VMM_H
#define _VMM_H

#include <arch/mmu.h>

#define VMM_FLAGS_PAGESIZE 1
#define VMM_FLAGS_ALLOCATE 2
#define VMM_FLAGS_PHYSICAL 4
#define VMM_FLAGS_FILE     8
#define VMM_FLAGS_EXACT   16
#define VMM_FLAGS_SHARED  32
#define VMM_FLAGS_COPY    64

#define VMM_PERMANENT_FLAGS_MASK (VMM_FLAGS_FILE | VMM_FLAGS_SHARED | VMM_FLAGS_COPY)

struct vmmcache_t;
typedef struct vmmrange_t{
	struct vmmrange_t *next;
	struct vmmrange_t *prev;
	void *start;
	size_t size;
	int flags;
	mmuflags_t mmuflags;
	void *private;
} vmmrange_t;

typedef struct {
	// TODO lock
	struct vmmcache_t *next;
	size_t freecount;
	uintmax_t firstfree;
} vmmcacheheader_t;

#define VMM_RANGES_PER_CACHE (PAGE_SIZE - sizeof(vmmcacheheader_t)) / sizeof(vmmrange_t)

typedef struct vmmcache_t {
	vmmcacheheader_t header;
	vmmrange_t ranges[VMM_RANGES_PER_CACHE];
} vmmcache_t;

typedef struct {
	vmmrange_t *ranges;
	void *start;
	void *end;
} vmmspace_t;

typedef struct {
	vmmspace_t space;
	pagetableptr_t pagetable;
} vmmcontext_t;

void *vmm_map(void *addr, size_t size, int flags, mmuflags_t mmuflags, void *private);
void vmm_unmap(void *addr, size_t size, int flags);
vmmcontext_t *vmm_newcontext();
void vmm_init();

#endif