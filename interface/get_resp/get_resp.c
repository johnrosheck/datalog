/*
 * Source File : get_resp.c
 * Begun : November 20, 2005
 * Latest Revision : June 14, 2014
 * Version : 0.1
 *
 * This program listens to communications from a serial port and collects
 * the data presented.  It parses and converts the data then averages it
 * over a second then writes that average to a logging file.
 *
 * Copyright (C) 2005-2014 John B. Rosheck, Jr.
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/ioctl.h>

/* this is used to restore setups on exiting */
struct termios def_serial;
struct termios new_serial;
int serial_fd; /* serial port file descriptor */

int start_serial_io(int fd, char *given_baud);
void restore_serial_io(int fd);
#define read_ch(a, b) read(a, b, 1)

double getwholetime(void);

char *progname = "";
char *version = "0.1";
char *dsrc = "/dev/ttyACM0";
char *baud = "115200";
int verbose=0;
int show_stats = 0;
char *log_file_name=NULL;
FILE *logfp=NULL;
int curday;
char curdatestring[24] = {0};
int fd;

int get_args(int argc, char **argv);
void show_usage(void);
int set_curdatestring(struct tm *t);
int start_log_file(void);
int my_set_outputs(char *filename, int fd);
int is_hex(char *p);

#define CHMAX 6
double ymax = 2.25;
double ymin = 0.25;
double xmax = 1024;
double xmin = 0;

double last_reading = 0.0;

double slopes[10];
double intercepts[10];

void setup_vars(void) {
  /* 0 is the internal temperature reference */
  slopes[0] = 0.01;
  intercepts[0] = 0.0;
  /* 1 is the internal reference voltage */
  slopes[1] = 0.01;
  intercepts[1] = 0.0;
  /* 2 (121120) is the high air temp */
  slopes[2] = 0.0098682982444561;
  intercepts[2] = -0.00323725408977156;
  /* 3 (121122) is the low air temp */
  slopes[3] = 0.00985812870971242;
  intercepts[3] = -0.0052142133288489;
  /* 4 (121119) is the high water temp */
  slopes[4] = 0.00987958880464085;
  intercepts[4] = 0.00501898952664597;
  /* 5 (121121) is the low water temp */
  slopes[5] = 0.00991392577438172;
  intercepts[5] = -0.0201306846608672;
  /* 6 (121123) is the outside air temp */
  slopes[6] = 0.00987845606433366;
  intercepts[6] = 0.0082486634917041;
  /* 7 is the relative solar incidence */
  slopes[7] = 1.0;
  intercepts[7] = 0.0;
}

typedef struct {
  char label[10];
  int channel_number;
  int raw_value;
  double mv_value;
  
} chan_def;


double slope; /* = (2000.0/10240.0); * scale mV/bit * 1deg/10mV */
double intercept; /*  = -125.0; */
/* double bias = -0.375; */
double bias = 0.0;

void make_line(void) {
  slope = (ymax - ymin) / (xmax - xmin);
  intercept = ymin - (slope * xmin);
  intercept += bias;
  /* printf("slope = %f, intercept = %f\n", slope, intercept); */
}

double convert_reading(int given) {
  double d;
  d = given * 1.0;
  d *= slope;
  d += intercept;
  return(d);
}

double convert_channel(int which, double mv_value, double aux_value) {
  double val = 0.0;
    double degc;
  double v;
  v = mv_value / 1000.0; /* convert mV to Volts */
  switch (which) {
  case 0: /* current 0 */
    /* val = mv_value / 10.0; */
    /* val += -3.55; * calibration offset */
    val = ((v - intercepts[0]) / slopes[0]);
    break;
  case 1: /* current 1 */
    /* val = mv_value / 10.0; */
    /* val += -2.9; * calibration offset */
    val = ((v - intercepts[1]) / slopes[1]);
    break;
  case 2: /* current 2 */
    /* val = mv_value / 10.0; */
    /* val += -1.8; * calibration offset */
    val = ((v - intercepts[2]) / slopes[2]);
    break;
  case 3: /* current 3 */
    /* val = mv_value / 10.0; */
    /* val += 0.35; * calibration offset */
    val = ((v - intercepts[3]) / slopes[3]);
    break;
  case 4:
    /* val = mv_value / 10.0; */
    val = ((v - intercepts[4]) / slopes[4]);
    break;
  case 5:
    /* val = mv_value / 10.0; */
    val = ((v - intercepts[5]) / slopes[5]);
    break;
  case 6:
    /* val = mv_value / 10.0; */
    val = ((v - intercepts[6]) / slopes[6]);
    break;
  case 7: /* probably a temperature sensor */
    /* val = mv_value / 10.0; */
    /* val += 0.8; * calibration offset */
    val = ((v - intercepts[7]) / slopes[7]);
    break;
  default:
    /* this is an error */
    break;
  }
  return(val);
}


double adjust_reading(double va) {
  double d;
  d = (va * 10500.0) / 10000.0;
  return(d);
}

double get_temperature(double mv) {
  double d;
  d = mv - 0.500; /* zero at the sensor */
  d = (d * 1000.0) / 10.0; /* V to mV * 10mV / degF */
  return(d);
}
 
double convert_to_mv(int given) {
  double d;
  /* d = mv - 0.500; * zero at the sensor */
  /* d = (d * 1000.0) / 10.0; * V to mV * 10mV / degF */
  d = given * 1.0;
  /* d *= 4.0; * 4096.0 mV / 1024 codes */
  d *= (5000.0 / 1023.0); /* with 5v reference */
  /* d *= (2480.0 / 1023.0); * using a 2.50 v reference (meas 2.48) */
  return(d);
}
 
int chop_input(char *buf, int *values, int *code) {
  char *p, *p2;
  int i, v;
  p = buf;
  for (i=0;i<=CHMAX;i++) {
    v = strtol(p, &p2, 16);
    values[i] = v;
    p = p2;
  }
  /* there shouldn't be anything left - don't do this */
  if (*p != 0) {
    v = strtol(p, &p2, 16);
    *code = v;
  } else {
    *code = 0;
  }
}

int cntr[CHMAX][1024];
double accum[CHMAX];
int acc_cnt[CHMAX];

void clear_reading_storage(int ch) {
  int i;
  for (i=0;i<1024;i++) {
    cntr[ch][i] = 0;
  }
  accum[ch] = 0.0;
  acc_cnt[ch] = 0;
}

int add_reading(int ch, int value) {
  /* double last = 0.0; */
  double cur, val;
  if ((value >= 0) && (value < 1024)) {
    cntr[ch][value]++;
    /* accum[ch] += convert_to_mv(value); */
    cur = convert_to_mv(value);
    val = convert_channel(ch, cur, last_reading);
    last_reading = val;
    accum[ch] += val;
    acc_cnt[ch]++;
    return(0);
  }
  return(-1);
}

void dump_reading(int ch) {
  int i, j;
  double acc;

  if (acc_cnt[ch] == 0) {
    fprintf(logfp, " 0000.0");
  } else {
    acc = accum[ch] / (acc_cnt[ch] * 1.0);
    if (logfp != NULL) {
      fprintf(logfp, " %4.1f", acc);
    } else {
      printf(" %4.1f", acc);
    }
  }
}

char safe_ctime_buf[512];
char *safe_ctime(time_t *curtime) {
  char *p;
  safe_ctime_buf[0] = 0;
  p = ctime(curtime);
  if ((strlen(p) > 0) && (strlen(p) < 510)) {
    strcpy(safe_ctime_buf, p);
    safe_ctime_buf[strlen(p)-1] = 0;
  }
  return(safe_ctime_buf);
}


char i1_buf[64];
char i2_buf[64];
char i3_buf[64];
#define MAXINTERVAL 63
int icount=0;
time_t last_sec=0;

int test_code_change(int code, time_t tcur) {
  if (tcur != last_sec) { /* new 'cell' to use in readout */
    if (icount >= MAXINTERVAL) { return(-1); }
    /* do input 0 */
    if (code & 0x01) i1_buf[icount] = '1';
    else i1_buf[icount] = '0';
    i1_buf[icount+1] = 0;
    /* do input 1 */
    if (code & 0x02) i2_buf[icount] = '1';
    else i2_buf[icount] = '0';
    i2_buf[icount+1] = 0;
    /* do input 2 */
    if (code & 0x04) i3_buf[icount] = '1';
    else i3_buf[icount] = '0';
    i3_buf[icount+1] = 0;
    icount++;
    last_sec = tcur;
  } else { /* only sets if it was previously reset */
    if (code & 0x01) { /* val1 set */
      if (i1_buf[icount] == '0') {
        i1_buf[icount] = '1';
      } /* else do nothing */
    }
    if (code & 0x02) { /* val2 set */
      if (i2_buf[icount] == '0') {
        i2_buf[icount] = '1';
      } /* else do nothing */
    }
    if (code & 0x04) { /* val3 set */
      if (i3_buf[icount] == '0') {
        i3_buf[icount] = '1';
      } /* else do nothing */
    }
  }
  return(0);
}

int clear_inputs(int interval) {
  int i;
  icount = 0;
  for (i=0;i<=MAXINTERVAL;i++) {
    i1_buf[i] = '0';
    i2_buf[i] = '0';
    i3_buf[i] = '0';
  }
  i1_buf[i] = 0;
  i2_buf[i] = 0;
  i3_buf[i] = 0;
  last_sec = 0; /* forces an initial load into buf */
  return(0);
}

int dump_inputs(void) {
  if (logfp != NULL) {
    fprintf(logfp, " %s %s %s", i1_buf, i2_buf, i3_buf);
  } else {
    printf(" %s %s %s", i1_buf, i2_buf, i3_buf);
  }
  icount = 0;
}

int main(int argc, char *argv[]) {
  int notdone=1;
  char buf[1024];
  int val[9], i, ch, code;
  time_t starttime, curtime;
  struct timeval tv;
  struct timezone tz;
  struct tm *ltime;
  int interval = 1;

  setup_vars();
  
  printf("Hello, World!\n");
  if (get_args(argc, argv) != 0) {
    show_usage();
    exit(1);
  }
  for(ch=0;ch<CHMAX;ch++) {
    clear_reading_storage(ch);
  }
  clear_inputs(interval);
  make_line();
  if ((fd = open(dsrc, O_RDWR|O_NONBLOCK)) < 0) {
    printf("couldn't open data source file\n");
    exit(1);
  }
  start_serial_io(fd, baud);
  my_fgets(buf, 1000, fd);
  notdone = 0;
  printf("syncing time...\n");
  while (notdone < 2) {
    starttime = time(NULL);
    ltime = localtime(&starttime);
    if ((ltime->tm_sec % interval) == 0) {
      notdone++;
    } else {
      /* sleep(1); */
      my_fgets(buf, 1000, fd);
    }
    curday = ltime->tm_mday;
    set_curdatestring(ltime);
  }
  if (log_file_name != NULL) {
    start_log_file();
  }
  printf("starting at %010d\n", (int) starttime);
  notdone = 1;
  while (notdone) {
    /* take care of time first */
    curtime = time(NULL);
    if (curtime >= (starttime + interval)) {
      if (logfp != NULL) {
        fprintf(logfp, "%010d (%s)", (int) starttime, safe_ctime(&starttime));
      } else {
        printf("%010d (%s)", (int) starttime, safe_ctime(&starttime));
      }
      for (ch=0;ch<CHMAX;ch++) {
        dump_reading(ch);
        clear_reading_storage(ch);
      }
      /* dump_inputs(); */
      if (logfp != NULL) {
        fprintf(logfp, "\n");
      } else {
        printf("\n");
      }
      starttime = curtime;
      ltime = localtime(&curtime); /* ltime is needed here and ctime messes it up */
      /* check for the day change here so that the last second of yesterday
         ends up in *yesterday's* log and this days data ends up in this
         days log */
      if (ltime->tm_mday != curday) {
        set_curdatestring(ltime);
        if (log_file_name != NULL) {
          fclose(logfp);
          start_log_file();
        }
        curday = ltime->tm_mday;
      } else {
        /* flush whenever a line is written */
        if (logfp != NULL) {
          fflush(logfp);
        } else {
          fflush(stdout);
        }
      }
    }
    if (my_fgets(buf, 1000, fd) == 1) {
      if (strlen(buf) > 4) {
        /* printf("%s", buf); */
        buf[strlen(buf) - 1] = 0;
        chop_input(buf, val, &code);
        /* test_code_change(~code, curtime); */
        for (ch=0;ch<CHMAX;ch++) {
          add_reading(ch, val[ch+1]);
        }
      }
    } else { /* don't consume 100% of the processor. */
      usleep(1000);
    }
    fflush(stdout);
  }
  exit(0);
}

double getwholetime(void) {
  double s, u;
  struct timeval tv;
  struct timezone tz;
  
  gettimeofday(&tv, &tz);
  s = tv.tv_sec * 1.0;
  u = tv.tv_usec * 1.0;
  u /= 1000000.0;
  s += u;
  /* printf("t%f\n", s); */
  return(s);
}

int get_args(int argc, char **argv) {
  int i;
  char *p;
  char *single_flags="vVhHsS";

  if (argc < 2) return(-1);
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
      case 'l':
      case 'L': /* example two part argument */
        log_file_name = *(argv + i);
        break;
      case 'd':
      case 'D':
        dsrc = *(argv + i);
        break;
      case 'b':
      case 'B':
        baud = *(argv + i);
        break;
      default:
        printf("%s: unknown option \"%s\"\n", *(argv + 0), p);
        return(-1);
        break;
      }
    } else { /* not a valid option flag, must be extra stuff */
      if (argc-1 == i) {
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
  printf("%s (%s) usage:\n", progname, version);
  printf("  %s [-l logfileprefix_and_path]\n", progname);
}

/* create a log file name from the given plus curdatestring plus extension */
int start_log_file(void) {
  char buf[2048];
  if (strlen(log_file_name) > 2000) {
    printf("ERROR - log_file_name size out of range\n");
    return(-1);
  }
  sprintf(buf, "%s-%s.log", log_file_name, curdatestring);
  if ((logfp = fopen(buf, "a")) == NULL) {
    printf("ERROR - couldn't open file \"%s\" - %s\n", buf, strerror(errno));
    return(-1);
  }
  printf("opened file \"%s\" for logging\n", buf);
  return(0);
}

int set_curdatestring(struct tm *t) {
  sprintf(curdatestring, "%04d%02d%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
  return(0);
}

/* set up the serial port */
int init_serial_fd(int fd, struct termios *t, int baud_to_use) {
  int i;
  i = tcgetattr(fd, t);
  printf("serial tcgetattr returns %d\n", i);
  /* if (use_logging) log_termios_settings("init serial termios :", logfilename, t); */
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
  t->c_cflag &= ~(CBAUD); /* zero out baud values */
  /* t->c_cflag |= (CS8 | B9600); * force to 9600 */
  t->c_cflag |= (CS8 | baud_to_use); /* force to 9600 */
  /* set it */
  i = tcsetattr(fd, TCSANOW, t);
  printf("result of new serial settings = %d\n", i);
  /* if (use_logging) log_termios_settings("init serial termios :", logfilename, t); */
  return(i);
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

#define MAXWAITTIME 0.1
int get_schar(char *p, int fd) {
  char c;
  int notdone=1;
  /* clock_t scnt, tcnt; */
  double scnt, tcnt;
  /* scnt = clock(); */
  scnt = getwholetime();
  while (notdone) {
    if (read_ch(fd, &c) == 1) {
      *p = c; /* pass the character back */
      return(1); /* got a character */
    } else {
      usleep(100);
    }
    tcnt = getwholetime() - scnt;
    if (tcnt > MAXWAITTIME) {
      notdone = 0;
    }
  }
  return(0);
}

int my_fgets(char *buf, int maxsize, int fd) {
  maxsize--;
  char c;
  char *p;
  p = buf;
  while (get_schar(&c, fd) == 1) {
    *p++ = c;
    maxsize--;
    if ((c == '\n') || (maxsize == 0)) {
      *p = 0;
      printf("got line - \"%s\"\n", buf);
      return(1);
    }
  }
  *p = 0;
  return(0);
}

int my_set_outputs(char *filename, int fd) {
  FILE *fp;
  char first;
  char buf[100];
  char send[100];
  int i, n;
  if ((fp = fopen(filename, "r")) == NULL) {
    return(-1);
  }
  if (fgets(buf, 99, fp) != NULL) {
    if ((i = is_hex(&buf[0])) >= 0) { /* try to send it */
      n = i & 0xf;
      sprintf(send, "L%01x%01x\n", n, n);      
      printf("%s", send);
    }
  }
  fclose(fp);
  return(0);
}

int is_hex(char *p) {
  if ((*p >= '0') && (*p <= '9')) {
    return(*p - '0');
  }
  if ((*p >= 'a') && (*p <= 'f')) {
    return(*p - 'a' + 10);
  }
  if ((*p >= 'A') && (*p <= 'F')) {
    return(*p - 'A' + 10);
  }
  return(-1);
}
