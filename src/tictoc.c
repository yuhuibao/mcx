/***************************************************************************//**
**  \mainpage Monte Carlo eXtreme - GPU accelerated Monte Carlo Photon Migration
**
**  \author Qianqian Fang <q.fang at neu.edu>
**  \copyright Qianqian Fang, 2009-2018
**
**  \section sref Reference:
**  \li \c (\b Fang2009) Qianqian Fang and David A. Boas, 
**          <a href="http://www.opticsinfobase.org/abstract.cfm?uri=oe-17-22-20178">
**          "Monte Carlo Simulation of Photon Migration in 3D Turbid Media Accelerated 
**          by Graphics Processing Units,"</a> Optics Express, 17(22) 20178-20190 (2009).
**  \li \c (\b Yu2018) Leiming Yu, Fanny Nina-Paravecino, David Kaeli, and Qianqian Fang,
**          "Scalable and massively parallel Monte Carlo photon transport
**           simulations for heterogeneous computing platforms," J. Biomed. Optics, 23(1), 010504, 2018.
**
**  \section slicense License
**          GPL v3, see LICENSE.txt for details
*******************************************************************************/

/***************************************************************************//**
\file    tictoc.h

@brief   Cross-platform C and CUDA timing functions
*******************************************************************************/

#include "tictoc.h"

#define DEFAULT_SOURCE

#ifndef USE_OS_TIMER          /**< use CUDA event for time estimation */
#include <hip/hip_runtime.h>
#include <hip/driver_types.h>
#include <hip/hip_runtime_api.h>
#define MAX_DEVICE 256

static hipEvent_t timerStart[MAX_DEVICE], timerStop[MAX_DEVICE];

/**
 * @brief CUDA timing function using hipEventElapsedTime
 *
 * Use CUDA events to query elapsed time in ms between two events
 */

unsigned int GetTimeMillis () {
  float elapsedTime;
  int devid;
  hipGetDevice(&devid);
  hipEventRecord(timerStop[devid],0);
  hipEventSynchronize(timerStop[devid]);
  hipEventElapsedTime(&elapsedTime, timerStart[devid], timerStop[devid]);
  return (unsigned int)(elapsedTime);
}

/**
 * @brief Start CUDA timer
 */

unsigned int StartTimer () {
  int devid;
  hipGetDevice(&devid);
  hipEventCreate(timerStart+devid);
  hipEventCreate(timerStop+devid);

  hipEventRecord(timerStart[devid],0);
  return 0;
}

#else                          /**< use host OS time functions */

static unsigned int timerRes;
#ifndef _WIN32
#if _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif
#include <sys/time.h>
#include <string.h>
void SetupMillisTimer(void) {}
void CleanupMillisTimer(void) {}

/**
 * @brief Unix timing function using gettimeofday
 *
 * Use gettimeofday to query elapsed time in us between two events
 */

long GetTime (void) {
  struct timeval tv;
  timerRes = 1000;
  gettimeofday(&tv,NULL);
  long temp = tv.tv_usec;
  temp+=tv.tv_sec*1000000;
  return temp;
}

/**
 * @brief Convert us timer output to ms
 */

unsigned int GetTimeMillis () {
  return (unsigned int)(GetTime ()/1000);
}
unsigned int StartTimer () {
   return GetTimeMillis();
}

#else
#include <windows.h>
#include <stdio.h>

/**
 * @brief Windows timing function using QueryPerformanceCounter
 *
 * Retrieves the current value of the performance counter, which is a high 
 * resolution (<1us) time stamp that can be used for time-interval measurements.
 */

int GetTime(void)
{
   static double cycles_per_usec;
   LARGE_INTEGER counter;

   if (cycles_per_usec == 0) {
      static LARGE_INTEGER lFreq;
      if (!QueryPerformanceFrequency(&lFreq)) {
         fprintf(stderr, "Unable to read the performance counter frquency!\n");
         return 0;
      }

      cycles_per_usec = 1000000 / ((double) lFreq.QuadPart);
   }

   if (!QueryPerformanceCounter(&counter)) {
      fprintf(stderr,"Unable to read the performance counter!\n");
      return 0;
   }

   return ((long) (((double) counter.QuadPart) * cycles_per_usec));
}

#pragma comment(lib,"winmm.lib")

unsigned int GetTimeMillis(void) {
  return (unsigned int)timeGetTime();
}

/**
  @brief Set timer resolution to milliseconds

  By default in 2000/XP, the timeGetTime call is set to some resolution
  between 10-15 ms query for the range of value periods and then set timer
  to the lowest possible.  Note: MUST make call to corresponding
  CleanupMillisTimer
*/

void SetupMillisTimer(void) {

  TIMECAPS timeCaps;
  timeGetDevCaps(&timeCaps, sizeof(TIMECAPS)); 

  if (timeBeginPeriod(timeCaps.wPeriodMin) == TIMERR_NOCANDO) {
    fprintf(stderr,"WARNING: Cannot set timer precision.  Not sure what precision we're getting!\n");
  }else {
    timerRes = timeCaps.wPeriodMin;
    fprintf(stderr,"(* Set timer resolution to %d ms. *)\n",timeCaps.wPeriodMin);
  }
}

/**
  @brief Start system timer
*/

unsigned int StartTimer () {
   SetupMillisTimer();
   return 0;
}

/**
  @brief Reset system timer
*/

void CleanupMillisTimer(void) {
  if (timeEndPeriod(timerRes) == TIMERR_NOCANDO) {
    fprintf(stderr,"WARNING: bad return value of call to timeEndPeriod.\n");
  }
}

#endif

#endif

#ifdef _WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

/**
  @brief Cross-platform sleep function
*/

void sleep_ms(int milliseconds){
#ifdef _WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}
