/*
 * Source File : tnames.c
 * Begun : April 22, 1996
 * Latest Revision : July 22, 2014
 * Version : 1.00
 *
 * Copyright (C) 1996-2014 John B. Rosheck, Jr.
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
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "termnames.h"
#include "tnames.h"

void show_flags_by_mask(FlagNames *entries, int maxflags, tcflag_t flag) {
  int i, firstflag=0;
  /* printf("showing %08x\n", (int) flag); */
  for (i=0;i<maxflags;i++) {
    if (entries[i].mask & flag) { /* set */
      if (firstflag) {
        printf(", %s", entries[i].name);
      } else {
        printf("%s", entries[i].name);
        firstflag = 1;
      }
    }
  }
  printf("\n");
}

void fshow_flags_by_mask(FILE *f, FlagNames *entries, int maxflags, tcflag_t flag) {
  int i, firstflag=0;
  /* fprintf(f, "showing %08x\n", (int) flag); */
  for (i=0;i<maxflags;i++) {
    if (entries[i].mask & flag) { /* set */
      if (firstflag) {
        fprintf(f, ", %s", entries[i].name);
      } else {
        fprintf(f, "%s", entries[i].name);
        firstflag = 1;
      }
    }
  }
  fprintf(f, "\n");
}

void show_flags_by_value(FlagNames *entries, int maxflags, tcflag_t flag) {
  int i;
  /* printf(" showing %08x - ", (int) flag); */
  for (i=0;i<maxflags;i++) {
    if (entries[i].mask == flag) { /* set */
      printf("%s\n", entries[i].name);
      return;
    }
  }
  printf("out of range = %d\n", (int) flag);
}

void fshow_flags_by_value(FILE *f, FlagNames *entries, int maxflags, tcflag_t flag) {
  int i;
  /* printf(" showing %08x - ", (int) flag); */
  for (i=0;i<maxflags;i++) {
    if (entries[i].mask == flag) { /* set */
      fprintf(f, "%s\n", entries[i].name);
      return;
    }
  }
  printf("out of range = %d\n", (int) flag);
}

void dump_termios_settings(struct termios *t) {
  int i;
  /* dump input mode flags */
  printf("in flags: ");
  show_flags_by_mask(in_flags, MAXINFLAGS, t->c_iflag);  
  /* dump output mode flags */
  printf("out flags: ");
  show_flags_by_mask(out_flags, MAXOUTFLAGS, t->c_oflag);
  /* printf("output mode flags = %04x\n", (int)t->c_oflag); */
  /* dump control mode flags */
  printf("control flags: ");
  show_flags_by_mask(cntl_flags, MAXCNTLFLAGS, t->c_cflag);
  printf("baud rate : ");
  show_flags_by_value(baud_names, MAXBAUDNAMES, (t->c_cflag&BAUDMASK));
  /* printf("control mode flags = %04x\n", (int)t->c_cflag); */
  /* dump local mode flags */
  show_flags_by_mask(local_flags, MAXLOCALFLAGS, t->c_lflag);
  /* printf("local mode flags = %04x\n", (int)t->c_lflag); */
  /* show line discipline */
  switch(t->c_line) {
    case N_TTY:
      printf("TTY line discipline\n");
      break;
    case N_SLIP:
      printf("SLIP line discipline\n");
      break;
    case N_MOUSE:
      printf("MOUSE line discipline\n");
      break;
    case N_PPP:
      printf("PPP line discipline\n");
      break;
    default:
      printf("unknown line discipline %d\n", t->c_line);
      break;
  }
  return;
  /* show control characters in use */
  for (i=0;i<NCCS;i++) {
    if (i < MAXCCNAMES) {
      printf("Ccode[%s] = %02x\n", cntl_names[i].name, (int)t->c_cc[i]);
    } else {
      printf("Ccode[%d] = %02x\n", i, (int)t->c_cc[i]);
    }
  }
}

void log_termios_settings(char *hdr, char *lname, struct termios *t) {
  int i;
  FILE *f;
  if ((f = fopen(lname, "a")) != NULL) {
    fprintf(f, "%s\n", hdr);
    /* dump input mode flags */
    fprintf(f, "in flags: ");
    fshow_flags_by_mask(f, in_flags, MAXINFLAGS, t->c_iflag);  
    /* dump output mode flags */
    fprintf(f, "out flags: ");
    fshow_flags_by_mask(f, out_flags, MAXOUTFLAGS, t->c_oflag);
    /* printf("output mode flags = %04x\n", (int)t->c_oflag); */
    /* dump control mode flags */
    fprintf(f, "control flags: ");
    fshow_flags_by_mask(f, cntl_flags, MAXCNTLFLAGS, t->c_cflag);
    fprintf(f, "baud rate : ");
    fshow_flags_by_value(f, baud_names, MAXBAUDNAMES, (t->c_cflag&BAUDMASK));
    /* printf("control mode flags = %04x\n", (int)t->c_cflag); */
    /* dump local mode flags */
    fshow_flags_by_mask(f, local_flags, MAXLOCALFLAGS, t->c_lflag);
    /* printf("local mode flags = %04x\n", (int)t->c_lflag); */
    /* show line discipline */
    switch(t->c_line) {
      case N_TTY:
        fprintf(f, "TTY line discipline\n");
        break;
      case N_SLIP:
        fprintf(f, "SLIP line discipline\n");
        break;
      case N_MOUSE:
        fprintf(f, "MOUSE line discipline\n");
        break;
      case N_PPP:
        fprintf(f, "PPP line discipline\n");
        break;
      default:
        fprintf(f, "unknown line discipline %d\n", t->c_line);
        break;
    }
    fclose(f);
    return;
    /* show control characters in use */
    for (i=0;i<NCCS;i++) {
      if (i < MAXCCNAMES) {
        fprintf(f, "Ccode[%s] = %02x\n", cntl_names[i].name, (int)t->c_cc[i]);
      } else {
        fprintf(f, "Ccode[%d] = %02x\n", i, (int)t->c_cc[i]);
      }
    }
  } else {
    printf("error opening log file \"%s\"\n", strerror(errno));
  }
}

FlagNames in_flags[MAXINFLAGS] = {
  { "IGNBRK", IGNBRK },
  { "BRKINT", BRKINT },
  { "IGNPAR", IGNPAR },
  { "PARMRK", PARMRK },
  { "INPCK",  INPCK },
  { "ISTRIP", ISTRIP },
  { "INLCR",  INLCR },
  { "IGNCR",  IGNCR },
  { "ICRNL",  ICRNL },
  { "IUCLC",  IUCLC },
  { "IXON",   IXON },
  { "IXANY",  IXANY },
  { "IXOFF",  IXOFF },
  { "IMAXBEL", IMAXBEL }
};

FlagNames out_flags[MAXOUTFLAGS] = {
  { "OPOST", OPOST },
  { "OLCUC", OLCUC },
  { "ONLCR", ONLCR },
  { "OCRNL", OCRNL },
  { "ONOCR", ONOCR },
  { "ONLRET", ONLRET },
  { "OFILL", OFILL },
  { "OFDEL", OFDEL },
  { "NLDLY", NLDLY }, /* NLDLY IS 0 OR 1 */
  { "CRDLY", CRDLY }, /* CRDLY IS 0, 1, 2 OR 3 */
  { "TABDLY", TABDLY }, /* TABDLY IS 0, 1, 2, OR 3 */
  { "BSDLY", BSDLY }, /* BSDLY IS 0 OR 1 */
  { "VTDLY", VTDLY }, /* VTDLY IS 0 OR 1 */
  { "FFDLY", FFDLY }  /* FFDLY IS 0 OR 1 */
};

FlagNames cntl_flags[MAXCNTLFLAGS] = {
  { "CSTOPB", CSTOPB },
  { "CREAD", CREAD },
  { "PARENB", PARENB },
  { "PARODD", PARODD },
  { "HUPCL", HUPCL },
  { "CLOCAL", CLOCAL },
  { "CBAUDEX", CBAUDEX },
  { "CRTSCTS", CRTSCTS }
};

FlagNames csize_names[MAXCSIZENAMES] = {
  { "5", CS5 },
  { "6", CS6 },
  { "7", CS7 },
  { "8", CS8 }
};

FlagNames baud_names[MAXBAUDNAMES] = {
  { "50", B50 },
  { "75", B75 },
  { "110", B110 },
  { "134", B134 },
  { "150", B150 },
  { "200", B200 },
  { "300", B300 },
  { "600", B600 },
  { "1200", B1200 },
  { "1800", B1800 },
  { "2400", B2400 },
  { "4800", B4800 },
  { "9600", B9600 },
  { "19200", B19200 },
  { "38400", B38400 },
  { "57600", B57600 }, /* The following are extended baud rates */
  { "115200", B115200 },
  { "230400", B230400 }
};

FlagNames local_flags[MAXLOCALFLAGS] = {
  { "ISIG", ISIG },
  { "ICANON", ICANON },
  { "XCASE", XCASE },
  { "ECHO", ECHO },
  { "ECHOE", ECHOE },
  { "ECHOK", ECHOK },
  { "ECHONL", ECHONL },
  { "NOFLSH", NOFLSH },
  { "TOSTOP", TOSTOP },
  { "ECHOCTL", ECHOCTL },
  { "ECHOPRT", ECHOPRT },
  { "ECHOKE", ECHOKE },
  { "FLUSHO", FLUSHO },
  { "PENDIN", PENDIN },
  { "IEXTEN", IEXTEN }
};

FlagNames cntl_names[MAXCCNAMES] = {
  { "VINTR", VINTR },
  { "VQUIT", VQUIT },
  { "VERASE", VERASE },
  { "VKILL", VKILL },
  { "VEOF", VEOF },
  { "VTIME", VTIME },
  { "VMIN", VMIN },
  { "VSWTC", VSWTC },
  { "VSTART", VSTART },
  { "VSTOP", VSTOP },
  { "VSUSP", VSUSP },
  { "VEOL", VEOL },
  { "VREPRINT", VREPRINT },
  { "VDISCARD", VDISCARD },
  { "VWERASE", VWERASE },
  { "VLNEXT", VLNEXT },
  { "VEOL2", VEOL2 }
};

FlagNames line_names[MAXLINENAMES] = {
  { "LE", TIOCM_LE },
  { "DTR", TIOCM_DTR },
  { "RTS", TIOCM_RTS },
  { "ST", TIOCM_ST },
  { "SR", TIOCM_SR },
  { "CTS", TIOCM_CTS },
  { "DCD", TIOCM_CAR },
  { "RNG", TIOCM_RNG },
  { "DSR", TIOCM_DSR }
};

