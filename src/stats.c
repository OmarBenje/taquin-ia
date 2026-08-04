#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include "stats.h"

void stats_reset(stats_t *s)
{
  memset(s, 0, sizeof(*s));
  s->solution_len = -1;
}

double now_seconds(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

size_t process_peak_rss(void)
{
  struct rusage ru;

  if (getrusage(RUSAGE_SELF, &ru) != 0) return 0;

  /* ru_maxrss est en octets sur macOS, en kilo-octets sur Linux. */
#if defined(__APPLE__)
  return (size_t)ru.ru_maxrss;
#else
  return (size_t)ru.ru_maxrss * 1024u;
#endif
}
