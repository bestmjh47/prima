/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __WDA_DEBUG_H__
#define __WDA_DEBUG_H__
#if  !defined (ANI_OS_TYPE_ANDROID)
#include <stdio.h>
#endif
#include <stdarg.h>

#include "utilsApi.h"
#include "sirDebug.h"
#include "sirParams.h"
#define WDA_DEBUG_LOGIDX  ( LOG_INDEX_FOR_MODULE(SIR_WDA_MODULE_ID) )



#ifdef WLAN_DEBUG

#define WDALOGP(x0)  x0
#define WDALOGE(x0)  x0
#define WDALOGW(x0)  x0
#define WDALOG1(x0)  x0

#ifdef HAL_DEBUG_LOG2
#define WDALOG2(x0)  x0
#else
 #define WDALOG2(x0)
#endif

#ifdef HAL_DEBUG_LOG3
#define WDALOG3(x0)  x0
#else
 #define WDALOG3(x0)
#endif

#ifdef HAL_DEBUG_LOG4
#define WDALOG4(x0)  x0
#else
 #define WDALOG4(x0)
#endif

#define STR(x)  x

#else

#define WDALOGP(x)  x
#define WDALOGE(x)  {}
#define WDALOGW(x)  {}
#define WDALOG1(x)  {}
#define WDALOG2(x)  {}
#define WDALOG3(x)  {}
#define WDALOG4(x)  {}
#define STR(x)      ""
#endif

void wdaLog(tpAniSirGlobal pMac, tANI_U32 loglevel, const char *pString,...);

#endif // __WDA_DEBUG_H__

