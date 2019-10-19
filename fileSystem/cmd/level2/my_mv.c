#include "my_mv.h"
#include "my_cp.h"
#include "my_open.h"
#include "my_close.h"
#include "../level1/my_unlink.h"
#include "../level1/my_link.h"

/**** globals defined in main.c file ****/
MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;
char gpath[MAX_FILENAME_LEN];
char *name[MAX_COMPONENTS];
int n;
int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start;
char line[MAX_INPUT_LEN], cmd[32], pathname[MAX_FILENAME_LEN];

int my_mv(int argc, char *argv[])
{
	// argv[0] is src; argv[1] is dst (where we moving to!)
	if (argc < 2) {
		printf("my_mv: ERROR -- need two files to proceed\n");
		return -1;
	}
	char src[MAX_FILENAME_LEN], dst[MAX_FILENAME_LEN];
	strcpy(src, argv[0]); strcpy(dst, argv[1]);
	return run_mv(src, dst);
}

int run_mv(char *src, char *dst) {
	// 1. verify src exists; get its INODE in ==> you already know its dev
	int src_fd = run_open(src, 0);
	if (src_fd == -1) {
		printf("my_mv: ERROR -- %s doesn't exist\n", src);
		return -1;
	}
	// 2. check whether src is on the same dev as src
	int dev = running->fd[src_fd]->mptr->dev;

	// CASE 1: same dev; hard link dst with src (i.e. same INODE number)
	if (running->cwd->dev == dev) {  // I think this checks?
		if (run_link(src, dst) == -1) return -1;
	} else {
    	// CASE 2: not the same dev; cp src to dst
		if (run_cp(src, dst) == -1) return -1;
	}
	if (run_close(src_fd) == -1) return -1;
	// 3. unlink src (i.e. rm src name from its parent directory and reduce INODE's link count by 1).
	if (run_unlink(src) == -1) return -1;
	return 0;
}