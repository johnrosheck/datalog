/*
 * Source File : cmd_resp.c
 * Begun : November 20, 2005
 * Latest Revision : June 14, 2014
 * Version : 0.1
 *
 * This program listens to communications from a serial port and collects
 * the data presented.  It parses and converts the data then averages it
 * over a second then writes that average to a logging file.  This program
 * also will send commands that set bits on the remote end of the serial
 * link.
 *
 * Previously called get_resp.c
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
#include <signal.h>
#include <setjmp.h>
#include <sys/stat.h>

/* buffer for time marker on stdout messages */
char current_time_string[128];
/* this function updates the buffer then returns it as a string */
char *get_current_time(void);
/* records this process's pid value in a file */
int set_pid_file(void);

/* this is used to restore setups on exiting */
struct termios def_serial;
struct termios new_serial;
int serial_fd; /* serial port file descriptor */

int start_serial_io(int fd, char *given_baud);
void restore_serial_io(int fd);
#define read_ch(a, b) read(a, b, 1)

double getwholetime(void);

char *dsrc = "/dev/ttyS2";
char *progname = "";
char *version = "0.1";
int verbose=0;
int show_stats = 0;
char *log_file_name=NULL;
FILE *logfp=NULL;
int curday;
char curdatestring[24] = {0};
int fd;
char *control_settings_file="control_settings.txt";
char *pid_file_path_name="cmd_resp_pid.txt";
int pid;
int get_args(int argc, char **argv);
void show_usage(void);
int set_curdatestring(struct tm *t);
int start_log_file(void);

time_t last_st_mtime;

int last_output_state = 0;
int last_code_state = 0;

#define CHMAX 4

double last_reading = 0.0;

int user_int_hit = 0;
int stop_run_hit = 0;
static struct sigaction old_sigusr1_handler;
static struct sigaction old_sigint_handler;
static struct sigaction old_sighup_handler;

static void control_sigusr1_handler(int v) {
  user_int_hit = 1;
}

static void control_sigint_handler(int v) {
  stop_run_hit = 1;
}

static void control_sighup_handler(int v) {
  stop_run_hit = 2;
}

int set_estop(int fd, int c) {
  char obuf[10];
  char bbuf[200];
  char *p;
  sprintf(obuf, "E%02X%02X", c, c);
  printf("%s : estop command = %5s (%d)", get_current_time(), obuf, c);
  obuf[5] = 10; obuf[6] = 0; obuf[7] = 0;
  p = &obuf[0];
  while (*p != 0) {
    write(fd, p, 1);
    usleep(100);
    p++;
  }
  return(0);
}

int set_output(int fd, int c) {
  char obuf[10];
  char bbuf[200];
  char *p;
  sprintf(obuf, "L%01X%01X", c, c);
  sprintf(bbuf, "output command = %5s (%d)", obuf, c);
  printf("%s : %s\n", get_current_time(), bbuf);
  
  obuf[3] = 10; obuf[4] = 0; obuf[5] = 0;
  p = &obuf[0];
  while (*p != 0) {
    write(fd, p, 1);
    usleep(100);
    p++;
  }
  return(0);
}

typedef struct {
  char label[10];
  int channel_number;
  int raw_value;
  double mv_value;
  
} chan_def;

double convert_channel(int which, double mv_value, double aux_value) {
  double val = 0.0;
  double degc;
  switch (which) {
  case 0: /* current measure out of battery */
    /* Vo = Is * Rs *Rl / 5000, where Rs is 0.0001 ohms and Rl is 500K */
    /* val = mv_value * 0.01; this should convert it to amps of battery draw */
    val = (mv_value / 1000.0) / 0.01;
    break;
  case 1: /* DC representation of the grid supply */
    val = mv_value;
    break;
  case 2: /* internal temperature */
    val = mv_value / 10.0; /* convert to degrees at 10 mV per deg F */
    /* val += -14.0; * calibration offset */
    break;
  case 3: /* controller supplied voltage using */
    val = (mv_value / 1000.0) / 0.128236877;
    break;
  case 4:
    val = mv_value;
    break;
  case 5:
    val = mv_value;
    break;
  case 6:
    val = mv_value;
    break;
  case 7:
    val = mv_value;
    break;
  default:
    /* this is an error */
    break;
  }
  return(val);
}

double convert_to_mv(int given) {
  double d;
  d = given * 1.0;
  d *= 5020.0 / 1024;
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
  v = strtol(p, &p2, 16);
  *code = v;
  return(0);
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

  acc = accum[ch] / (acc_cnt[ch] * 1.0);
  if (logfp != NULL) {
    fprintf(logfp, " %4.1f", acc);
  } else {
    printf(" %4.1f", acc);
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

void update_state(int new_state) {
  set_output(fd, new_state);
  last_code_state = new_state;
}

void update_system_control(int current_state) {
  int new_code;

  new_code = stat_settings_file(); /* reload settings */
  if (new_code != current_state) { /* do the update */
    update_state(new_code);
  }
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
  struct sigaction siga;

  if (get_args(argc, argv) != 0) {
    show_usage();
    exit(1);
  }
  printf("%s : starting cmnd_sys version %s...\n", get_current_time(), version);
  /* setup signals */
  siga.sa_handler = control_sigusr1_handler;
  siga.sa_flags = 0;
  sigaddset(&siga.sa_mask, SIGUSR1);
  if (sigaction(SIGUSR1, &siga, &old_sigusr1_handler) != 0) {
    printf("ERROR - couldn't install new SIGUSR1 handler\n");
    exit(1);
  }
  siga.sa_handler = control_sigint_handler;
  siga.sa_flags = 0;
  sigaddset(&siga.sa_mask, SIGINT);
  if (sigaction(SIGINT, &siga, &old_sigusr1_handler) != 0) {
    printf("ERROR - couldn't install new SIGINT handler\n");
    exit(1);
  }
  siga.sa_handler = control_sighup_handler;
  siga.sa_flags = 0;
  sigaddset(&siga.sa_mask, SIGHUP);
  if (sigaction(SIGHUP, &siga, &old_sigusr1_handler) != 0) {
    printf("ERROR - couldn't install new SIGHUP handler\n");
    exit(1);
  }
  
  if ((i=get_control_settings()) < 0) {
    printf("ERROR - control file unavailable\n");
    exit(1);
  }
  last_code_state = i; /* initialize so it doesn't change it unwantedly */

  /* record the pid so other programs can signal this one */
  set_pid_file();

  /* setup the data area */
  for(ch=0;ch<CHMAX;ch++) {
    clear_reading_storage(ch);
  }

  if ((fd = open(dsrc, O_RDWR|O_NONBLOCK)) < 0) {
    printf("%s : couldn't open data source file\n", get_current_time());
    fclose(logfp);
    exit(1);
  }
  start_serial_io(fd, "9600");
  my_fgets(buf, 1000, fd);
  notdone = 0;
  printf("%s : syncing time...\n", get_current_time());
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
  printf("%s : entering normal run loop\n", get_current_time());
  notdone = 1;
  while (notdone) {
    /* if (user_int_hit == 1) {
      printf("%s : reloading control settings file\n", get_current_time());
      get_control_settings();
      user_int_hit = 0;
    } */
    if (my_fgets(buf, 1000, fd) == 1) {
      curtime = time(NULL);
      ltime = localtime(&curtime);
      if (ltime->tm_mday != curday) {
        set_curdatestring(ltime);
        if (log_file_name != NULL) {
          fclose(logfp);
          start_log_file();
        }
        curday = ltime->tm_mday;
      }
      if (strlen(buf) > 4) {
        /* printf("%s", buf); */
        buf[strlen(buf) - 1] = 0; /* step on LF */
        chop_input(buf, val, &code);
        for (ch=0;ch<CHMAX;ch++) {
          add_reading(ch, val[ch+1]);
        }
      }
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
        code &= 0xf; /* only care about the last 4 bits */
        starttime = curtime;
        if (logfp != NULL) {
          fprintf(logfp, " %01x\n", code);
        } else {
          printf(" %01x\n", code);
        }
        /* do anything else that needs to be done every interval (second) */
        update_system_control(code);
      }
      if (logfp != NULL) {
        fflush(logfp);
      } else {
        fflush(stdout);
      }
    } else {
      usleep(100);
    }
    /* test for exit signals */
    if (stop_run_hit > 0) {
      if (stop_run_hit == 1) {
        printf("%s : exiting program on INT signal\n", get_current_time());
      } else {
        printf("%s : exiting program on HUP signal\n", get_current_time());
      }
      notdone = 0;
    }
  }
  if (logfp != NULL) {
    fclose(logfp);
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
      case 'c':
      case 'C':
        control_settings_file = *(argv + i);
        break;
      case 'p':
      case 'P':
        pid_file_path_name = *(argv + i);
        break;
      case 'd':
      case 'D':
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
  printf("  %s [options]\n", progname);
  printf("  -v = verbose reporting\n");
  printf("  -l logfileprefix_and_path\n");
  printf("  -c path_and_filename_for_control_settings_file\n");
  printf("  -p pid_file_path_name\n");
  printf("  -h = print usage information\n");
}

/* create a log file name from the given plus curdatestring plus extension */
int start_log_file(void) {
  char buf[2048];
  if (strlen(log_file_name) > 2000) {
    printf("%s : ERROR - log_file_name size out of range\n", get_current_time());
    return(-1);
  }
  sprintf(buf, "%s-%s.log", log_file_name, curdatestring);
  if ((logfp = fopen(buf, "a")) == NULL) {
    printf("%s : ERROR - couldn't open file \"%s\" - %s\n", get_current_time(), buf, strerror(errno));
    return(-1);
  }
  printf("%s : opened file \"%s\" for logging\n", get_current_time(), buf);
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
  printf("%s : serial tcgetattr returns %d\n", get_current_time(), i);
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
  printf("%s : result of new serial settings = %d\n", get_current_time(), i);
  /* if (use_logging) log_termios_settings("init serial termios :", logfilename, t); */
  return(i);
}

int start_serial_io(int fd, char *given_baud) {
  int i, baud;

  serial_fd = fd;
  baud = B9600;
  i = tcgetattr(fd, &def_serial);
  /* printf("serial tcgetattr returns %d\n", i); */
  if (given_baud != NULL) {
    if (strcmp(given_baud, "1200") == 0) {
      baud = B1200;
      printf("%s : using baud = 1200\n", get_current_time());
    } else if (strcmp(given_baud, "2400") == 0) {
      baud = B2400;
      printf("%s : using baud = 2400\n", get_current_time());
    } else if (strcmp(given_baud, "4800") == 0) {
      baud = B4800;
      printf("%s : using baud = 4800\n", get_current_time());
    } else if (strcmp(given_baud, "9600") == 0) {
      baud = B9600;
      printf("%s : using baud = 9600\n", get_current_time());
    } else if (strcmp(given_baud, "19200") == 0) {
      baud = B19200;
      printf("%s : using baud = 19200\n", get_current_time());
    } else {
      /* printf("not a valid baud rate, using default of 9600\n"); */
      baud = B9600;
    }
  }
  init_serial_fd(fd, &new_serial, baud);
  return(0);
}

void restore_serial_io(int fd) {
  int i;
  if (fd != serial_fd) {
    printf("%s : error - fd has changed, new %d, old %d\n", get_current_time(), fd, serial_fd);
    return;
  } else {
    i = tcsetattr(fd, TCSANOW, &def_serial); /* force original one */
    /* printf("serial restore tcsetattr returns %d\n", i); */
  }
}

/* #define MAXWAITTIME 30 */
#define MAXWAITTIME 300000
int get_schar(char *p, int fd) {
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
      return(1);
    }
  }
  *p = 0;
  return(0);
}

int get_control_settings(void) {
  FILE *fp;
  char buf[512];
  char *p;
  unsigned long flags = 0;
  struct stat stat_buf;
  if ((fp = fopen(control_settings_file, "r")) == NULL) {
    /* assume that its the first time so use the program defaults */
    return(-1);
  }
  if (fgets(buf, 511, fp) != NULL) {
    p = &buf[0];
    flags = strtoul(p, NULL, 16);
  } else {
    fclose(fp);
    printf("%s : error reading flag file\n", get_current_time());
    return(-1);
  }
  fclose(fp);
  printf("%s : control flag = %xH\n", get_current_time(), flags);
  if (stat(control_settings_file, &stat_buf) == 0) { /* worked */
    last_st_mtime = stat_buf.st_mtime;
    printf("%s : last stat time is now %d\n", get_current_time(), (int) last_st_mtime);
  } else {
    printf("%s : failed to stat control settings file - %d\n", get_current_time(), errno);
  }
  fflush(stdout);
  /* sleep(15); */
  return(flags & 0xffff);
}

char *get_current_time(void) {
  struct tm *atm;
  time_t ltime;
  /* get the time independently */
  ltime = time(NULL);
  atm = localtime(&ltime);
  sprintf(current_time_string, "%04d%02d%02d-%02d%02d%02d", atm->tm_year + 1900, atm->tm_mon + 1, atm->tm_mday, atm->tm_hour, atm->tm_min, atm->tm_sec);
  return(current_time_string);
}

int set_pid_file(void) {
  FILE *pidfp;

  pid = getpid();
  if ((pidfp = fopen(pid_file_path_name, "w")) == NULL) {
    printf("%s : ERROR - couldn't open pid file \"%s\" - %s\n", get_current_time(), pid_file_path_name, strerror(errno));
    return(-1);
  }
  fprintf(pidfp, "%d\n", pid);
  fclose(pidfp);
  printf("%s : pid stored in file \"%s\"\n", get_current_time(), pid_file_path_name);
  return(0);
}

int stat_settings_file(void) {
  struct stat stat_buf;
  if (stat(control_settings_file, &stat_buf) == 0) { /* worked */
    if (stat_buf.st_mtime != last_st_mtime) { /* new settings file */
      printf("%s : new stat file mtime found\n", get_current_time());
      /* load the new settings */
      return(get_control_settings());
    } /* else file has not been updated so ignore */
    return(last_code_state);
  } else { /* failed to stat file */
    printf("%s : failed to stat control settings file - %d\n", get_current_time(), errno);
  }
  return(-1);
}
