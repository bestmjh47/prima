/*===========================================================================
 *
 *  @file:     wlan_status_code.h
 *
 *  @brief:    Common header file containing all the status codes
 *             All status codes have been consolidated into one enum
 *
 *  @author:   Kumar Anand
 *             Copyright (C) 2010, Qualcomm, Inc. 
 *             All rights reserved.
 *
 *=========================================================================*/

#ifndef __WLAN_STATUS_CODE_H__
#define __WLAN_STATUS_CODE_H__

/*-------------------------------------------------------------------------
  Include Files
-------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

/* This is to force compiler to use the maximum of an int ( 4 bytes ) */
#define WLAN_STATUS_MAX_ENUM_SIZE    0x7FFFFFFF

/*----------------------------------------------------------------------------
 *  Type Declarations
 * -------------------------------------------------------------------------*/

typedef enum
{
   /* PAL Request succeeded!*/
   PAL_STATUS_SUCCESS = 0,

   /* HAL Request succeeded!*/
   eHAL_STATUS_SUCCESS = 0,
 
	/* Request failed because there of an invalid request.  This is 
      typically the result of invalid parameters on the request*/
   PAL_STATUS_INVAL,

	/* Request refused because a request is already in place and 
   	another cannot be handled currently */
   PAL_STATUS_ALREADY,

	/* Request failed because of an empty condition */
   PAL_STATUS_EMPTY,

	/* Request failed for some unknown reason. */
   PAL_STATUS_FAILURE,

   /* HAL general failure */
   eHAL_STATUS_FAILURE,

   /* Invalid Param*/
   eHAL_STATUS_INVALID_PARAMETER,   

   /* Invalid Station Index*/
   eHAL_STATUS_INVALID_STAIDX,

   /* DPU descriptor table full*/
   eHAL_STATUS_DPU_DESCRIPTOR_TABLE_FULL,

   /* No interrupts */
   eHAL_STATUS_NO_INTERRUPTS,

   /* Interrupt present */
   eHAL_STATUS_INTERRUPT_PRESENT,

   /* Stable Table is full */
   eHAL_STATUS_STA_TABLE_FULL,

   /* Duplicate Station found */
   eHAL_STATUS_DUPLICATE_STA,

   /* BSSID is invalid */
   eHAL_STATUS_BSSID_INVALID,

   /* STA is invalid */
   eHAL_STATUS_STA_INVALID,
   
   /* BSSID is is duplicate */
   eHAL_STATUS_DUPLICATE_BSSID,

   /* BSS Idx is invalid */
   eHAL_STATUS_INVALID_BSSIDX,

   /* BSSID Table is full */
   eHAL_STATUS_BSSID_TABLE_FULL,

   /* Invalid DPU signature*/
   eHAL_STATUS_INVALID_SIGNATURE,

   /* Invalid key Id */
   eHAL_STATUS_INVALID_KEYID,

   /* Already on requested channel */
   eHAL_STATUS_SET_CHAN_ALREADY_ON_REQUESTED_CHAN,

   /* UMA descriptor table is full */
   eHAL_STATUS_UMA_DESCRIPTOR_TABLE_FULL,
   
   /* MIC Key table is full */
   eHAL_STATUS_DPU_MICKEY_TABLE_FULL,

   /* A-MPDU/BA related Error codes */
   eHAL_STATUS_BA_RX_BUFFERS_FULL,
   eHAL_STATUS_BA_RX_MAX_SESSIONS_REACHED,
   eHAL_STATUS_BA_RX_INVALID_SESSION_ID,
   
   eHAL_STATUS_TIMER_START_FAILED,
   eHAL_STATUS_TIMER_STOP_FAILED,
   eHAL_STATUS_FAILED_ALLOC,
   
   /* Scan failure codes */
   eHAL_STATUS_NOTIFY_BSS_FAIL,

   /* Max status value */
   eHAL_STATUS_MAX_VALUE = WLAN_STATUS_MAX_ENUM_SIZE

} palStatus, eHalStatus;

/* Helper Macros */
#define PAL_IS_STATUS_SUCCESS(status) (PAL_STATUS_SUCCESS  == (status))
#define HAL_STATUS_SUCCESS( status )  (eHAL_STATUS_SUCCESS == (status))

#endif //__WLAN_STATUS_CODE_H__
