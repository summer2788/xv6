#ifndef _MM_H_
#define _MM_H_

#define MMAPBASE 0x40000000
#define MAX_MMAP_AREA 64

struct mmap_area {
  struct file *f; // Backing file
  uint addr;      // Start address of the mapping
  int length;     // Length of the mapping
  int offset;     // Offset in the file
  int prot;       // Protection flags
  int flags;      // Flags
  struct proc *p; // Process owning this mapping
};

extern struct mmap_area mmap_array[MAX_MMAP_AREA];


#endif // _MM_H_
