/*
 * Source File : control_io.c
 * Begun : November 24, 1997
 * Latest Revision : July 22, 2014
 * Version : 1.00
 *
 * Copyright (C) 1997-2014 John B. Rosheck, Jr.
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

#include "termnames.h"
#include "tnames.h"
/* #include "mlines.h" */
#include "control_io.h"

#define STDIN STDIN_FILENO
#define STDOUT STDOUT_FILENO

/* ************************ Initialize I/O stuff *************************** */
/* set up the serial port */
int init_serial_fd(int fd, struct termios *t, int baud_to_use);
/* set up the keyboard terminal */
int init_iterm_fd(int fd, struct termios *t);
int init_oterm_fd(int fd, struct termios *t);

/* this is used to restore setups on exiting */
struct termios def_serial;
struct termios orig_cons_in;
struct termios orig_cons_out;

struct termios new_serial;
struct termios new_cons_in;
struct termios new_cons_out;

int serial_fd; /* serial port file descriptor */
int con_in_fd; /* console (term) input descriptor */
int con_out_fd; /* console (term) output descriptor */

#define MAXLOGNAMELEN 1024
char logfilename[MAXLOGNAMELEN];
int use_logging;

void setup_logging(char *lname) {
  int i;
  i = strlen(lname);
  if ((i > 0) && (i < MAXLOGNAMELEN-1)) {
    printf("starting log file \"%s\"\n", lname);
    strcpy(logfilename, lname);
    use_logging = 1;
  } else {
    use_logging = 0;
  }
}

/* ************************ Initialize I/O stuff *************************** */

/* set up the serial port */
int init_serial_fd(int fd, struct termios *t, int baud_to_use) {
  int i;
  i = tcgetattr(fd, t);
  /* printf("serial tcgetattr returns %d\n", i); */
  if (use_logging) log_termios_settings("init serial termios :", logfilename, t);
  t->c_iflag &= ~(BRKINT|PARMRK|INPCK|IUCLC|IXON|IXOFF);
  t->c_iflag |= (IGNBRK | IGNPAR | IGNCR);
  /* t->c_oflag &= ~(ONOCR|OCRNL|ONLCR); */
  t->c_oflag &= ~(ONOCR);
  t->c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH);
  t->c_lflag |= (ISIG);
  t->c_cc[VMIN] = 0;
  t->c_cc[VTIME] = 0;
  t->c_cc[VSUSP] = 0; /* disable suspend */
  /* force baud rate to 9600 */
  t->c_cflag &= ~(BAUDMASK); /* zero out baud values */
  /* t->c_cflag |= (CS8 | B9600); * force to 9600 */
  t->c_cflag |= (CS8 | baud_to_use); /* force to 9600 */
  /* set it */
  i = tcsetattr(fd, TCSANOW, t);
  /* printf("result of new serial settings = %d\n", i); */
  if (use_logging) log_termios_settings("init serial termios :", logfilename, t);
  return(i);
}

/* set up the keyboard terminal */
int init_iterm_fd(int fd, struct termios *t) {
  int i;
  i = tcgetattr(fd, t);
  /* printf("terminal %d tcgetattr returns %d\n", fd, i); */
  if (use_logging) log_termios_settings("starting stdin termios ************* ", logfilename, t);
  t->c_iflag &= ~(BRKINT|PARMRK|INPCK|IUCLC|IXON|IXOFF);
  t->c_iflag |= (IGNBRK | IGNPAR);
  t->c_oflag &= ~(ONOCR);
  t->c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH);
  t->c_lflag |= (ISIG);
  t->c_cc[VMIN] = 0;
  t->c_cc[VTIME] = 0;
  t->c_cc[VSUSP] = 0; /* disable suspend */
  i = tcsetattr(fd, TCSANOW, t);
  /* printf("result of new terminal settings = %d\n", i); */
  if (use_logging) log_termios_settings("ending stdin termios *************** ", logfilename, t);
  return(i);
}

/* set up the keyboard terminal */
int init_oterm_fd(int fd, struct termios *t) {
  int i;
  i = tcgetattr(fd, t);
  /* printf("terminal %d tcgetattr returns %d\n", fd, i); */
  if (use_logging) log_termios_settings("starting stdout termios ************* ", logfilename, t);
  t->c_iflag &= ~(BRKINT|PARMRK|INPCK|IUCLC|IXON|IXOFF);
  t->c_iflag |= (IGNBRK | IGNPAR);
  t->c_oflag &= ~(ONOCR);
  t->c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH);
  t->c_lflag |= (ISIG | FLUSHO);
  t->c_cc[VMIN] = 0;
  t->c_cc[VTIME] = 0;
  t->c_cc[VSUSP] = 0; /* disable suspend */
  i = tcsetattr(fd, TCSANOW, t);
  /* printf("result of new terminal settings = %d\n", i); */
  if (use_logging) log_termios_settings("ending stdout termios *************** ", logfilename, t);
  return(i);
}

/* ******************** i/o store and restore routines ******************* */

int start_console_io(void) {
  int i;
  
  con_in_fd = STDIN;
  i = tcgetattr(con_in_fd, &orig_cons_in);
  /* printf("terminal stdin tcgetattr returns %d\n", i); */
  con_out_fd = STDOUT;
  i = tcgetattr(con_out_fd, &orig_cons_out);
  /* printf("terminal stdout tcgetattr returns %d\n", i); */
  init_iterm_fd(con_in_fd, &new_cons_in);
  init_oterm_fd(con_out_fd, &new_cons_out);
  return(0);
}

void restore_console_io(void) {
  int i;
  i = tcsetattr(con_out_fd, TCSANOW, &orig_cons_out); /* force original */
  /* printf("console out tcsetattr returns %d\n", i); */
  i = tcsetattr(con_in_fd, TCSANOW, &orig_cons_in); /* force original */
  /* printf("console in tcsetattr returns %d\n", i); */
}

int start_serial_io(int fd, char *given_baud) {
  int i, baud;

  serial_fd = fd;
  baud = B9600;
  i = tcgetattr(fd, &def_serial);
  printf("serial tcgetattr returns %d\n", i);
  if (given_baud != NULL) {
    if (strcmp(given_baud, "1200") == 0) {
      baud = B1200;
      printf("using baud = 1200\n");
    } else if (strcmp(given_baud, "2400") == 0) {
      baud = B2400;
      printf("using baud = 2400\n");
    } else if (strcmp(given_baud, "4800") == 0) {
      baud = B4800;
      printf("using baud = 4800\n");
    } else if (strcmp(given_baud, "9600") == 0) {
      baud = B9600;
      printf("using baud = 9600\n");
    } else if (strcmp(given_baud, "19200") == 0) {
      baud = B19200;
      printf("using baud = 19200\n");
    } else if (strcmp(given_baud, "38400") == 0) {
      baud = B38400;
      printf("using baud = 38400\n");
    } else if (strcmp(given_baud, "57600") == 0) {
      baud = B57600;
      printf("using baud = 57600\n");
    } else if (strcmp(given_baud, "115200") == 0) {
      baud = B115200;
      printf("using baud = 115200\n");
    } else if (strcmp(given_baud, "230400") == 0) {
      baud = B230400;
      printf("using baud = 230400\n");
    } else {
      printf("not a valid baud rate, using default of 9600\n");
      baud = B9600;
    }
  }
  init_serial_fd(fd, &new_serial, baud);
  return(0);
}

void restore_serial_io(int fd) {
  int i;
  if (fd != serial_fd) {
    printf("error - fd has changed, new %d, old %d\n", fd, serial_fd);
    return;
  } else {
    i = tcsetattr(fd, TCSANOW, &def_serial); /* force original one */
    /* printf("serial restore tcsetattr returns %d\n", i); */
  }
}
