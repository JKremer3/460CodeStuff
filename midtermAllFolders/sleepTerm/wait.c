int tswitch();

int ksleep(int event)
{
  int sr = int_off();
  kprintf("proc %d going to sleep on event=%d\n", running->pid, event);

  running->event = event;
  running->status = SLEEP;
  enqueue(&sleepList, running);
  printList("sleepList", sleepList);
  tswitch();
  int_on(sr);
}

int kwakeup(int event)
{
  PROC *temp, *p;
  temp = 0;
  int sr = int_off();
  
  printList("sleepList", sleepList);

  while (p = dequeue(&sleepList)){
     if (p->event == event){
	kprintf("wakeup %d\n", p->pid);
	p->status = READY;
	enqueue(&readyQueue, p);
     }
     else{
	enqueue(&temp, p);
     }
  }
  sleepList = temp;
  printList("sleepList", sleepList);
  int_on(sr);
}

int kexit(int exitValue)
{
  PROC *p;
  kprintf("proc %d in kexit(), value=%d\n", running->pid, exitValue);
  
  if(running->pid == 1)
  {
    kprintf("Heroes never die\n");
    return -1;
  }
  
  for (int i = 0; i < NPROC; i++)
  {
    p = &proc[i];
    if( p->status != FREE && p->ppid == running->pid)
    {
      p->ppid = 1;
      p->parent = &proc[i];
    }
  }
  
  running->exitCode = exitValue;
  running->status = ZOMBIE;
  
  kwakeup(&proc[1]);
  kwakeup(running->parent);

  tswitch();
}

int kwait(int *status)
{
  int i;
  for (i = 0; i <NPROC; i++)
  {
    if(proc[i].ppid == running->pid && proc[i].status != FREE && proc[i].status != ZOMBIE)
      break;
  }

  //if no children, stop
  if(i == NPROC)
  {
    kprintf("There are no children here!\n");
    return -1;
  }

  while(1){
    //find zombie(s)
    for(i = 0; i< NPROC; i++)
    {
      if(proc[i].status == ZOMBIE && proc[i].ppid == running->pid)
      {
        *status =proc[i].exitCode;
        proc[i].status = FREE;
        enqueue(&freeList, &proc[i]);
        return proc[i].pid;
      }
    }
    ksleep(running->pid);
  }
  
}
  
