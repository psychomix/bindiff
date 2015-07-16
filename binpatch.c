/* 
 * binpatch.c
 * Copyright (c) 2015 by Gergely Tamas <dice@psychomix.org>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <openssl/md5.h>

#include <ctype.h>

#include <string.h>

#define min(x,y) (x<y?x:y)

#define MAX_NUM_PATCHES 512

int die (char *msg) {
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

typedef struct {
  size_t s_pos;
  unsigned int s_byte, d_byte;
} patch_t;

int main (int argc, char **argv) {
  int fd_1, fd_2;
  struct stat sb_1, sb_2;
  unsigned char *addr_1, *addr_2, md5[16], dest[255], line[513], s_md5[33], d_md5[33];
  patch_t patch[MAX_NUM_PATCHES]; // max number of patches in file
  int i, j, k, l, m, cnt = 0;

  if (argc < 2) {
    printf ("usage: %s file patch\n", *argv);
    exit (255);
  }

  fd_1 = open (argv[1], O_RDWR);
  if (fd_1 == -1) die ("unable to open argv[1]");

  if (fstat (fd_1, &sb_1) == -1) die ("unable to stat fd_1");

  addr_1 = mmap (NULL, sb_1.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_1, 0);
  if (addr_1 == MAP_FAILED) die ("mmap[1] failed");

  snprintf (dest, 254, "%s.orig", argv[1]);

  fd_2 = creat (dest, 0644);
  if (fd_2 == -1) die ("unable to create backup file");
 
  if (write (fd_2, addr_1, sb_1.st_size) != sb_1.st_size) die ("unable to create backup file");
  close (fd_2);

  MD5(addr_1, sb_1.st_size, md5);
  for (i=0,j=0; i<16; i++) { snprintf (&s_md5[j], 3, "%02x", md5[i]); j+=2; }

  fd_2 = open (argv[2], O_RDONLY);
  if (fd_2 == -1) die ("unable to open argv[2]");

  if (fstat (fd_2, &sb_2) == -1) die ("unable to stat fd_2");

  addr_2 = mmap (NULL, sb_2.st_size, PROT_READ, MAP_PRIVATE, fd_2, 0);
  if (addr_2 == MAP_FAILED) die ("mmap[2] failed");

  for (i=0,j=0,k=0; i < sb_2.st_size; i++) {
    if (addr_2[i] == '\r') continue;
    if (addr_2[i] == '#' || addr_2[i] == '\n' || i == sb_2.st_size-1) {
      while (j<i) { 
        if (addr_2[j] != '\n' && addr_2[j] != '\r') { 
          if (k || !isspace(addr_2[j])) { 
            if (isspace(addr_2[j]) && isspace(line[k-1])) { j++; continue; } // ignore multiple white-space characters
            line[k++] = addr_2[j++];
          } else j++;
        } else j++;
      }
      if (k) {
        line[k] = 0; k=0;
        for (l=0,m=0; line[l]; l++) {
          if (isspace(line[l])) {
            if (!m) {
              switch (l) {
                case  8: patch[cnt].s_pos = patch[cnt].s_byte = patch[cnt].d_byte = 0;
                         sscanf (line, "%08x %02x %02x", &patch[cnt].s_pos, &patch[cnt].s_byte, &patch[cnt].d_byte);
                         cnt ++;
                         if (cnt == MAX_NUM_PATCHES) die ("max number of possible patches reached, please recompile with a larger value");
                         break;

                case 32: if (memcmp (line, s_md5, 32) != 0) die ("md5 checksum does not match");
                         strncpy (d_md5, &line[33], 32);
                         break;
              }
            }
            m=l;
          }
        }
      }
      if (addr_2[i] == '#') { while (addr_2[i] != '\n' && i<sb_2.st_size) i++; j=i; }
    }
  }

  for (i=0; i<cnt; i++) {
    if (addr_1[patch[i].s_pos] == patch[i].s_byte) {
      printf ("%08x %02x %02x\n", patch[i].s_pos, patch[i].s_byte, patch[i].d_byte);
      addr_1[patch[i].s_pos] = patch[i].d_byte;
    } else {
      printf ("FAILED at %08x : wanted -> %02x , got -> %02x\n", patch[i].s_pos, addr_1[patch[i].s_pos], patch[i].s_byte);
    }
  }

  MD5(addr_1, sb_1.st_size, md5);
  for (i=0,j=0; i<16; i++) { snprintf (&s_md5[j], 3, "%02x", md5[i]); j+=2; }
  if (memcmp (d_md5, s_md5, 32) != 0) die ("patch failed, md5 checksum does not match");

  printf ("patch successful applied...\n");
}
