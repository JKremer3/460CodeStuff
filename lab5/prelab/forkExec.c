
int body(), goUmode();

PROC *kfork(char *filename)
{
  // LOOK HERE, entries that are missing in kfork2
  int i; 
  int *ptable, pentry;
  char *addr;
  /*
  char *cp, *cq;

  char line[8];
  int usize1, usize;
  int *ustacktop, *usp;
  u32 BA, Btop, Busp;
  */
  PROC *p = dequeue(&freeList);
  if (p==0){
    kprintf("kfork failed\n");
    return (PROC *)0;
  }
  p->ppid = running->pid;
  p->parent = running;
  p->parent = running;
  p->status = READY;
  p->priority = 1;
  p->cpsr = (int *)0x10;
  p->sibling = 0;
  p->child = 0;

 // build p's pgtable 
  p->pgdir = (int *)(0x600000 + (p->pid - 1)*0x4000);
  ptable = p->pgdir;
  // initialize pgtable
  for (i=0; i<4096; i++)
    ptable[i] = 0;
  pentry = 0x412;
  for (i=0; i<258; i++){
    ptable[i] = pentry;
    pentry += 0x100000;
  }
  // ptable entry flag=|AP0|doma|1|CB10|=110|0001|1|1110|=0xC3E or 0xC32       
  //ptable[2048] = 0x800000 + (p->pid - 1)*0x100000|0xC3E;
  ptable[2048] = 0x800000 + (p->pid - 1)*0x100000|0xC32;
  
  p->cpsr = (int *)0x10;    // previous mode was Umode
  
  if(p->parent->child == 0)
  {
    p->parent->child = p;
  }
  else
  {
    PROC *temp = p->parent->child;

    while(temp->sibling != 0)
    {
      temp = temp->sibling;
    }

    temp->sibling = p;
  }
  // set kstack to resume to goUmode, then to VA=0 in Umode image
  for (i=1; i<29; i++)  // all 28 cells = 0
    p->kstack[SSIZE-i] = 0;

  p->kstack[SSIZE-15] = (int)goUmode;  // in dec reg=address ORDER !!!
  p->ksp = &(p->kstack[SSIZE-28]);

  // kstack must contain a resume frame FOLLOWed by a goUmode frame
  //  ksp  
  //  -|-----------------------------------------
  //  r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 fp ip pc|
  //  -------------------------------------------
  //  28 27 26 25 24 23 22 21 20 19 18  17 16 15
  //  
  //   usp      NOTE: r0 is NOT saved in svc_entry()
  // -|-----goUmode--------------------------------
  //  r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 ufp uip upc|
  //-------------------------------------------------
  //  14 13 12 11 10 9  8  7  6  5  4   3    2   1
  /********************

  // to go Umode, must set new PROC's Umode cpsr to Umode=10000
  // this was done in ts.s dring init the mode stacks ==> 
  // user mode's cspr was set to IF=00, mode=10000  

  ***********************/
  // must load filename to Umode image area at 7MB+(pid-1)*1MB
  addr = (char *)(0x700000 + (p->pid)*0x100000);
  kprintf("\n\npassing in %s\n\n", filename);
  load(filename, p);
  
  // must fix Umode ustack for it to goUmode: how did the PROC come to Kmode?
  // by swi # from VA=0 in Umode => at that time all CPU regs are 0
  // we are in Kmode, p's ustack is at its Uimage (8mb+(pid-1)*1mb) high end
  // from PROC's point of view, it's a VA at 1MB (from its VA=0)
  // but we in Kmode must access p's Ustack directly

  /***** this sets each proc's ustack differently, thinking each in 8MB+
  ustacktop = (int *)(0x800000+(p->pid)*0x100000 + 0x100000);
  TRY to set it to OFFSET 1MB in its section; regardless of pid
  **********************************************************************/
  //p->usp = (int *)(0x80100000);
  p->usp = (int *)VA(0x100000);

  //  p->kstack[SSIZE-1] = (int)0x80000000;
  p->kstack[SSIZE-1] = VA(0);
  // -|-----goUmode-------------------------------------------------
  //  r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 ufp uip upc|string       |
  //----------------------------------------------------------------
  //  14 13 12 11 10 9  8  7  6  5  4   3    2   1 |             |

  enqueue(&readyQueue, p);

  kprintf("proc %d kforked a child %d: ", running->pid, p->pid); 
  printQ(readyQueue);

  return p;
}
//pgdir fork?
int kfork2()
{
  // LOOK HERE, what is the descrepency between kfork and this, which should be named fork, not kfork2
  int i;
  char *PA, *CA;
  PROC *p = getproc(&freeList);

  if(p== 0)
  {
    kprintf("Fork Failed\n");
    return -1;
  }

  p->ppid = running->pid;
  p->parent = running;
  p->status = READY;
  p->priority = 1;
  p->pgdir = (int *)(0x600000 + (p->pid - 1)*0x4000);

  // LOOK HERE where is the page table initialization or page entry?

  PA = running->pgdir[2048] & 0xFFFF0000;
  CA = p->pgdir[2048] & 0xFFFF0000;

  memcpy(CA, PA, 0x100000);

  for (i=1; i <=14; i++)
  {
    p->kstack[SSIZE-1] = running->kstack[SSIZE -1];
  }
  //LOOK HERE what about the rest of the registers? 

  p->kstack[SSIZE - 14] = 0;
  p->kstack[SSIZE - 15] = (int)goUmode;
  p->ksp = &(p->kstack[SSIZE - 28]);
  p->usp = running->usp;
  //p->ucpsr = running-> ucpsr
  enqueue(&readyQueue, p);
  return p->pid;
}

int kexec(char *cmdline)
{
    int i, upa, usp;
    char *cp, kline[128], file[32], filename[32];
    PROC *p = running;
    kstrcpy(kline, cmdline);
    //get first token of kline as filename
    cp = kline; i = 0;
    while(*cp != '\0'){
        filename[i] = *cp;
        i++; cp++;
    }
    filename[i] = 0;
    file[0] = 0;
    //if filename relative
    // if(filename[0] != '/'){
    //     kstrcpy(file, ""); //prefix with /bin/
    // }
    kstrcat(file, filename);
    upa = p->pgdir[2048] & 0xFFFF0000; //PA of umode image
    if(!load(file, p)){
        return -1;
    }

    //copy cmdline to high end of Ustack in Umode image
    usp = upa + 0x100000 - 128;
    kstrcpy((char *) usp, kline);
    p->usp = ((int *)VA(0x100000 - 128));
    //fix syscall fram in kstack to return to VA=0 of new image
    for(i = 2; i<14; i++){ //cleatr umode regs r1-r12
        p->kstack[SSIZE - i] - 0;
    }
    // p->kstack[SSIZE-14] = (int)VA(p->usp);
    p->kstack[SSIZE - 1] = (int)VA(0);

    return (int)p->usp; //will replace saved r0 in the kstack
}