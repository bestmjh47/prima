
/** ------------------------------------------------------------------------- * 
    ------------------------------------------------------------------------- *  

  
    \file palTimer.h
  
    \brief Define data structure and ptototype for PAL timer.
  
    $Id$ 
  
    Copyright (C) 2006 Airgo Networks, Incorporated
  
    ... description...
  
   ========================================================================== */

#if !defined( PALTIMER_H__ )
#define PALTIMER_H__


/*
PAL TIMER
  This timer can be used for every module in Windows side. 
  On Linus side, this can only be used by timer for HDD. Not for timers used in rtlib, hence it doesn't replace TX_TIMER
*/

typedef void * tPalTimerHandle;

#define PAL_INVALID_TIMER_HANDLE (NULL)

typedef void (*palTimerCallback)(void *);

#define PAL_TIMER_TO_MS_UNIT      1000
#define PAL_TIMER_TO_SEC_UNIT     1000000


//PAL timer functions
//pPalTimer is a pointer to a caller allocated tPalTimer object
//pContext is a pointer to an object that will be passed in when callback is called
//fRestart to set whether the timer is restart after callback returns
#ifdef TIMER_MANAGER
#define palTimerAlloc(hHdd, phPalTimer, pCallback, pContext) \
              palTimerAlloc_debug(hHdd, phPalTimer, pCallback, pContext, __FILE__, __LINE__)
eHalStatus palTimerAlloc_debug( tHddHandle hHdd, tPalTimerHandle *phPalTimer, 
                          palTimerCallback pCallback, void *pContext, char* fileName, v_U32_t lineNum  );              
#else
eHalStatus palTimerAlloc(tHddHandle hHdd, tPalTimerHandle *phPalTimer, palTimerCallback pCallback, void *pContext);
#endif
//This function will free the timer
//On Windows platform, it can only be called when device is unloading.
eHalStatus palTimerFree(tHddHandle, tPalTimerHandle);
//To start a timer
//uExpireTime is the timer lapse before timer fires. If the timer is in running state and the fRestart is true,
//uExpireTime is set so that it is the new interval, in units of microseconds
eHalStatus palTimerStart(tHddHandle, tPalTimerHandle, tANI_U32 uExpireTime, tANI_BOOLEAN fRestart);
//palTimerStop will cancel the timer but doesn't guarrantee the callback will not called afterwards
//For Windows, if the driver is halting, the callback is not called after this function returns. 
eHalStatus palTimerStop(tHddHandle, tPalTimerHandle); 


#endif
