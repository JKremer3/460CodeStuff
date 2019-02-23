// timer.c file
/***** timer confiuguration values *****/
#define CTL_ENABLE          ( 0x00000080 )
#define CTL_MODE            ( 0x00000040 )
#define CTL_INTR            ( 0x00000020 )
#define CTL_PRESCALE_1      ( 0x00000008 )
#define CTL_PRESCALE_2      ( 0x00000004 )
#define CTL_CTRLEN          ( 0x00000002 )
#define CTL_ONESHOT         ( 0x00000001 )

// timer register offsets from base address
/**** byte offsets *******
#define TLOAD   0x00
#define TVALUE  0x04y
#define TCNTL   0x08
#define TINTCLR 0x0C
#define TRIS    0x10
#define TMIS    0x14
#define TBGLOAD 0x18
*************************/
/** u32 * offsets *****/
#define TLOAD   0x0
#define TVALUE  0x1
#define TCNTL   0x2
#define TINTCLR 0x3
#define TRIS    0x4
#define TMIS    0x5
#define TBGLOAD 0x6

typedef volatile struct timer{
  u32 *base;            // timer's base address; as u32 pointer
  int tick, hh, mm, ss; // per timer data area
  char clock[16]; 
}TIMER;
volatile TIMER timer;  // 4 timers; 2 timers per unit; at 0x00 and 0x20

int kprintf(char *fmt, ...);
int kpchar(char, int, int);
int unkpchar(char, int, int);

void timer_init()
{
  int i;
  TIMER *tp;
  kprintf("timer_init()\n");
  tp = &timer;
  tp->base = (u32 *)0x101E2000; 
    
    //printf("%d: %x %x %x\n", i, tp->base, tp->base+TLOAD, tp->base+TVALUE);
  *(tp->base+TLOAD) = 0x0;   // reset
  *(tp->base+TVALUE)= 0xFFFFFFFF;
  *(tp->base+TRIS)  = 0x0;
  *(tp->base+TMIS)  = 0x0;
  *(tp->base+TLOAD) = 0x100;
  *(tp->base+TCNTL) = 0x62; //011-0000=|NOTEn|Pe|IntE|-|scal=00|32-bit|0=wrap|
  *(tp->base+TBGLOAD) = 0xF0000;

  tp->tick = tp->hh = tp->mm = tp->ss = 0;
  kstrcpy((char *)tp->clock, "00:00:00");
  
}

void timer_handler() 
{
    u32 ris, mis, value, load, bload, i;
    TIMER *t = &timer;
    // read all status info
    ris   = *(t->base+TRIS);
    mis   = *(t->base+TMIS);
    value = *(t->base+TVALUE);
    load  = *(t->base+TLOAD);
    bload = *(t->base+TBGLOAD);

    t->tick++; t->ss = t->tick;
    t->ss %= 60;
    if (t->ss==0){
      t->mm++;
      if ((t->mm % 60)==0){
        t->mm = 0;
	      t->hh++;
      }
    }
    for (i=0; i<8; i++){
      unkpchar(t->clock, 0, 70+i);
    }
    t->clock[7]='0'+(t->ss%10); t->clock[6]='0'+(t->ss/10);
    t->clock[4]='0'+(t->mm%10); t->clock[3]='0'+(t->mm/10);
    t->clock[1]='0'+(t->hh%10); t->clock[0]='0'+(t->hh/10);
 
    for (i=0; i<8; i++){
       kpchar(t->clock[i], 0, 70+i);
    }

    timer_clearInterrupt(0);
    return;
} 

void timer_start(int n) // timer_start(0), 1, etc.
{
  TIMER *tp = &timer;

  kprintf("timer_start %d base=%x\n", n, tp->base);
  *(tp->base+TCNTL) |= 0x80;  // set enable bit 7
}

int timer_clearInterrupt(int n) // timer_start(0), 1, etc.
{
  TIMER *tp = &timer;
  *(tp->base+TINTCLR) = 0xFFFFFFFF;
}

void timer_stop(int n) // timer_start(0), 1, etc.
{
  TIMER *tp = &timer;
  *(tp->base+TCNTL) &= 0x7F;  // clear enable bit 7
}
