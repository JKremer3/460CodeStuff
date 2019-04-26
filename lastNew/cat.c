

#include "ucode.c"

char *cp, mytty[64];

char realname[64], linkname[64];
char newline = '\n';
char CR = '\r';

int main(int argc, char *argv[])
{
	int fd, i, n;
	char buf[1244], dummy;
	char *cp;

	print2f("Now Running Cat \n\r");

	fd = 0;
	//if theres more than one arg, then attempts to open the file 
	if (argc > 1)
	{
		fd = open(argv[1], O_RDONLY);
		if (fd < 0)
		{
			printf("cat %s error\n", argv[1]);
			exit(0);
		}
	}

	//cat from stdin
	if (argc < 2)
	{	
		//while typing in a line
		while (gets(buf))
		{	
			//print the line, then format
			print2f(buf);
			print2f("\n\r");
		}
		exit(0);
	}

	//catting a file
	//while youre still reading from a file --
	while ((n = read(fd, buf, 1024)))
	{
		buf[n] = 0;
		cp = buf;
		
		//if theres a file
		if (fd)
		{
			//for n characters
			for (i = 0; i < n; i++)
			{
				//writes to stdout
				write(1, &buf[i], 1);
				if (buf[i] == '\n')
				//writes \n to error to not mess with spacing
					write(2, &CR, 1);
			}
		}

		else
		{ // fd=0 case
			cp = buf;
			//if *cp is '\r'
			if (*cp == '\r')
				//writes to stderr
				write(2, &newline, 1);
			//writes to stdout
			write(1, cp, 1);
		}
	}

	close(fd);
	exit(0);
}
