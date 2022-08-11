// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(int cpuID,void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

struct spinlock allLock;

void steal(int cpuId){
    release(&kmem[cpuId].lock);
    acquire(&allLock);
    for(int i = 0;i < NCPU;i++){
        if(i == cpuId)
            continue;

        // steal.
        acquire(&kmem[i].lock);

        if(kmem[i].freelist){
            struct run* s = kmem[i].freelist;
            while(s->next)
                s = s->next;
            s->next = kmem[cpuId].freelist;
            kmem[cpuId].freelist = kmem[i].freelist;
            kmem[i].freelist = 0;
        }

        release(&kmem[i].lock);
    }

    acquire(&kmem[cpuId].lock);
    release(&allLock);
}

void
kinit()
{
  initlock(&allLock, "all");
  uint64 partLen = (PHYSTOP - (uint64)end)>>3;
    for(uint64 i = 0;i < NCPU;i++){
        initlock(&kmem[i].lock, "kmem");
        void *st = (void *)PGROUNDUP((uint64) (end) + i*partLen);
        void *ed = (void *)PGROUNDUP((uint64) (end) + (i + 1)*partLen);
        freerange(i,st, ed);
    }
}

void
freerange(int cpuID,void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    struct run *r=(struct run*)p;
    r->next=kmem[cpuID].freelist;
    kmem[cpuID].freelist=r;
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  push_off();
  int cpuID=cpuid();

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[cpuID].lock);
  r->next = kmem[cpuID].freelist;
  kmem[cpuID].freelist = r;
  release(&kmem[cpuID].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  push_off();
  struct run *r;
  int cpuID=cpuid();

  acquire(&kmem[cpuID].lock);
  r = kmem[cpuID].freelist;
  if(!r){
      steal(cpuID);
      r = kmem[cpuID].freelist;
  }
  if(r)
    kmem[cpuID].freelist = r->next;
  release(&kmem[cpuID].lock);
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


