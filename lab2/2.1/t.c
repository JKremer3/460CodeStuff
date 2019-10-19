typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/*
UART0 base address: 0x101f1000;
UART1 base address: 0x101f2000;
UART2 base address: 0x101f3000;
UART3 base address: 0x10009000;

// flag register at 0x18
//  7    6    5    4    3    2   1   0
// TXFE RXFF TXFF RXFE BUSY
// TX FULL : 0x20
// TX empty: 0x80
// RX FULL : 0x40
// RX empty: 0x10
// BUSY=1 :  0x08
*/

int N;
int v[] = {1,2,3,4,5,6,7,8,9,10};
int sum;

char *tab = "0123456789ABCDEF";

//#include "string.c"
#include "uart.c"

UART *up;

int strcmp(char *a, char *b)
{
    int i; 
    for (i = 0; a[i] && b[i]; ++i) 
    { 
        /* If characters are same or inverting the  
           6th bit makes them same */
        if (a[i] == b[i] || (a[i] ^ 32) == b[i]) 
           continue; 
        else
           break; 
    } 
    /* Compare the last (or first mismatching in  
       case of not same) characters */
    if (a[i] == b[i]) 
        return 0; 
  
    // Set the 6th bit in both, then compare 
    if ((a[i] | 32) < (b[i] | 32))  
        return -1; 
    return 1; 
}


int main()
{
  int i;
  int size = sizeof(int);
  char string[32]; 
  char line[128]; 

  N = 10;

  uart_init();
  for(i = 0; i < 4; i++)
  {
    up = &uart[i];
    fuprintf(up, "%s","enter a line from this UART : ");
    ugets(up, string);
    fuprintf(up, " ECHO : %s\n", string);
  }

  uprints(up, "Enter lines from UART terminal, enter quit to exit\n\r");
  
  while(1){
    ugets(up, string);
    uprints(up, "    ");
    if (strcmp(string, "quit")==0)
       break;
    uprints(up, string);  uprints(up, "\n\r");
  }


  uprints(up, "Compute sum of array\n\r");
  sum = 0;
  for (i=0; i<N; i++)
    sum += v[i];
  uputc(up, (sum/10)+'0'); uputc(up, (sum%10)+'0');
  uprints(up,"\n\rEND OF RUN\n\r");
}