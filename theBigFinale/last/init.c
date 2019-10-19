#include "ucode.c"

int console;

int parent()
{
    int pid, status;
    while(1)
    {
        printf("INIT: wait for ZOMBIE child\n");
        pid = wait(&status);
        if(pid == console) //if console login process died
        {
            printf("INIT: forks a new console login\n");
            console = fork(); // fork a new console
            if(console)
                continue;
            else
                exec("login /dev/tty0"); //new console login process
        }
        printf("INIT: Buried an Orphan Child Proc %d\n", pid);
    }
}

int main(void)
{
    int in, out; // file descriptors for terminal I/O

    in = open("/dev/tty0", O_RDONLY); //file descriptor 0
    out = open("/dev/tty0", O_WRONLY); //for display to console

    printf("INIT: fork a login proc on console\n");
    console = fork();

    if(console)
        parent();
    else
        exec("login /dev/tty0");
        
}