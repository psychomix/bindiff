/* 
 * bindiff.c
 * Copyright (c) 2015 by Gergely Tamas <dice@psychomix.org>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <openssl/md5.h>

#define min(x,y) (x<y?x:y)

int die (char *msg) {
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

int main (int argc, char **argv) {
  int fd_1, fd_2;
  struct stat sb_1, sb_2;
  unsigned char *addr_1, *addr_2, md5[16];
  int i;

  if (argc < 3) {
    printf ("usage: %s file1 file2\n", *argv);
    exit (255);
  }

  fd_1 = open (argv[1], O_RDONLY);
  if (fd_1 == -1) die ("unable to open argv[1]");

  fd_2 = open (argv[2], O_RDONLY);
  if (fd_2 == -1) die ("unable to open argv[2]");

  if (fstat (fd_1, &sb_1) == -1) die ("unable to stat fd_1");

  if (fstat (fd_2, &sb_2) == -1) die ("unable to stat fd_2");

  addr_1 = mmap (NULL, sb_1.st_size, PROT_READ, MAP_PRIVATE, fd_1, 0);
  if (addr_1 == MAP_FAILED) die ("mmap[1] failed");

  addr_2 = mmap (NULL, sb_2.st_size, PROT_READ, MAP_PRIVATE, fd_2, 0);
  if (addr_2 == MAP_FAILED) die ("mmap[2] failed");

  MD5(addr_1, sb_1.st_size, md5);
  for (i=0; i<16; i++) {
    printf ("%02x", md5[i]);
  }
  printf (" ");

  MD5(addr_2, sb_2.st_size, md5);
  for (i=0; i<16; i++) {
    printf ("%02x", md5[i]);
  }
  printf ("\n");


  for (i=0; i<min(sb_1.st_size,sb_2.st_size); i++) {
    if (addr_1[i] != addr_2[i]) {
      printf ("%08x %02x %02x\n", i, addr_1[i], addr_2[i]);
    }
  }
}
