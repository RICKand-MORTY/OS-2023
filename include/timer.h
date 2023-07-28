#ifndef _TIMER_H
#define _TIMER_H



typedef long clock_t;

struct tms
  {
    clock_t tms_utime;		/* User CPU time.  */
    clock_t tms_stime;		/* System CPU time.  */

    clock_t tms_cutime;		/* User CPU time of dead children.  */
    clock_t tms_cstime;		/* System CPU time of dead children.  */
  };


#define timer_init()    reset_timer()
void reset_timer();

#endif