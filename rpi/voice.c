/* voice - the double act: performs the .bit files under bits/ in two voices.
   Events arrive on stdin: a bare trigger name per line fires that trigger
   (the testing seam); an "ok k=v ..." telemetry line derives triggers
   (jolt now, battery when the keys exist). Spontaneous idle bits
   self-schedule on a slow mood clock — silence is the default state.
   -n performs to stdout only (no audio). See software.md, Voice. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <math.h>
#include <sys/wait.h>
#include <sys/select.h>

#define BITS_DIR    "bits"
#define MAX_BITS    64
#define MAX_LINES   48
#define TEXT_MAX    512

#define IDLE_BASE   900         /* s between spontaneous bits, mood-scaled */
#define CD_DEFAULT  600         /* s cooldown when a bit doesn't say */

struct bline {
  char who;                     /* 'M' or 'B' */
  char text[TEXT_MAX];          /* raw, {a|b|c} slots intact */
  int  last_pick;               /* previous first-slot variant, -1 = none */
};

struct bit {
  char   name[64];
  char   trigger[32];
  int    pool;                  /* mode: pool = one line per firing */
  int    cooldown;
  time_t last;                  /* 0 = never fired */
  int    n;
  struct bline line[MAX_LINES];
  int    bag[MAX_LINES], bagn;  /* pool shuffle bag */
};

static struct bit bits[MAX_BITS];
static int   nbits;
static int   quiet;             /* -n: print only, no audio */
static float ph1, ph2;          /* mood sine phases, random each run */

/* one file, one bit: key: value header, blank line, SPEAKER: lines */
static void load_bit(const char *path, const char *name)
{
  FILE *f = fopen(path, "r");
  if (!f || nbits >= MAX_BITS) {
    if (f) fclose(f);
    return;
  }
  struct bit *b = &bits[nbits];
  memset(b, 0, sizeof *b);
  snprintf(b->name, sizeof b->name, "%s", name);
  b->cooldown = CD_DEFAULT;

  char ln[TEXT_MAX];            /* content lines cap at TEXT_MAX too */
  int in_header = 1;
  while (fgets(ln, sizeof ln, f)) {
    ln[strcspn(ln, "\r\n")] = '\0';
    if (ln[0] == '#')
      continue;
    if (!ln[0]) {
      in_header = 0;
      continue;
    }
    if (in_header) {
      char *c = strchr(ln, ':');
      if (!c)
        continue;
      *c = '\0';
      char *v = c + 1;
      while (*v == ' ')
        v++;
      if      (!strcmp(ln, "trigger"))  snprintf(b->trigger, sizeof b->trigger, "%s", v);
      else if (!strcmp(ln, "mode"))     b->pool = !strcmp(v, "pool");
      else if (!strcmp(ln, "cooldown")) b->cooldown = atoi(v);
      continue;                 /* unknown header keys are ignored */
    }
    char who;
    char *t;
    if      (!strncmp(ln, "MIKE:", 5)) { who = 'M'; t = ln + 5; }
    else if (!strncmp(ln, "BODY:", 5)) { who = 'B'; t = ln + 5; }
    else continue;              /* unknown speakers are ignored */
    while (*t == ' ')
      t++;
    if (b->n < MAX_LINES) {
      b->line[b->n].who = who;
      b->line[b->n].last_pick = -1;
      snprintf(b->line[b->n].text, TEXT_MAX, "%s", t);
      b->n++;
    }
  }
  fclose(f);
  if (b->trigger[0] && b->n)
    nbits++;
}

/* *.bit here plus one level of subdirs (bits/private/ etc) */
static void scan_dir(const char *dir)
{
  DIR *d = opendir(dir);
  if (!d)
    return;
  struct dirent *e;
  char path[512];
  while ((e = readdir(d))) {
    if (e->d_name[0] == '.')
      continue;
    snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
    size_t l = strlen(e->d_name);
    if (l > 4 && !strcmp(e->d_name + l - 4, ".bit")) {
      load_bit(path, e->d_name);
    } else {
      DIR *sub = opendir(path);
      if (sub) {
        closedir(sub);
        scan_dir(path);
      }
    }
  }
  closedir(d);
}

/* expand {a|b|c} slots; the first slot won't repeat its previous pick */
static void expand(struct bline *l, char *out, size_t cap)
{
  const char *s = l->text;
  size_t o = 0;
  int slot = 0;
  while (*s && o < cap - 1) {
    if (*s != '{') {
      out[o++] = *s++;
      continue;
    }
    const char *end = strchr(s, '}');
    if (!end) {
      out[o++] = *s++;
      continue;
    }
    int nvar = 1;
    for (const char *p = s + 1; p < end; p++)
      nvar += *p == '|';
    int pick = rand() % nvar;
    if (slot == 0 && nvar > 1 && pick == l->last_pick)
      pick = (pick + 1) % nvar;
    if (slot == 0)
      l->last_pick = pick;
    const char *p = s + 1;
    for (int k = 0; k < pick; k++) {
      while (*p != '|')
        p++;
      p++;
    }
    while (p < end && *p != '|' && o < cap - 1)
      out[o++] = *p++;
    s = end + 1;
    slot++;
  }
  out[o] = '\0';
}

/* say one line: always printed, spoken unless -n; the say scripts are
   the tunable seam (pace, pitch) so exec them rather than piper direct */
static void speak(char who, const char *text)
{
  printf("%s: %s\n", who == 'M' ? "MIKE" : "BODY", text);
  fflush(stdout);
  if (quiet)
    return;
  pid_t pid = fork();
  if (pid == 0) {
    const char *tool = who == 'M' ? "./mikesay" : "./bodysay";
    execl(tool, tool, text, (char *)NULL);
    _exit(127);
  }
  if (pid > 0)
    waitpid(pid, NULL, 0);
  usleep(400 * 1000);           /* the beat between lines */
}

static void perform(struct bit *b)
{
  char out[TEXT_MAX];
  b->last = time(NULL);
  if (b->pool) {                /* one line, dealt from a shuffle bag */
    if (!b->bagn) {
      for (int i = 0; i < b->n; i++)
        b->bag[i] = i;
      for (int i = b->n - 1; i > 0; i--) {
        int j = rand() % (i + 1), t = b->bag[i];
        b->bag[i] = b->bag[j];
        b->bag[j] = t;
      }
      b->bagn = b->n;
    }
    struct bline *l = &b->line[b->bag[--b->bagn]];
    expand(l, out, sizeof out);
    speak(l->who, out);
    return;
  }
  for (int i = 0; i < b->n; i++) {   /* script: the whole sketch */
    expand(&b->line[i], out, sizeof out);
    speak(b->line[i].who, out);
  }
}

/* fire a trigger: one eligible bit, chosen at random; cooldowns gate */
static void fire(const char *trigger)
{
  time_t now = time(NULL);
  struct bit *pick[MAX_BITS];
  int n = 0;
  for (int i = 0; i < nbits; i++)
    if (!strcmp(bits[i].trigger, trigger) &&
        (bits[i].last == 0 || now - bits[i].last >= bits[i].cooldown))
      pick[n++] = &bits[i];
  if (n)
    perform(pick[rand() % n]);
}

static int tel_val(const char *line, const char *key, long *out)
{
  char pat[32];
  snprintf(pat, sizeof pat, " %s=", key);
  const char *p = strstr(line, pat);
  if (!p)
    return -1;
  *out = strtol(p + strlen(pat), NULL, 10);
  return 0;
}

/* derive triggers from a tel reply line */
static void telemetry(const char *line)
{
  static long prev_moving = -1;
  long moving, thr, mv;

  if (!tel_val(line, "moving", &moving)) {
    if (tel_val(line, "thr", &thr))
      thr = 0;
    if (moving == 1 && prev_moving == 0 && thr == 0)
      fire("jolt");             /* moved, but nobody commanded it */
    prev_moving = moving;
  }
  if (!tel_val(line, "vbat_mv", &mv)) {  /* future key, handled today */
    if (mv < 10800)
      fire("battery_critical");
    else if (mv < 11400)
      fire("battery_low");
  }
}

/* slow mood: scales the spontaneous interval; ~0.35..1.9 */
static float mood(time_t t)
{
  float m = 1.0f + 0.45f * sinf((float)t / 1130.0f + ph1)
                 + 0.35f * sinf((float)t / 407.0f  + ph2);
  return m < 0.35f ? 0.35f : m > 1.9f ? 1.9f : m;
}

int main(int argc, char **argv)
{
  quiet = argc > 1 && !strcmp(argv[1], "-n");
  srand(time(NULL) ^ getpid());
  ph1 = (rand() % 628) / 100.0f;
  ph2 = (rand() % 628) / 100.0f;

  scan_dir(BITS_DIR);
  if (!nbits) {
    fprintf(stderr, "voice: no bits in %s/\n", BITS_DIR);
    return 1;
  }
  fprintf(stderr, "voice: %d bits loaded\n", nbits);

  fire("boot");
  time_t next_idle = time(NULL) + (time_t)(IDLE_BASE * mood(time(NULL)));

  char buf[256], line[256];
  int len = 0;
  for (;;) {
    fd_set rf;
    FD_ZERO(&rf);
    FD_SET(0, &rf);
    struct timeval tv = { 1, 0 };
    if (select(1, &rf, NULL, NULL, &tv) > 0) {
      int n = read(0, buf, sizeof buf);
      if (n <= 0)
        break;                  /* EOF: bench pipes end here */
      for (int i = 0; i < n; i++) {
        char c = buf[i];
        if (c == '\n') {
          line[len] = '\0';
          len = 0;
          if (!line[0])
            continue;
          if (!strncmp(line, "ok ", 3))
            telemetry(line);
          else
            fire(line);
        } else if (len < (int)sizeof line - 1 && c != '\r') {
          line[len++] = c;
        }
      }
    }
    time_t now = time(NULL);
    if (now >= next_idle) {
      fire("idle");
      next_idle = now + (time_t)(IDLE_BASE * mood(now));
    }
  }
  return 0;
}
