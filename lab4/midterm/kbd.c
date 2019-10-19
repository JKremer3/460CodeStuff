
//#include "keymap"
#include "keymap2"

#define LSHIFT 0x12
#define RSHIFT 0x59
#define ENTER  0x5A
#define LCTRL  0x14
#define RCTRL  0xE014

#define ESC    0x76
#define F1     0x05
#define F2     0x06
#define F3     0x04
#define F4     0x0C

#define KCNTL 0x00
#define KSTAT 0x04
#define KDATA 0x08
#define KCLK  0x0C
#define KISTA 0x10

typedef volatile struct kbd{
  char *base;
  char buf[128];
  int head, tail;
  int data, room;
}KBD;

volatile KBD kbd;
int shifted = 0;
int release = 0;
int control = 0;

volatile int keyset;
int kputc(char);
extern int color;

int kbd_init()
{
  char scode;
  keyset = 1; // default to scan code set-1
  
  KBD *kp = &kbd;
  kp->base = (char *)0x10006000;
  *(kp->base + KCNTL) = 0x10; // bit4=Enable bit0=INT on
  *(kp->base + KCLK)  = 8;
  kp->head = kp->tail = 0;

  kp->data = 0;
  kp->room = 128;
   
  shifted = 0;
  release = 0;
  control = 0;

  kprintf("Detect KBD scan code: press the ENTER key : ");
  while( (*(kp->base + KSTAT) & 0x10) == 0);
  scode = *(kp->base + KDATA);
  kprintf("scode=%x ", scode);
  if (scode==0x5A)
    keyset=2;
  kprintf("keyset=%d\n", keyset);
}

void kbd_handler()
{
  u8 scode, c;

  KBD *kp = &kbd;
  
  scode = *(kp->base + KDATA);  // get scan code from KDATA reg => clear IRQ

  // printf("scanCode = %x\n", scode);



  if (scode == 0xF0){       // key release 
     release = 1;           // set flag
     return;
  }

  if(scode == 0x14)
  {
    control=!control;
    if(!control)
    {
      release = 0;
    }
    return;
  }

  if(control&&(scode==0x21)&&release)
  {
    kprintf("control c\n");
    release = 0;
    c = 0;
    return;
  }

  //adding EOF which is 0x04
  if(control&&(scode==0x23)&&release)
  {
    kprintf("control d\n");
    release = 0;
    // printf("c=%x %c\n", c, c);

    //manually add to buffer since control shuts it off
    c = 0x04;
    kprintf("c=%x %c\n", c, c);
    
    kp->buf[kp->head++] = c;
    kp->head %= 128;
    kp->data++; kp->room--;
    return;
  }
  
  if(scode == 0x12 || scode == 0x59) //shift press/release for right left shift
  {
    shifted = !shifted; //just invert the current bit
    if(!shifted) //if shift is released we count this as a total release to rescan
    {
      release = 0;
    }
    return;
  }
  
  if (release && scode){    // next scan code following key release
     release = 0;           // clear flag 
     return;
  }

  if (!shifted)            
     c = ltab[scode];
  else               // ONLY IF YOU can catch LEFT or RIGHT shift key
     c = utab[scode];
  

  //not saving to buffer if we have control pressed since its a command
  if(!control)
  {
    kprintf("%c", c);

    kp->buf[kp->head++] = c;
    kp->head %= 128;
    kp->data++; kp->room--;
    kwakeup(&kp->data);//only added line from book
  }

}

int kgetc()
{
  char c;
  KBD *kp = &kbd;
  while(kp ->data == 0)
    ksleep(&kp->data);

  lock();
  c = kp->buf[kp->tail++];
  kp->tail %= 128;
  kp->data--; kp->room++;
  unlock();
  return c;
}

int kgets(char s[ ])
{
  char c;
  KBD *kp = &kbd;
  
  while( (c = kgetc()) != '\r'){
    if (c=='\b'){
      s--;
      continue;
    }
    *s++ = c;
  }
  *s = 0;
  return strlen(s);
}
