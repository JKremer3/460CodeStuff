int tswitch();

int sleep(int event)
{
  printf("proc %d going to sleep on event=%d\n", running->pid, event);

  running->event = event;
  running->status = SLEEP;
  enqueue(&sleepList, running);
  printList("sleepList", sleepList);
  tswitch();
}

int wakeup(int event)
{
  PROC *temp, *p;
  temp = 0;
  printList("sleepList", sleepList);

  while (p = dequeue(&sleepList)){
     if (p->event == event){
	printf("wakeup %d\n", p->pid);
	p->status = READY;
	enqueue(&readyQueue, p);
     }
     else{
	enqueue(&temp, p);
     }
  }
  sleepList = temp;
  printList("sleepList", sleepList);
}

int kexit(int exitValue)
{
  PROC *p;
  printf("proc %d in kexit(), value=%d\n", running->pid, exitValue);
  
  if(running->pid == 1)
  {
    printf("Heroes never die\n");
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
  
  wakeup(&proc[1]);
  wakeup(running->parent);

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
    printf("There are no children here!\n");
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
    sleep(running->pid);
  }
  
}

  
