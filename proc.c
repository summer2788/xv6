#include "types.h"
#include "mmu.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "stringutil.h" 
#include "mmap.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#define INT_MAX 2147483647

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;


const int sched_prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};



int nextpid = 1;
extern void forkret(void);
extern void trapret(void);
extern uint ticks;

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->nice = 20; //set default nice value.
  p->weight = sched_prio_to_weight[p->nice];  // Using the provided array, index 20 for default nice value
  p->vruntime.high = 0;  // Start with 0 vruntime
  p->vruntime.low =0; 
  p->runtime = 0;   // Start with 0 runtime
  p->timeslice = 0;  // Will be calculated later in scheduler	
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  np->vruntime = curproc->vruntime;
  np->nice  = curproc->nice;
  np->runtime = 0;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.




// This function returns the total weight of all RUNNABLE processes.
// This function only run within scheduler() function 
int compute_total_weight_of_runnable_processes() {
    struct proc *pr;
    int total_weight = 0;

    for(pr = ptable.proc; pr < &ptable.proc[NPROC]; pr++) {
        if(pr->state == RUNNABLE) {
            total_weight += sched_prio_to_weight[pr->nice];
        }
    }

    return total_weight;
}


void scheduler(void) {
  struct proc *p;
  struct proc *min_vruntime_proc;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    sti();
    min_vruntime_proc = 0;

    acquire(&ptable.lock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
	
	//calculate total weight of runnable processes 
    
    // Traverse the ptable to find the process with the minimum vruntime
    // Pick the process with the minimum vruntime
      if(!min_vruntime_proc || compare(p->vruntime, min_vruntime_proc->vruntime)==1)
        min_vruntime_proc = p;
    }
	// If we found a process to run
    if(min_vruntime_proc){
    
      // Calculate time slice 
      int total_weight = compute_total_weight_of_runnable_processes(); 
      min_vruntime_proc->timeslice = (10000 * min_vruntime_proc->weight) / total_weight;
      //change the current process running on this CPU
      c->proc = min_vruntime_proc; 
      c->proc->cpu_start_time = ticks;
      //switch to the user space memory of the selected process.
      switchuvm(min_vruntime_proc); 
      //change the process's state
      min_vruntime_proc->state = RUNNING;
      //actual context switching 
      swtch(&(c->scheduler), min_vruntime_proc->context); 
      //change to the kernel space 
      switchkvm();

      c->proc = 0;
    }

    release(&ptable.lock);
  }
}



// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//This function only for wakeup1
struct bigint compute_min_vruntime() {
    struct proc *p;
    struct bigint min_vruntime;
    struct bigint temp_vruntime;
    struct bigint zero = {0, 0};

    // Initialize min_vruntime to the low maximum value
    min_vruntime.high = 9999999;
    min_vruntime.low = 9999999;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if(p->state == RUNNABLE && compare(p->vruntime, min_vruntime) == 1) { // if p->vruntime < min_vruntime
            min_vruntime = p->vruntime;
        }
    }

    // Check if min_vruntime is still at its initial maximum value, which means no runnable process
    temp_vruntime.high = 9999999;
    temp_vruntime.low = 9999999;

    if(compare(min_vruntime, temp_vruntime) == 2) {  // if min_vruntime == INT_MAX
        return zero;   //set 0
    }

    return min_vruntime;  // min_vruntime
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;
  struct bigint zero = {0,0};
  struct bigint adjustment_value;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      //update vruntime based on the minumum vruntime of runnable processes
      p->vruntime = compute_min_vruntime();
      if(compare(p->vruntime, zero) != 2) {
         // If p->vruntime is NOT zero
         adjustment_value.high = 0;
         adjustment_value.low = 1000 * 1024 / sched_prio_to_weight[p->nice];
	 p->vruntime = subtract(p->vruntime, adjustment_value);
      }

      p->state = RUNNABLE;
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

//retrieves the nice value of target process ID.
//Return -1 if there is no process corresponing to the pid
int
getnice(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid && p->state != UNUSED) {
      release(&ptable.lock);
      return p->nice;
    }
  }
  release(&ptable.lock);
  return -1;  // Return -1 if no corresponing process found 
}

//sets the nice value of target process ID
//Return 0 on success. Return -1 if there is no corresponding process or nice value is invalid 

int
setnice(int pid, int value)
{
  struct proc *p;

  if (value < 0 || value > 39) // Check for range of valid
    return -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid && p->state != UNUSED) {
      p->nice = value;
      release(&ptable.lock);
      return 0;  // Return 0 for success
    }
  }
  release(&ptable.lock);
  return -1;  // Return -1 if no corresponding process 
}



void bigint_to_str(struct bigint num, char *buf) {
    if (num.high == 0) {
        itoa(num.low, buf, 10);
    } else {
        char low_buf[8]; // Assuming the low part has 7 digits at max.
        itoa(num.low, low_buf, 10);
        itoa(num.high, buf, 10);
        // concatenate low_buf to buf
        int i = 0, j = 0;
        while (buf[i] != '\0') {
            i++;
        }
        while (low_buf[j] != '\0') {
            buf[i] = low_buf[j];
            i++; j++;
        }
        buf[i] = '\0';
    }
}



//right above the ps function 
void print_ps(const char *name, int pid, const char *state, int priority, 
              int runtime_weight, int runtime, struct bigint vruntime, int printHeader) {
    
    char name_buf[11], pid_buf[6], state_buf[11], priority_buf[11], 
         runtime_weight_buf[17], runtime_buf[11], vruntime_buf[15], tick_buf[11];

         
    char padded[20]; // Assuming no strings will exceed this length

	 if (printHeader) {
        padstring(padded, "name", 10);
        cprintf("%s", padded);
        padstring(padded, "pid", 5);
        cprintf("%s", padded);
        padstring(padded, "state", 10);
        cprintf("%s", padded);
        padstring(padded, "priority", 10);
        cprintf("%s", padded);
        padstring(padded, "runtime/weight", 16);
        cprintf("%s", padded);
        padstring(padded, "runtime", 10);
        cprintf("%s", padded);
        padstring(padded, "vruntime", 14);
        cprintf("%s", padded);
        cprintf("%s  ", "tick");
	itoa(ticks*1000, tick_buf, 10);
	padstring(tick_buf, tick_buf, 10);
	cprintf("%s\n", tick_buf);
	return;
    }

    // Convert numbers to string and pad
    itoa(pid, pid_buf, 10);
    padstring(pid_buf, pid_buf, 5);
    
    itoa(priority, priority_buf, 10);
    padstring(priority_buf, priority_buf, 10);
    
    itoa(runtime_weight, runtime_weight_buf, 10);
    padstring(runtime_weight_buf, runtime_weight_buf, 16);

    itoa(runtime, runtime_buf, 10);
    padstring(runtime_buf, runtime_buf, 10);

    bigint_to_str(vruntime, vruntime_buf);
    padstring(vruntime_buf, vruntime_buf, 14);


    // Pad other strings
    padstring(name_buf, name, 10);
    padstring(state_buf, state, 10);


    // Print the values using padded strings
    cprintf("%s%s%s%s%s%s%s\n",
            name_buf, pid_buf, state_buf, priority_buf,
            runtime_weight_buf, runtime_buf, vruntime_buf);
}

//prints process information, which includes name,pid,state and priority(nice value)
//of each process.
//if the pid is 0, print out all process' information.
//Otherwise, print out corresponding process's information.
//if there is no process corresponding to the pid, print out nothing.
void
ps(int pid)
{
  struct proc *p;
   int existProcess = 0;  // flag to check if there's a process
  const char* stateNames[] = {"UNUSED", "EMBRYO", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"};
  
  acquire(&ptable.lock);

  // Check if there's any process
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->state != UNUSED && (pid == 0 || p->pid == pid)) {
      existProcess = 1;
      break;
    }
  }

  // If there's any process, then print the header
  if(existProcess) {
      print_ps(0, 0, 0, 0, 0, 0, p->vruntime, 1);
  }


  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->state != UNUSED) {
     if(pid == 0 || p->pid == pid) { // If pid is 0, print all. Else print matching pid.iii
	     print_ps(p->name, p->pid, stateNames[p->state], p->nice,
             p->runtime / sched_prio_to_weight[p->nice], p->runtime,
             p->vruntime,0);
     }
   }
  }
  release(&ptable.lock);
}


uint mmap(uint addr, int length, int prot, int flags, int fd, int offset)
{
    // addr must be page aligned, length must be multiple of page size
    if (addr % PGSIZE != 0 || length % PGSIZE != 0)
    {
        return 0; // Failure
    }

    struct proc *p = myproc();
    uint start_addr = MMAPBASE + addr;
    int perm = PTE_U; // User bit must be set for user pages
    if (prot & PROT_READ)
        perm |= PTE_P; // Present
    if (prot & PROT_WRITE)
        perm |= PTE_W; // Writable
    struct file *f;

    // Ensure that the mapping doesn't overlap existing mappings
    // [Omitted: Check for overlap with existing mmap areas or other segments.]

    // Find a free mmap_area structure to use
    struct mmap_area *mmap = 0;
    for (int i = 0; i < MAX_MMAP_AREA; i++)
    {
        if (mmap_array[i].addr == 0)
        {
            mmap = &mmap_array[i];
            break;
        }
    }
    if (mmap == 0)
    {
        return 0; // No free mmap_area structure available
    }

    // Whether 'MAP_POPULATE' is given, the case is separated.
    if (flags & MAP_POPULATE)
    {
        // Allocate the page from physical memory and map it
        if (fd == -1 && !(flags & MAP_ANONYMOUS))
        {
            return 0; // Error, MAP_POPULATE without MAP_ANONYMOUS requires a valid fd
        }

        if (!(flags & MAP_ANONYMOUS))
        {
            // This is a file-backed mapping
            if ((f = p->ofile[fd]) == 0)
                return 0; // The file descriptor is not open

            begin_op();
            ilock(f->ip);
            // Check file permissions here (implementation details omitted)
            if (prot & PROT_WRITE)
            {
                // Trying to map the file as writable, ensure it's not opened as read-only
                if (f->readable == 0)
                {
                    iunlock(f->ip);
                    end_op();
                    return 0; // Error, the file is not writable
                }
            }

            if (prot & PROT_READ)
            {
                // Trying to map the file as readable, ensure it's not opened as write-only
                if (f->writable == 0)
                {
                    iunlock(f->ip);
                    end_op();
                    return 0; // Error, the file is not readable
                }
            }

            // Map pages for the required length
            for (int i = 0; i < length; i += PGSIZE)
            {
                char *mem = kalloc();
                if (mem == 0)
                {
                    iunlock(f->ip);
                    end_op();
                    return 0; // Failed to allocate memory
                }

                memset(mem, 0, PGSIZE);
                if (readi(f->ip, mem, offset + i, PGSIZE) != PGSIZE)
                {
                    kfree(mem);
                    iunlock(f->ip);
                    end_op();
                    return 0; // Error reading from file
                }

                if (mappages(p->pgdir, (char *)(start_addr + i), PGSIZE, V2P(mem), perm) < 0)
                {
                    kfree(mem);
                    iunlock(f->ip);
                    end_op();
                    return 0; // Failed to map pages
                }
            }
            mmap->f = f;
            iunlock(f->ip);
            end_op();
        }
        else
        {
            // This is an anonymous mapping, populate it with zeroed pages
            for (int i = 0; i < length; i += PGSIZE)
            {
                char *mem = kalloc();
                if (mem == 0)
                    return 0; // Failed to allocate memory
                memset(mem, 0, PGSIZE);
                if (mappages(p->pgdir, (char *)(start_addr + i), PGSIZE, V2P(mem), perm) < 0)
                {
                    kfree(mem);
                    return 0; // Failed to map pages
                }
            }
        }
    }
    else
    {
        // MAP_POPULATE is not given, so just reserve the address space
        // The actual page allocation will be handled lazily (on page fault)
        // [Omitted: Logic to reserve address space without populating pages]
    }

    // allocate the mmap_area structure
    mmap->addr = start_addr;
    mmap->length = length;
    mmap->prot = prot;
    mmap->flags = flags;
    mmap->offset = offset;
    mmap->p = p;

    return start_addr; // Return the start address of the mapping area
}

int handle_page_fault(struct trapframe *tf)
{
    uint fault_addr = rcr2(); // Step 1: Get faulting address
    int write = tf->err & 2;  // Step 2: Get write flag
    struct mmap_area *mmap = 0;

    // Step 3: Find corresponding mmap_area
    for (int i = 0; i < MAX_MMAP_AREA; i++)
    {
        if (mmap_array[i].addr <= fault_addr &&
            fault_addr < mmap_array[i].addr + mmap_array[i].length)
        {
            mmap = &mmap_array[i];
            break;
        }
    }
    if (!mmap)
    {
        return -1; // Failed, no corresponding mmap_area
    }

    // Step 4: Write check
    if (write && !(mmap->prot & PROT_WRITE))
    {
        return -1; // Failed, write access to a write-protected area
    }

    // Step 5: Handle the page fault
    // 5.1 Allocate a new physical page
    char *mem = kalloc();
    if (!mem)
    {
        return -1; // Failed, cannot allocate memory
    }
    memset(mem, 0, PGSIZE); // 5.2 Fill new page with 0

    if (!(mmap->flags & MAP_ANONYMOUS))
    {
        // 5.3 If it is file mapping, read file into the physical page with offset
        begin_op();
        ilock(mmap->f->ip);
        if (readi(mmap->f->ip, mem, mmap->offset + (fault_addr - mmap->addr), PGSIZE) != PGSIZE)
        {
            iunlock(mmap->f->ip);
            end_op();
            kfree(mem);
            return -1; // Failed, error while reading file
        }
        iunlock(mmap->f->ip);
        end_op();
    }
    // Otherwise, it's an anonymous mapping, just use the zero-filled page.
    int perm = PTE_P; // PTE_P is the Present flag, which must always be set

    if (mmap->prot & PROT_WRITE)
    {
        perm |= PTE_W; // Set the PTE_W flag if PROT_WRITE is set
    }
    // 5.4 Map the new page in the page table
    if (mappages(myproc()->pgdir, (char *)PGROUNDDOWN(fault_addr), PGSIZE, V2P(mem), perm) < 0)
    {
        kfree(mem);
        return -1; // Failed, error in mappages
    }

    return 1; // Succeed
}

int munmap(uint addr) {
  struct proc *p = myproc(); // Get current process
  struct mmap_area *mmap = 0;

  // Step 2: Find the corresponding mmap_area structure
  for (int i = 0; i < MAX_MMAP_AREA; i++) {
    if (mmap_array[i].addr == addr) {
      mmap = &mmap_array[i];
      break;
    }
  }

  // If there is no mmap_area starting with the address, return -1
  if (mmap == 0) {
    return -1;
  }

  // Step 3 & 4: Free any allocated physical pages and remove mmap_area structure
  for (int i = 0; i < mmap->length; i += PGSIZE) {
    char *mem = (char*)(mmap->addr + i);
    pte_t *pte = walkpgdir(p->pgdir, mem, 0);
    if (pte && (*pte & PTE_P)) {
      char *v = P2V(PTE_ADDR(*pte));
      memset(v, 1, PGSIZE); // Fill with 1s before freeing
      kfree(v);
      *pte = 0;
    }
  }
  // Invalidate the TLB for this process after unmapping pages
  switchuvm(p);

  // Step 5: Clear the mmap_area structure
  memset(mmap, 0, sizeof(*mmap));

  return 1; // Success
}
