/*
 * Source File : interactive.c
 * Begun : May 9, 2006
 * Latest Revision : July 22, 2014
 * Version : 1.00
 *
 * Copyright (C) 2006-2014 John B. Rosheck, Jr.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
/* #include <pwd.h> */
/* #include <sys/times.h> */
/* #include <sys/stat.h> */
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>

#include "termnames.h"
#include "tnames.h"
#include "mlines.h"
#include "control_io.h"
#include "interactive.h"

#define STDIN STDIN_FILENO
#define STDOUT STDOUT_FILENO

int fd;

/* #define BS_CHAR 0x08 */
#define BS_CHAR 0x7f
#define EOL_CHAR 0x0a
#define ESC_CHAR 0x1b

void display_char(char c) {
  int i, dop=0;
  char sbuf[40];

  i = c; i &= 0xff;
  /* test filter for i */
  /* test display */
  if (dop) {
    if (i < ' ') {
      sprintf(sbuf, "<%d>", i);
      write(STDOUT, sbuf, strlen(sbuf));
    }
  }
  if (c != 13) write(STDOUT, &c, 1);
}

/* escape decoder - first char is the esc char (27),
   later codes are :
     91, 91, 65 = F1
     91, 91, 66 = F2
     91, 91, 67 = F3
     91, 91, 68 = F4
     91, 91, 69 = F5
     91, 49, 55 = F6
     91, 49, 56 = F7
     91, 49, 57 = F8
     91, 50, 48 = F9
     91, 50, 49 = F10
     91, 50, 51 = F11
     91, 50, 52 = F12
 */
int decode_esc(int *res) {
  int j, k, l;
  char c;
  /* look for more stuff */
  if (read_ch(STDIN, &c) == 1) {
    j = c; j &= 0xff;
    if (read_ch(STDIN, &c) == 1) {
      k = c; k &= 0xff;
      if (read_ch(STDIN, &c) == 1) { /* this has to be the max size */
        l = c; l &= 0xff;
        if ((j == 91) && (k == 91)) { /* function keyS F1 to F5 */
          if (l == 65) return(1);
          else if (l == 66) return(2);
          else if (l == 67) return(3);
          else if (l == 68) return(4);
          else if (l == 69) return(5);
          else {
            printf("ESC string run with %d, %d, %d\n", j, k, l);
          }
        } else if ((j == 91) && (k == 49)) { /* F6 to F8 */
          if (l == 55) return(6);
          else if (l == 56) return(7);
          else if (l == 57) return(8);
          else {
            printf("ESC string run with %d, %d, %d\n", j, k, l);
          }
        } else if ((j == 91) && (k == 50)) { /* F9 to F12 */
          if (l == 48) return(9);
          else if (l == 49) return(10);
          else if (l == 51) return(11);
          else if (l == 52) return(12);
          else {
            printf("ESC string run with %d, %d, %d\n", j, k, l);
          }
        } else {
          printf("ESC string run with %d, %d, %d\n", j, k, l);
        }
      } else {
        printf("ESC string run with %d, %d\n", j, k);
      }
    } else {
      printf("ESC string run with %d\n", j);
    }
  }
  return(0);
}


/* read from the standard input, copy to std out, record as appropiate */
int get_input_line(char *buf, int maxsize) {
  int cnt=0, running=1, i;
  char sbuf[10];
  char c, b;
  buf[cnt] = 0;
  while (running) {
    if (read_ch(STDIN, &c) == 1) { /* got a char */
      i = c;
      if ((i >= ' ') && (i < 0x7f)) { /* displayable */
        buf[cnt] = c; cnt++; /* add char */
        buf[cnt] = 0; /* reterminate */
        write(STDOUT, &c, 1);
      } else { /* non-displayable */
        if (i == BS_CHAR) { /* backspace */
          if (cnt > 0) {
            cnt--;
            buf[cnt] = 0; /* reterminate */
            /* write(STDOUT, &c, 1); */
            b = 8; write(STDOUT, &b, 1);
            b = ' '; write(STDOUT, &b, 1);
            b = 8; write(STDOUT, &b, 1);
          }
        } else if (c == EOL_CHAR) { /* end of line reached */
          return(0);
        } else { /* something to dump */
          if (c == ESC_CHAR) { /* bomb out if single */
            if (read_ch(STDIN, &c) != 1) { /* single hit */
              running = 0;
            } else { /* something behind the escape - dump all */
              while (read_ch(STDIN, &c) == 1);
              running = 0;
            }
          } else {
            i = c; i &= 0xff;
            sprintf(sbuf, "<%d>", i);
            write(STDOUT, sbuf, strlen(sbuf));
          }
        }
      }
    }
  }
  return(-1);
}

#define MAXWAITTIME 300000
int get_schar(char *p) {
  char c;
  int notdone=1;
  clock_t scnt, tcnt;
  scnt = clock();
  while (notdone) {
    if (read_ch(fd, &c) == 1) {
      *p = c; /* pass the character back */
      return(1); /* got a character */
    }
    tcnt = clock() - scnt;
    if (tcnt > MAXWAITTIME) {
      notdone = 0;
    }
  }
  return(0);
}

int new_get_prompt(void) {
  char c;
  int nd=1;

  /* wait for response */
  while ((nd == 1) && (get_schar(&c) == 1)) {
    write(STDOUT, &c, 1); /* transfer to the screen */
    if ((c != 13) && (c != 10)) { /* this is first real */
      nd = 0;
    }
  }
  if (c == 'P') {
    if (get_schar(&c) == 1) {
      write(STDOUT, &c, 1);
      if (c == 'U') {
        if (get_schar(&c) == 1) {
          write(STDOUT, &c, 1);
          if (c == '>') {
            return(1);
          } else { printf("not a >\n"); }
        } else { printf("no char 4\n"); }
      } else { printf("not a U\n"); }
    } else { printf("no char 3\n"); }
  } else { printf("not a P\n"); }
  return(0);
}

int get_prompt(void) {
  char c;

  /* wait for response */
  if (get_schar(&c) == 1) {
    write(STDOUT, &c, 1); /* transfer to the screen */
    if ((c == 13) || (c == 10)) { /* this is first */
      if (get_schar(&c) == 1) {
        write(STDOUT, &c, 1);
        if (c == 'P') {
          if (get_schar(&c) == 1) {
            write(STDOUT, &c, 1);
            if (c == 'U') {
              if (get_schar(&c) == 1) {
                write(STDOUT, &c, 1);
                if (c == '>') {
                  return(1);
                } else { printf("not a >\n"); }
              } else { printf("no char 4\n"); }
            } else { printf("not a U\n"); }
          } else { printf("no char 3\n"); }
        } else { printf("not a P\n"); }
      } else { printf("no char 2\n"); }
    } else { printf("not a CR\n"); }
  } else { printf("no char 1\n"); }
  return(0);
}

int xfer_hex_file(FILE *fp) {
  char lbuf[512];
  char c, p[2];
  int notdone=1,cnt=0, i, t, endflag=0;

  while (read_ch(fd, &c) == 1) write(STDOUT, &c, 1); /* dump buffer */
  c = 13; /* send carriage return */
  write(fd, &c, 1);
  while (notdone) {
    if (new_get_prompt() != 1) { /* no prompt from micro */
      if (endflag == 0) {
        printf("error - no prompt from micro\n");
        return(-1);
      }
    }
    if (fgets(lbuf, 511, fp) != NULL) {
      cnt++;
      /* send the string to the unit */
      for (i=0;lbuf[i]!=0;i++) {
        t =  lbuf[i];
        /* test for char in range */
        if ( ((t >= '0') && (t <= '9')) || ((t >= 'A') && (t <= 'F')) || (t == ':') ) {
          write(fd, &(lbuf[i]), 1); /* send the character */
          /* wait for the echo */
          if (get_schar(&c) != 1) {
            printf("time out error\n");
            return(-1);
          }
          write(STDOUT, &c, 1); /* copy to screen */
        } else { /* this better be the end of the line */
          if ((t == 13) || (t == 10)) {
            /* ignore or what */
          } else {
            p[0] = t; p[1] = 0;
            printf("bad char \"%s\" code %d\n", p, t);
            return(-1);
          }
        }
      }
      /* test for end of file marker */
      if (strlen(lbuf) > 8) { /* line was big enough */
        if ((lbuf[7] == '0') && (lbuf[8] == '1')) {
          /* printf("reached last record\n"); */
          endflag = 1;
        }
      }
    } else {
      if (endflag == 0) {
        printf("error reading file on line %d\n", cnt);
        return(-1);
      } else {
        notdone = 0;
      }
    }
  }
  return(0);
}

void load_text_file(void) {
  char buf[2000];
  char *label="  Enter file name : ";
  FILE *fp;
  
  buf[0] = 0;
  /* printf("  Enter file name : "); */
  write(STDOUT, label, strlen(label));
  if (get_input_line(buf, 1999) == 0) { /* got something */
    /* printf("\ngot file \"%s\"\n", buf); */
    if ((fp = fopen(buf, "r")) != NULL) {
      /* ignore for now */
      printf("\nopened file \"%s\"\n", buf);
      xfer_hex_file(fp);
      fclose(fp);
      return;
    } else {
      printf("error opening file \"%s\"\n", buf);
    }
  }
  printf("error on input file \"%s\"\n", buf);
}

void show_char_set(void) {
  unsigned char c;
  int i;
  int j;
  j = 0;
  for (i=0;i<256;i++) {
    c = i;
    write(STDOUT, &c, 1);
    j++;
    if (j == 16) {
      c = 10;
      write(STDOUT, &c, 1);
      j = 0;
    }
  }
}

void show_all_lines(void) {
  if (get_dtr(fd)) printf("DTR=1,");
  else printf("DTR=0,");
  if (get_rts(fd)) printf("RTS=1,");
  else printf("RTS=0,");
  if (get_dsr(fd)) printf("DSR=1,");
  else printf("DSR=0,");
  if (get_cts(fd)) printf("CTS=1,");
  else printf("CTS=0,");
  if (get_dcd(fd)) printf("DCD=1\n");
  else printf("DCD=0\n");
}

/* ************************* run loop routines *************************** */
void do_run_loop(void) {
  int running=1;
  int i, j, dump=0;
  char c, sbuf[50];
  sprintf(sbuf,"starting loop...\n");
  write(STDOUT, sbuf, strlen(sbuf));
  while (running) {
    /* check for any input */
    /* while ((i=read_ch(fd, &c)) == 1) { * got a character */
    if ((i=read_ch(fd, &c)) == 1) { /* got a character */
      display_char(c);
    }
    /* check for any key hit */
    if (read_ch(STDIN, &c) == 1) {
      i = c; i &= 0xff;
      if (i == 27) {
        if ((j=decode_esc(&i)) > 0) { /* got a good decode */
          switch (j) {
            case 1: /* f1 pressed */
              /* this is the interface help screen */
              printf("\n*   Interface help screen\n");
              printf("* F1 = help screen           F5 : Set DTR = 1\n");
              printf("* F2 = hex file download     F6 : Set DTR = 0\n");
              printf("* F3 = show character set    F7 : Set RTS = 1\n");
              printf("* F4 = exit                  F8 : Set RTS = 0\n");
              break;
            case 2: /* f2 pressed */
              /* this is the hex file download screen */
              /* printf("F2 pressed\n"); */
              load_text_file();
              break;
            case 3: /* f3 pressed */
              printf("* F3 pressed\n");
              show_char_set();
              break;
            case 4: /* f4 pressed */
              running = 0;
              break;
            case 5: /* f5 pressed */
              printf("* setting DTR=1 : lines status ");
              set_dtr(fd);
              show_all_lines();
              break;
            case 6: /* f6 pressed */
              printf("* setting DTR=0 : lines status ");
              clr_dtr(fd);
              show_all_lines();
              break;
            case 7: /* f7 pressed */
              printf("* setting RTS=1 : lines status ");
              set_rts(fd);
              show_all_lines();
              break;
            case 8: /* f8 pressed */
              printf("* setting RTS=0 : lines status ");
              clr_rts(fd);
              show_all_lines();
              break;
            case 9: /* f9 pressed */
              printf("* F9 pressed - sending ESC\n");
              c = 27;
              write(fd, &c, 1);
              break;
            case 10: /* f10 pressed */
              printf("* F10 pressed - sending ESC 0\n");
              c = 27;
              write(fd, &c, 1);
              c = 0x30;
              write(fd, &c, 1);
              break;
            case 11: /* f11 pressed */
              printf("* F11 pressed - sending ESC 1\n");
              c = 27;
              write(fd, &c, 1);
              c = 0x31;
              write(fd, &c, 1);
              break;
            case 12: /* f12 pressed */
              printf("F12 pressed\n");
              break;
            default:
              printf("oor %d pressed\n", j);
              break;
          }
        } else {
          write(fd, &c, 1);
        }
      } else {
        write(fd, &c, 1); /* send it */
        if (dump) {
          if ((i < ' ') || (i >= 0x7f)) {
            sprintf(sbuf, "<%d>", i);
            write(STDOUT, sbuf, strlen(sbuf));
          } else {
            write(STDOUT, &c, 1);
          }
        }
      }
    } else { /* no key hit -- scrub some cycles */
      usleep(100);
    }
  }
}
