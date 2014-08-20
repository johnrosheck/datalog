/*
 * Source File : axon.c
 * Begun : May 9, 2006
 * Latest Revision : June 14, 2014
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
#include <stdlib.h>
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

int get_args(int argc, char **argv);
/* globals set by get_args */
char *progname = NULL;
char *log_filepath = "logfile.txt";
char *prog_filepath = NULL;
char *exec_filepath = NULL;
char *baud_given = NULL;
char *dev_name = "/dev/ttyS1"; /* the current default */
int verbose = 0;
int noninteractive = 0; /* default to interactive */

int main(int argc, char **argv);
void show_usage(void);
int auto_download_file(char *filename);
int do_remote_exec(char *hexaddr);

void leave_gracefully(int s_num) {
  /* int i; */
  printf("%s: exiting on signal : %d\n", progname, s_num);
  /* restore original term settings */
  restore_console_io();
  restore_serial_io(fd);
  exit(10);
}

int main(int argc, char **argv) {
  int i, result;

  if (get_args(argc, argv) != 0) {
    show_usage();
    exit(0);
  }
  
  /* handle some common signals */
  signal(SIGHUP, SIG_IGN);
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);

  if (log_filepath != NULL) {
    setup_logging(log_filepath);
  } else {
    setup_logging("logfile.txt");
  }
  if (dev_name != NULL) {
    /* first command line name is this, second is the device to use */
    if (verbose > 0) {
      printf("trying to access device \"%s\"...\n", dev_name);
    }
    if ((fd = open(dev_name, O_RDWR|O_NONBLOCK)) > 0) { /* got it */
      if (verbose > 0) {
        printf("opened \"%s\" as r/w, non-block file...\n", dev_name);
      }
      /* get some info on it */
      start_serial_io(fd, baud_given);
      /* display line settings */
      ioctl(fd, TIOCSERGETLSR, &result); /* read line status */
      if (verbose > 0) {
        show_flags_by_mask(line_names, MAXLINENAMES, (tcflag_t) result);
      }
      i = get_mlines(fd);
      show_flags_by_mask(line_names, MAXLINENAMES, (tcflag_t) i);
      /* read(STDIN, &c, 1); * wait for a character * */
      i |= DTR;
      if (verbose > 1) {
        printf("setting DTR\n");
      }
      i = put_mlines(fd, i);
      show_flags_by_mask(line_names, MAXLINENAMES, (tcflag_t) i);
      /* read(STDIN, &c, 1); * wait for a character * */
      /* change terminal to nowait i/o */
      if (noninteractive == 1) {
        freopen("/dev/null", "r", stdin);
        if (log_filepath == NULL) {
          freopen("/dev/null", "w", stdout);
          freopen("/dev/null", "w", stderr);
        } else {
          freopen(log_filepath, "w", stdout);
          freopen(log_filepath, "w", stderr);
        }
        i = 0;
        if (prog_filepath != NULL) { /* download this and run it */
          if (auto_download_file(prog_filepath) == 0) {
            printf("file downloaded ok\n");
            if (do_remote_exec("2000") == 0) {
              printf("remote exec succeeded\n");
              i = 1;
            } else {
              printf("remote exec failed\n");
              /* should fail out of program */
            }
          } else {
            printf("file download failed\n");
            /* should fail out of program */
          }
        }
        if ((i == 1) && (exec_filepath != NULL)) {
          restore_serial_io(fd);
          close(fd); /* have to do this first */
          system(exec_filepath);
          exit(0);
        }
      } else {
        printf("press F1 to view system help\n");
        start_console_io();
        /* do something */
        if (prog_filepath != NULL) { /* download this and run it */
          if (auto_download_file(prog_filepath) == 0) {
            printf("file downloaded ok\n");
            if (do_remote_exec("2000") == 0) {
              printf("remote exec succeeded\n");
            } else {
              printf("remote exec failed\n");
            }
          } else {
            printf("file download failed\n");
          }
        }
        do_run_loop();
        restore_console_io();
      }
      /* printf("reseting dtr\n"); */
      /* i = clr_dtr(fd); */
      show_flags_by_mask(line_names, MAXLINENAMES, (tcflag_t) i);
      /* read(STDIN, &c, 1); * wait for a character */
      restore_serial_io(fd);
      close(fd);
    } else { /* couldn't open */
      printf("couldn't open \"%s\", error = %s\n", dev_name, strerror(errno));
    }
  }
  exit(0);
}

int do_remote_exec(char *hexaddr) {
  char *addr;
  char *ematch, *p;
  char c;
  int i, maxtries;

  if ((hexaddr == NULL) || (*hexaddr == 0)) {
    addr = "2000";
  } else {
    addr = hexaddr;
  }
  /* dump anything in the buffer */
  while (read_ch(fd, &c) == 1) write(STDOUT, &c, 1); /* dump buffer */

  c = 'g'; /* send go char return */
  write(fd, &c, 1);
  while (get_schar(&c) == 1) write(STDOUT, &c, 1); /* dump buffer */
  i = 0;
  while (addr[i] != 0) {
    c = addr[i]; /* send go char return */
    write(fd, &c, 1);
    while (get_schar(&c) == 1) write(STDOUT, &c, 1); /* dump buffer */
    i++;
  }
  c = 13; /* send go char return */
  write(fd, &c, 1);
  /*  p[0] = c; p[1] = 0;
    printf("sending \"%s\"\n", p); */
  ematch = "Executing program.....";
  p = ematch;
  i = 0;
  maxtries = 100;
  while (i == 0) {
    if (get_schar(&c) == 1) {
      maxtries = 100;
      write(STDOUT, &c, 1); /* dump buffer */
      if (*p == c) {
        p++;
        if (*p == 0) { /* at end */
          return(0);
        }
      }
    } else {
      if (maxtries == 0) { /* fail */
        printf("error getting execute string\n");
        return(1);
      } else {
        maxtries--;
      }
    }
  }
  return(0);
}

int auto_download_file(char *filename) {
  FILE *fp;
  
  if (verbose > 0) {
    printf("auto download file \"%s\"\n", filename);
  }
  if ((fp = fopen(filename, "r")) != NULL) {
    /* ignore for now */
    if (verbose > 0) {
      printf("opened file \"%s\"\n", filename);
    }
    xfer_hex_file(fp);
    fclose(fp);
    return(0);
  } else {
    printf("error opening file \"%s\" - %s\n", filename, strerror(errno));
  }
  return(-1);
}

int get_args(int argc, char **argv) {
  int i;
  char *p;
  char *single_flags="vVhHnN";

  if (argc < 2) return(0);
  progname = *(argv + 0); /* record pointer to program name */
  i = 1;
  while (i < argc) {
    p = *(argv + i);
    if (p == NULL) return(-1);
    if (*p == 0) return(-1);
    if (*p == '-') { /* valid option flag */
      p++; /* point to next character */
      if (strchr(single_flags, (int) *p) == NULL) { /* not a toggle flag */
        i++; /* next arg index */
        if (i >= argc) {
          printf("%s: missing option argument\n", *(argv + 0));
          return(-1);
        }
      }
      switch (*p) {
      case 'h':
      case 'H':
        return(-1); /* force printing of usage */
      case 'v':
      case 'V': /* print extra stuff */
        verbose = 1;
        break;
      case 'L':
      case 'l':
        log_filepath = *(argv + i);
        break;
      case 'b':
      case 'B':
        baud_given = *(argv + i);
        break;
      case 'd':
      case 'D':
        dev_name = *(argv + i);
        break;
      case 'p':
      case 'P':
        prog_filepath = *(argv + i);
        break;
      case 'x':
      case 'X':
        exec_filepath = *(argv + i);
        break;
      case 'n':
      case 'N':
        noninteractive = 1;
        break;
      default:
        printf("%s: unknown option \"%s\"\n", *(argv + 0), p);
        return(-1);
        break;
      }
    } else { /* not a valid option flag, must be extra stuff */
      if (argc == i) {
        return(0);
      } else {
        printf("%s: extra or missing options\n", *(argv + 0));
        return(-1);
      }
    }
    i++; /* next arg */
  }
  /* printf("should never get here in get_args\n"); */
  return(0);
}

void show_usage(void) {
  printf("usage: %s [options]\n", progname);
  printf("  -l log_filepath (default is \"%s\")\n", log_filepath);
  printf("  -d device_name (default is \"%s\")\n", dev_name);
  printf("  -b baud_selection (only regular values allowed, i.e.:\n");
  printf("                      1200,2400,4800,9600,19200)\n");
  printf("  -p microprog = download and execute contents of file\n");
  printf("                 path specified by \"microprog\"\n");
  printf("  -x execprog = exec the program given by the path\n");
  printf("                \"execprog\" and pass it the same -l and\n");
  printf("                -d parameters\n");
  printf("  -u - unlink flag = force stdio to the bit bucket\n");
  printf("  -n - noninteractive flag = no stdio to console\n");
  printf("  -v - verbose flag = print debug info\n");
}
