#ifndef _VOS_POWER_H_
#define _VOS_POWER_H_

/*!
  @file
  vos_power.h

  @brief
  This is the interface to VOSS power APIs using for power management 
  of the WLAN Libra module from the MSM PMIC. These implementation of 
  these APIs is very target dependent, also these APIs should only be
  used when the WLAN Libra module is powered from the MSM PMIC and not
  from an external independent power source

*/

/*===========================================================================

  Copyright (c) 2008 QUALCOMM Incorporated. All Rights Reserved

  Qualcomm Proprietary

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  QUALCOMM Incorporated and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of QUALCOMM Incorporated.

===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
01/20/09   rg      Initial creation (based on Henri's slides)
===========================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/
#include "vos_api.h"

/*===========================================================================

                        DEFINITIONS AND TYPES

===========================================================================*/
typedef enum
{
  VOS_CALL_SYNC,    /* operation is synchronous */
  VOS_CALL_ASYNC    /* operation is asynchronous */    

} vos_call_status_type;

typedef v_VOID_t (*vos_power_cb_type)
(
  v_PVOID_t  user_data,    /* user cookie */ 
  VOS_STATUS result        /* result of operation:
                              VOS_STATUS_SUCCESS for success
                              VOS_STATUS_E_FAILURE for failure  */
);

/*===========================================================================

                    FUNCTION PROTOTYPES

===========================================================================*/

/**
  @brief vos_chipPowerUp() - This API will power up the Libra chip

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  The Libra SDIO core will have been initialized if the operation completes
  successfully

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipPowerUp
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipPowerDown() - This API will power down the Libra chip

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipPowerDown
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipReset() - This API will reset the Libra chip

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  A hard reset will involve a powerDown followed by a PowerUp; a soft reset
  can potentially be accomplished by writing to some device registers

  The Libra SDIO core will have been initialized if the operation completes
  successfully

  @param status [out] : whether this operation will complete sync or async
  @param soft [in] : VOS_TRUE if a soft reset is desired 
                     VOS_FALSE for a hard reset i.e. powerDown followed by powerUp
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_NOSUPPORT - soft reset asked for but not supported
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipReset
(
  vos_call_status_type* status,
  v_BOOL_t              soft,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipVoteOnPASupply() - This API will power up the PA supply

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipVoteOnPASupply
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipVoteOffPASupply() - This API will vote to turn off the 
  PA supply. Even if we succeed in voting, there is a chance PA supply will not 
  be turned off. This will be treated the same as a failure.

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately) could be 
                         because the voting algorithm decided not to power down PA  
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipVoteOffPASupply
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipAssertDeepSleep() - This API will assert the deep 
  sleep signal to Libra

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipAssertDeepSleep
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipDeAssertDeepSleep() - This API will de-assert the deep sleep
  signal to Libra

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipDeAssertDeepSleep
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipVoteOnRFSupply() - This API will power up the RF supply

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipVoteOnRFSupply
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipVoteOffRFSupply() - This API will vote to turn off the 
  RF supply. Even if we succeed in voting, there is a chance RF supply will not 
  be turned off as RF rails could be shared with other modules (outside WLAN)

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately) could be 
                         because the voting algorithm decided not to power down PA  
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipVoteOffRFSupply
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);
/**
  @brief vos_chipVoteOnBBAnalogSupply() - This API will power up the I/P voltage
  used by Base band Analog.

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipVoteOnBBAnalogSupply
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipVoteOffBBAnalogSupply() - This API will vote off the BB Analog supply.

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately) could be 
                         because the voting algorithm decided not to power down PA  
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipVoteOffBBAnalogSupply
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);
/**
  @brief vos_chipVoteOnXOBuffer() - This API will vote to turn on the XO buffer from
  PMIC. This API will be used when Libra uses the TCXO from PMIC on the MSM

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately)       
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipVoteOnXOBuffer
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);

/**
  @brief vos_chipVoteOffXOBuffer() - This API will vote off PMIC XO buffer.

  This operation may be asynchronous. If so, the supplied callback will
  be invoked when operation is complete with the result. The callback will 
  be called with the user supplied data. If the operation is known to be 
  sync, there is no need to supply a callback and user data.

  @param status [out] : whether this operation will complete sync or async
  @param callback [in] : user supplied callback invoked when operation completes
  @param user_data [in] : user supplied context callback is called with

  @return 
  VOS_STATUS_E_INVAL - status is NULL 
  VOS_STATUS_E_FAULT - the operation needs to complete async and a callback 
                       and user_data has not been specified (status will be
                       set to VOS_CALL_ASYNC) 
  VOS_STATUS_E_ALREADY - operation needs to complete async but another request
                         is already in progress (status will be set to VOS_CALL_ASYNC)  
  VOS_STATUS_E_FAILURE - operation failed (status will be set appropriately) could be 
                         because the voting algorithm decided not to power down PA  
  VOS_STATUS_SUCCESS - operation completed successfully if status is SYNC (will be set)
                       OR operation started successfully if status is ASYNC (will be set)

*/
VOS_STATUS vos_chipVoteOffXOBuffer
(
  vos_call_status_type* status,
  vos_power_cb_type     callback,
  v_PVOID_t             user_data
);
#endif /* _VOS_POWER_H_ */
