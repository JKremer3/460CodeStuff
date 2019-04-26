#include "ucode.c"
#define BLK 1024

char *name[16];
int nk;
int nowait;

char buf[1024];
int color = 0x00C;

void menu()
{
   print2f("########################################\n\r");
   print2f("# ls   cd    pwd   cat   cp    mv   ps #\n\r");
   print2f("# mkdir rmdir creat rm chmod more grep #\n\r");
   print2f("# lpr (I/O and Pipe) :  >  >>  <  |    #\n\r");
   print2f("########################################\n\r");
}

int main(int argc, char *argv[])
{
  int pid, status, i;
  char buf[256], tbuf[256], *cp, *cq;

  signal(2, 1); /* ignore signal#2: Control-C interrupts */

  color = getpid() + 10;
  //printf("sh %d running\n", getpid());

  while (1)
  {
    printf("sh %d# ", getpid());

    //grabs le buf
    gets(buf);
    
    if (buf[0] == 0)
      continue;

    /* condition input string */
    cp = buf;
    while (*cp == ' ') // skip leading blanks
      cp++;

    cq = cp;
    while (*cq) // zero our trailing blanks
      cq++;
    cq--;

    //backtrack from last blank and 0 it out
    while (*cq == ' ')
    {
      *cq = 0;
      cq--;
    }

    //printf("input=%s\n", buf);

    if (strcmp(cp, "") == 0) // if nothing or a bunch of spaces
      continue;              // repeat the while loop

    //ensure tbuf and buf are the same, which should be cp
    // which should be process
    strcpy(tbuf, cp);
    strcpy(buf, tbuf);
    strcpy(tbuf, buf); 

    //CONSUME tbuf, and store names in name[]
    //numbers of names stored in nk
    nk = eatpath(tbuf, name);

    nowait = 0;

    //delet?
    //formerly attempting to & commands
    if (nk > 1)
    {
      if (strcmp(name[nk - 1], "&") == 0)
      {
        // must delete & from buf[ ]
        cp = buf;
        while (*cp)
          cp++; // cp to end to buf
        while (*cp != ' ')
          cp--;  // cp back to space
        *cp = 0; // delete & from buf[ ] end

        nk--;
        nowait = 1;
      }
    }

    if (strcmp(name[0], "cd") == 0)
    {
      if (name[1] == 0)
        chdir("/");
      else
        chdir(name[1]);
      continue;
    }

    if (strcmp(name[0], "pwd") == 0)
    {
      pwd();
      continue;
    }

    if (strcmp(name[0], "?") == 0 || strcmp(name[0], "help") == 0)
    {
      menu();
      continue;
    }

    if (strcmp(name[0], "logout") == 0)
    {
      print2f("##########################################\n\r");
      print2f("*************   Goodbye\007!   **************\n\r");
      print2f("##########################################\n\r");
      chdir("/");
      exit(0);
    }

    if (strcmp(name[0], "exit") == 0)
    {
      exit(0);
      continue;
    }

    printf("parent sh %d: fork a child\n", getpid());

    pid = fork(); /* sh forks child */

    if (pid)
    { /* parent sh */

      if (nowait)
      { //do a process switch, and continue
        printf("parent sh %d: no wait for child\n", getpid());
        nowait = 0;
        tswitch();
        continue;
      }
      else
      { //wait child, then continue
        printf("parent sh %d: wait for child %d to die\n", getpid(), pid);
        pid = wait(&status);
        printf("sh %d: child %d exit status = %x\n", getpid(), pid, status);
        continue;
      }
    }
    else
    {
      
      printf("child sh %d running : cmd=%s\n", getpid(), buf);
      do_pipe(buf, 0);
    }
  }
}

// scan breaks up buf = "head | tail" into  "head"  "tail"
int scan(char *buf, char **tail)
{
  char *p;

  p = buf;
  *tail = 0;

  while (*p) // scan to buf end line
    p++;

  while (p != buf && *p != '|') // scan backward until |
    p--;

  if (p == buf) // did not see any |, so head=buf
    return 0;

  *p = 0;           // change | to NULL
  p++;              // move p right by 1
  while (*p == ' ') // skip over any blanks
    p++;

  *tail = p; // change tail pointer to p

  return 1; // head points at buf; return head
}

int do_pipe(char *buf, int *rpd)
{
  int hasPipe, pid;
  char *tail;
  int lpd[2];

  if (rpd)
  {
    // as writer on RIGHT side pipe
    //close  PipeReader cuz reading from stdin
    //close stdout cuz outputing with PipeWriter
    //duplicate PipeWriter for use now
    //close PipeWriter to open new PipeWriter
    close(rpd[0]);
    close(1);
    dup(rpd[1]);
    close(rpd[1]);
  }

  hasPipe = scan(buf, &tail);

  if (hasPipe)
  { // buf=head; tail->tailString
    if (pipe(lpd) < 0)
    { // create LEFT side pipe
      printf("proc %d pipe call failed\n", getpid());
      exit(1);
    }
    
    pid = fork();

    if (pid < 0)
    {
      printf("proc %d fork failed\n", getpid());
      exit(1);
    }

    if (pid)
    { // parent as reader on LEFT side pipe
      //Closing Left PipeWriter cuz writing with stdout
      //Close stdin, cuz reading from PipeWriter
      //Duplicate PipeReader for use
      //Close PipeReader to open new PipeReader
      close(lpd[1]);
      close(0);
      dup(lpd[0]);
      close(lpd[0]);
      printf("proc %d exec %s\n", getpid(), tail);
      command(tail);
    }
    else
    { // child gets LEFT pipe as its RIGHT pipe
      do_pipe(buf, lpd);
    }
  }
  else
  { // no pipe symbol in string must be the leftmost cmd
    command(buf);
  }
  return 1;
}

int command(char *s)
{
  char *name[16], tbuf[64];
  int i, j, nk, I;
  char cmdline[64];

  //copy s into tbuf
  //so it can be eaten for the path
  strcpy(tbuf, s);
  nk = eatpath(tbuf, name);

  //duplicate nk
  I = nk;

  // hunting for input redirect
  for (i = 0; i < nk; i++)
  {
    if (strcmp(name[i], "<") == 0)
    {
      printf("proc %d redirect input from %s\n", getpid(), name[i + 1]);
      if (I > i)
        I = i; // I = index of first < or > or >>
      if (name[i + 1] == 0)
      {
        printf("invalid < in command line\n\r");
        exit(1);
      }
      close(0);
      if (open(name[i + 1], 0) < 0)
      {
        printf("no such input file\n");
        exit(2);
      }
      break;
    }
  }
  //hunt for output redirect
  for (i = 0; i < nk; i++)
  {
    if (strcmp(name[i], ">") == 0)
    {
      printf("proc %d redirect outout to %s\n", getpid(), name[i + 1]);
      if (I > i)
        I = i; // I = index of first > or < or >>
      if (name[i + 1] == 0)
      {
        printf("invalid > in command line\n\r");
        exit(3);
      }
      close(1);
      open(name[i + 1], O_WRONLY | O_CREAT);
      break;
    }
  }
  //hunt for output append redirect
  for (i = 0; i < nk; i++)
  {
    if (strcmp(name[i], ">>") == 0)
    {
      printf("proc %d append output to %s\n", getpid(), name[i + 1]);
      if (I > i)
        I = i;
      if (name[i + 1] == 0)
      {
        printf("invalid >> in command line\n\r");
        exit(4);
      }

      close(1);
      //open the string after >> for append
      open(name[i + 1], O_WRONLY | O_CREAT | O_APPEND); 
      break;
    }
  }

  //strcpy first command into cmdline
  strcpy(cmdline, name[0]);
  
  //copy until first io redirect
  for (j = 1; j < I; j++)
  {
    strcat(cmdline, " ");
    strcat(cmdline, name[j]);
  }
  /********* must write to 2 for correct output redirect **********/
  if (getpid() < 9)
  {
    if (exec(cmdline) < 0)
      exit(1);
  }

  while (1)
  {
    printf("%d : enter a key : ", getpid());
    getc();
  }

  return 1;
}
