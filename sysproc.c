#include "types.h"
#include "mmap.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
extern int free_page_count;

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_getnice(void)
{
    int pid;
    if(argint(0, &pid) < 0)
        return -1;
    return getnice(pid);
}


int
sys_setnice(void)
{
    int pid, value;
    if(argint(0, &pid) < 0 || argint(1, &value) < 0)
        return -1;
    return setnice(pid, value);
}


int
sys_ps(void)
{
    int pid;
    if(argint(0, &pid) < 0)
        return -1;
    ps(pid);

    return 0; 
}



int
sys_mmap(void)
{
  int addr;
  int length, prot, flags, fd, offset;

  // Fetch the arguments from the system call
  if(argint(0, &addr) < 0 ||   
     argint(1, &length) < 0 ||
     argint(2, &prot) < 0 ||
     argint(3, &flags) < 0 ||
     argint(4, &fd) < 0 ||
     argint(5, &offset) < 0) {
    return -1; // Return -1 on failure to fetch any argument
  }

  // Call the mmap function implemented in proc.c or another file
  return mmap(addr, length, prot, flags, fd, offset);
}

int
sys_munmap(void)
{
  int addr;
  if(argint(0, &addr) < 0)
    return -1;
  return munmap(addr);
}

int
sys_freemem(void)
{
  return free_page_count;
}

