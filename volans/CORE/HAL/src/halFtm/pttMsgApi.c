/**
 *
   Airgo Networks, Inc proprietary.
   All Rights Reserved, Copyright 2005 2006
   This program is the confidential and proprietary product of Airgo Networks Inc.
   Any Unauthorized use, reproduction or transfer of this program is strictly prohibited.


   pttMsgApi.h: Handles kernal mode messages sent from socket or pttApi.dll
   Author:  Mark Nelson
   Date:    6/22/05

   History -
   Date        Modified by              Modification Information
  --------------------------------------------------------------------------

 */

#include "ani_assert.h"
#include <sys_api.h>
#include "pttModuleApi.h"
#include "halPhyUtil.h"
#include "qwlan_version.h"
#include <halPhyVersion.h>
#ifndef VERIFY_HALPHY_SIMV_MODEL
#include "wlan_qct_sys.h"
#endif


#ifndef WLAN_FTM_STUB

extern void HEXDUMP(char *s0, char *s1, int len);


/* Our convention is that Ptt Messages are sent from a little-endian application in little-endian order over the network.
   The Socket Application will do byte ordering on the message length & ID so it knows to route the message to our pttProcessMsg function.
   Once pttProcessMsg gets it, the following macros must be applied to the msgBody.
   We would use hton for messages received, and ntoh for message responses sent,
   but they are really the same.
   If this code is built on a big-endian machine(some AP platforms), then the byte order must be
   reversed as appropriate for the message.
   If this code is built on a little-endian machine(all PC clients), then the macros will not swap the byte order.
   Note that all the messages are not packed and have been crafted with explicit padding to prevent any 16-bit or 32-bit boundary issues.

   Doing it this way, trades cross-platform compatibility for easier maintenance of the messages.
   These macros only have to be applied here, the central place for message processing,
   rather than having this done in multiple applications which would be a bigger maintenance problem.
   Due to the nature of the applications(mfg & regulatory), we do not expect to ever run these on big-endian platforms.
   They would always be run on little-endian PCs(Intel or AMD likely).


*/

#ifdef VERIFY_HALPHY_SIMV_MODEL
extern void HalPhy_Init(void);
extern void testRxIqCal(void);
extern void testDpdCal(tANI_U8 txGain);
extern void calDebugSetCalControlBitmap(void);
#endif


typedef struct
{
    tANI_U32 ipg                    :30;    //interframe space in 160MHz clocks
    tANI_U32 crc                    :1;     //always 1 = calculate CRC
    tANI_U32 cat                    :1;     //always 0 = normal packet

    tANI_U32 wud                    :16;    //warmup delay
    tANI_U32 cmd_len                :8;     //8 for 11b & 11a mixed mode, 11 for greenfield rates
    tANI_U32 svc_len                :5;     //always 0
    tANI_U32 pkt_type               :3;     //ignored

    tANI_U32 pyldr_len              :16;    //number of random payload bytes to be generated
    tANI_U32 pyldf_len              :16;    //number of MPDU header bytes (+ number of pregenerated fixed payload bytes)
}sPhyDbgHdr;



typedef struct
{
    //byte 0
    tANI_U32 command_length         :8;     //8 for 11b & 11a mixed mode, 11 for greenfield rates

    //byte 1
    tANI_U32 packet_type            :3;
    tANI_U32 subband_mode           :1;
    tANI_U32 reserved_1             :1;
    tANI_U32 bandwidth_mode         :2;
    tANI_U32 beamform_enable        :1;

    //byte 2
    tANI_U32 tx_antenna_enable      :3;
    tANI_U32 reserved_2             :1;
    tANI_U32 nss_11b_mode           :2;
    tANI_U32 b_rate                 :2; //seems to have vanished from the latest mpi document, but Brian is setting this

    //byte 3
    tANI_U32 tx_demanded_power      :5;
    tANI_U32 reserved_3             :3;

    //byte 4
    tANI_U32 escort_packet          :1;
    tANI_U32 reserved_4             :1;
    tANI_U32 plcp_override          :2;
    tANI_U32 short_guard_interval   :1;
    tANI_U32 concat_packet          :1;
    tANI_U32 reserved_5             :2;

    //byte 5 & 6
    tANI_U32 psdu_length            :16;

    //byte 7
    tANI_U32 psdu_rate              :4;
    tANI_U32 airgo_11n_rates        :1;
    tANI_U32 last_psdu_flag         :1;
    tANI_U32 a_mpdu_flag            :1;
    tANI_U32 reserved_6             :1;

    //byte 8 & 9
    tANI_U32 ppdu_length            :11;
    tANI_U32 reserved_7             :5;

    //byte 10
    tANI_U32 ppdu_rate              :4;
    tANI_U32 reserved_8             :4;

    //there is an unused byte here - don't forget to account for it when doing sizeof(tPhyDbgFrame)
}sMpiHdr;

typedef struct
{
    sPhyDbgHdr  phyDbgHdr;
    sMpiHdr     mpiHdr;
    //sMPDUHeader mpduHdr;
    //payload goes here
}tPhyDbgFrame;

extern void printFrameFields(tpAniSirGlobal pMac, tPhyDbgFrame *frame, eHalPhyRates rate);


#ifndef VERIFY_HALPHY_SIMV_MODEL
void pttSendMsgResponse(tpAniSirGlobal pMac, tPttMsgbuffer *pPttMsg)
{
     wlan_sys_ftm(pPttMsg);
}
#endif

/* KEEP AROUND FOR DEBUGGING MESSAGES IN LINUX
typedef struct
{
    char string[50];
    tANI_U16 id;
}sPttMsgIdStr;

const sPttMsgIdStr pttMsgDbgStrings[] =
{
    { "PTT_MSG_GET_TPC_CAL_STATE",                         0x3011 },
    { "PTT_MSG_RESET_TPC_CAL_STATE",                       0x3012 },
    { "PTT_MSG_SET_NV_CKSUM",                              0x3013 },
    { "PTT_MSG_GET_NV_CKSUM",                              0x3014 },
    { "PTT_MSG_GET_NV_TABLE",                              0x3016 },
    { "PTT_MSG_SET_NV_TABLE",                              0x3017 },
    { "PTT_MSG_SET_NV_IMAGE",                              0x3018 },
    { "PTT_MSG_BLANK_NV",                                  0x3019 },
    { "PTT_MSG_GET_NV_IMAGE",                              0x301E },
    { "PTT_MSG_DEL_NV_TABLE",                              0x301F },
    { "PTT_MSG_GET_NV_FIELD",                              0x3020 },
    { "PTT_MSG_SET_NV_FIELD",                              0x3021 },
    { "PTT_MSG_STORE_NV_TABLE",                            0x3022 },
    { "PTT_MSG_DBG_READ_REGISTER",                         0x3040 },
    { "PTT_MSG_DBG_WRITE_REGISTER",                        0x3041 },
    { "PTT_MSG_API_WRITE_REGISTER",                        0x3042 },
    { "PTT_MSG_API_READ_REGISTER",                         0x3043 },
    { "PTT_MSG_DBG_READ_MEMORY",                           0x3044 },
    { "PTT_MSG_DBG_WRITE_MEMORY",                          0x3045 },
    { "PTT_MSG_ENABLE_CHAINS",                             0x304F },
    { "PTT_MSG_SET_CHANNEL",                               0x3050 },
    { "PTT_MSG_SET_WAVEFORM",                              0x3071 },
    { "PTT_MSG_SET_TX_WAVEFORM_GAIN",                      0x3072 },
    { "PTT_MSG_GET_WAVEFORM_POWER_ADC",                    0x3073 },
    { "PTT_MSG_START_WAVEFORM",                            0x3074 },
    { "PTT_MSG_STOP_WAVEFORM",                             0x3075 },
    { "PTT_MSG_SET_RX_WAVEFORM_GAIN",                      0x3076 },
    { "PTT_MSG_CONFIG_TX_PACKET_GEN",                      0x3081 },
    { "PTT_MSG_START_STOP_TX_PACKET_GEN",                  0x3082 },
    { "PTT_MSG_POLL_TX_PACKET_PROGRESS",                   0x3083 },
    { "PTT_MSG_SET_PAYLOAD_TEMPLATE",                      0x3086 },
    { "PTT_MSG_FRAME_GEN_STOP_IND",                        0x3088 },
    { "PTT_MSG_QUERY_TX_STATUS",                           0x3089 },
    { "PTT_MSG_CLOSE_TPC_LOOP",                            0x30A0 },
    { "PTT_MSG_SET_PACKET_TX_GAIN_TABLE",                  0x30A1 },
    { "PTT_MSG_SET_PACKET_TX_GAIN_INDEX",                  0x30A2 },
    { "PTT_MSG_FORCE_PACKET_TX_GAIN",                      0x30A3 },
    { "PTT_MSG_SET_PWR_INDEX_SOURCE",                      0x30A4 },
    { "PTT_MSG_SET_TX_POWER",                              0x30A5 },
    { "PTT_MSG_GET_TX_POWER_REPORT",                       0x30A7 },
    { "PTT_MSG_SET_POWER_LUT",                             0x30A9 },
    { "PTT_MSG_GET_POWER_LUT",                             0x30AA },
    { "PTT_MSG_GET_PACKET_TX_GAIN_TABLE",                  0x30AB },
    { "PTT_MSG_DISABLE_AGC_TABLES",                        0x30D0 },
    { "PTT_MSG_ENABLE_AGC_TABLES",                         0x30D1 },
    { "PTT_MSG_SET_AGC_TABLES",                            0x30D2 },
    { "PTT_MSG_GET_RX_RSSI",                               0x30D3 },
    { "PTT_MSG_GET_AGC_TABLE",                             0x30D5 },
    { "PTT_MSG_SET_RX_DISABLE_MODE",                       0x30D4 },
    { "PTT_MSG_GET_RX_PKT_COUNTS",                         0x30E0 },
    { "PTT_MSG_RESET_RX_PACKET_STATISTICS",                0x30E2 },
    { "PTT_MSG_GRAB_RAM",                                  0x30F0 },
    { "PTT_MSG_GRAB_RAM_ONE_CHAIN",                        0x30F1 },
    { "PTT_MSG_RX_IQ_CAL",                                 0x3100 },
    { "PTT_MSG_RX_DCO_CAL",                                0x3101 },
    { "PTT_MSG_TX_CARRIER_SUPPRESS_CAL",                   0x3102 },
    { "PTT_MSG_TX_IQ_CAL",                                 0x3103 },
    { "PTT_MSG_SET_TX_CARRIER_SUPPRESS_CORRECT",           0x3110 },
    { "PTT_MSG_GET_TX_CARRIER_SUPPRESS_CORRECT",           0x3111 },
    { "PTT_MSG_SET_TX_IQ_CORRECT",                         0x3112 },
    { "PTT_MSG_GET_TX_IQ_CORRECT",                         0x3113 },
    { "PTT_MSG_SET_RX_IQ_CORRECT",                         0x3114 },
    { "PTT_MSG_GET_RX_IQ_CORRECT",                         0x3115 },
    { "PTT_MSG_SET_RX_DCO_CORRECT",                        0x3116 },
    { "PTT_MSG_GET_RX_DCO_CORRECT",                        0x3117 },
    { "PTT_MSG_SET_TX_IQ_PHASE_NV_TABLE",                  0x3118 },
    { "PTT_MSG_GET_TEMP_ADC",                              0x3202 },
    { "PTT_MSG_READ_RF_REG",                               0x3203 },
    { "PTT_MSG_WRITE_RF_REG",                              0x3204 },
    { "PTT_MSG_SYSTEM_RESET",                              0x32A0 },
    { "PTT_MSG_GET_BUILD_RELEASE_NUMBER",                  0x32A2 },
    { "PTT_MSG_ADAPTER_DISABLED_RSP",                      0x32A3 },
    { "PTT_MSG_ENABLE_ADAPTER",                            0x32A4 },
    { "PTT_MSG_DISABLE_ADAPTER",                           0x32A5 },
    { "PTT_MSG_PAUSE_RSP",                                 0x32A6 },
    { "PTT_MSG_CONTINUE_RSP",                              0x32A7 },
    { "PTT_MSG_HALPHY_INIT",                               0x32A8 },
    { "PTT_MSG_TEST_RXIQ_CAL",                             0x32A9 },
    { "PTT_MSG_START_TONE_GEN",                            0x32AA },
    { "PTT_MSG_STOP_TONE_GEN",                             0x32AB },
    { "PTT_MSG_RX_IM2_CAL",                                0x32AC },
    { "PTT_MSG_SET_RX_IM2_CORRECT",                        0x32AD },
    { "PTT_MSG_GET_RX_IM2_CORRECT",                        0x32AE },
    { "PTT_MSG_TEST_DPD_CAL",                              0x32AF },
    { "PTT_MSG_SET_CALCONTROL_BITMAP",                     0x32B0 }
};

static void dumpPttMsg(tPttMsgbuffer *pttMsg)
{
    unsigned int i;

    printk(KERN_ERR "0x%4X", pttMsg->msgId);
    printk(KERN_ERR "0x%4X", pttMsg->msgBodyLength);
    printk(KERN_ERR "0x%2X", pttMsg->msgResponse);

    for (i = 0; i < pttMsg->msgBodyLength; i+=4)
    {
        printk(KERN_ERR "%3d: 0x%2X 0x%2X 0x%2X 0x%2X\n", i,
               *(((unsigned char *)&pttMsg->msgBody) + i + 0),
               *(((unsigned char *)&pttMsg->msgBody) + i + 1),
               *(((unsigned char *)&pttMsg->msgBody) + i + 2),
               *(((unsigned char *)&pttMsg->msgBody) + i + 3)
              );
    }
}
*/
void pttProcessMsg(tpAniSirGlobal pMac, tPttMsgbuffer *pttMsg)
{
    eQWPttStatus retVal = PTT_STATUS_SUCCESS;

    uPttMsgs *msgBody = (uPttMsgs *)&(pttMsg->msgBody);
    //FOR DEBUG ONLY dumpPttMsg(pttMsg);

    assert(sizeof(tANI_BOOLEAN) == 1);

    switch (pttMsg->msgId)
    {
        case PTT_MSG_INIT:
        {
            tPttModuleVariables *ptt;

            pttMsgInit(pMac, &ptt);

            memcpy((tANI_U8 *)&(msgBody->MsgInit.ptt), ptt, sizeof(tPttModuleVariables));

            {
                /* Use NTOH on message body in response
                    //statistics have been reset to 0, so don't reorder those
                */
                tANI_U32 i, k;

                for (i = 0; i < PHY_MAX_TX_CHAINS; i++)
                {
                    tTxGain *gainPtr;

                    gainPtr = &msgBody->MsgInit.ptt.forcedTxGain[i];
                    NTOHL(gainPtr->coarsePwr);
                    NTOHL(gainPtr->finePwr);

                    for (k = 0; k < TPC_MEM_GAIN_LUT_DEPTH; k++)
                    {
                        gainPtr = &msgBody->MsgInit.ptt.tpcGainLut[i][k];
                        NTOHL(gainPtr->coarsePwr);
                        NTOHL(gainPtr->finePwr);
                    }
                }
                NTOHL(msgBody->MsgInit.ptt.frameGenParams.numTestPackets);
                NTOHL(msgBody->MsgInit.ptt.frameGenParams.interFrameSpace);
                NTOHL(msgBody->MsgInit.ptt.frameGenParams.rate);
                NTOHL(msgBody->MsgInit.ptt.frameGenParams.payloadContents);
                NTOHS(msgBody->MsgInit.ptt.frameGenParams.payloadLength);
            }
            break;
        }

        case PTT_MSG_BLANK_NV:
        {
            retVal = pttBlankNv(pMac);
            break;
        }

        case PTT_MSG_GET_NV_TABLE:
        {
            HTONL(msgBody->GetNvTable.nvTable);

            retVal = pttGetNvTable(pMac, msgBody->GetNvTable.nvTable, &(msgBody->GetNvTable.tableData));
            halByteSwapNvTable(pMac, msgBody->GetNvTable.nvTable, &(msgBody->GetNvTable.tableData));
            NTOHL(msgBody->GetNvTable.nvTable);

            break;
        }

        case PTT_MSG_SET_NV_TABLE:
        {
            HTONL(msgBody->SetNvTable.nvTable);
            halByteSwapNvTable(pMac, msgBody->SetNvTable.nvTable, &(msgBody->SetNvTable.tableData));
            retVal = pttSetNvTable(pMac, msgBody->SetNvTable.nvTable, &(msgBody->SetNvTable.tableData));
            NTOHL(msgBody->SetNvTable.nvTable);

            break;
        }

        case PTT_MSG_DEL_NV_TABLE:
        {
            HTONL(msgBody->DelNvTable.nvTable);

            retVal = pttDelNvTable(pMac, msgBody->DelNvTable.nvTable);
            NTOHL(msgBody->DelNvTable.nvTable);

            break;
        }

        case PTT_MSG_GET_NV_FIELD:
        {
            HTONL(msgBody->GetNvField.nvField);

            retVal = pttGetNvField(pMac, msgBody->GetNvField.nvField, &(msgBody->GetNvField.fieldData));

            {
                /* Use NTOH on message body in response

                */
                switch (msgBody->GetNvField.nvField)
                {
                    case NV_COMMON_PRODUCT_ID:
                        //all of these fields are 16 bits
                        NTOHS(*(tANI_U16 *)&msgBody->GetNvField.fieldData);
                        break;


                    case NV_COMMON_NUM_OF_TX_CHAINS:
                    case NV_COMMON_NUM_OF_RX_CHAINS:
                    case NV_COMMON_PRODUCT_BANDS:
                    case NV_COMMON_MAC_ADDR:
                    default:
                        // fields composed of single bytes - nothing to reorder
                        break;
                }
            }

            NTOHL(msgBody->GetNvField.nvField);
            break;
        }

        case PTT_MSG_SET_NV_FIELD:
        {
            {
                /* Use HTON on message body params

                */
                HTONL(msgBody->SetNvField.nvField);

                switch (msgBody->GetNvField.nvField)
                {
                    case NV_COMMON_PRODUCT_ID:
                        //all of these fields are 16 bits
                        HTONS(*(tANI_U16 *)&msgBody->GetNvField.fieldData);
                        break;

                    case NV_COMMON_NUM_OF_TX_CHAINS:
                    case NV_COMMON_NUM_OF_RX_CHAINS:
                    case NV_COMMON_PRODUCT_BANDS:
                    case NV_COMMON_MAC_ADDR:
                    default:
                        // fields composed of single bytes - nothing to reorder
                        break;
                }
            }

            retVal = pttSetNvField(pMac, msgBody->SetNvField.nvField, &(msgBody->SetNvField.fieldData));
            NTOHL(msgBody->SetNvField.nvField);
            break;
        }

        case PTT_MSG_STORE_NV_TABLE:
        {
            HTONL(msgBody->StoreNvTable.nvTable);

            retVal = pttStoreNvTable(pMac, msgBody->StoreNvTable.nvTable);
            NTOHL(msgBody->StoreNvTable.nvTable);

            break;
        }

        case PTT_MSG_SET_REG_DOMAIN:
        {
            HTONL(msgBody->SetRegDomain.regDomainId);

            retVal = pttSetRegDomain(pMac, msgBody->SetRegDomain.regDomainId);
            NTOHL(msgBody->SetRegDomain.regDomainId);

            break;
        }

        case PTT_MSG_DBG_READ_REGISTER:
        {
            HTONL(msgBody->DbgReadRegister.regAddr);

            retVal = pttDbgReadRegister(pMac, msgBody->DbgReadRegister.regAddr,
                                &(msgBody->DbgReadRegister.regValue));

            NTOHL(msgBody->DbgReadRegister.regAddr);
            NTOHL(msgBody->DbgReadRegister.regValue);
            break;
        }

        case PTT_MSG_DBG_WRITE_REGISTER:
        {
            HTONL(msgBody->DbgWriteRegister.regAddr);
            HTONL(msgBody->DbgWriteRegister.regValue);

            retVal = pttDbgWriteRegister(pMac, msgBody->DbgWriteRegister.regAddr,
                                msgBody->DbgWriteRegister.regValue);

            NTOHL(msgBody->DbgWriteRegister.regAddr);
            NTOHL(msgBody->DbgWriteRegister.regValue);
            break;
        }

        case PTT_MSG_DBG_READ_MEMORY:
        {
            HTONL(msgBody->DbgReadMemory.memAddr);
            HTONL(msgBody->DbgReadMemory.nBytes);

            retVal = palReadDeviceMemory(pMac->hHdd, msgBody->DbgReadMemory.memAddr, (tANI_U8 *)&msgBody->DbgReadMemory.pMemBuf[0], msgBody->DbgReadMemory.nBytes);

            if (((tANI_U32)&msgBody->DbgReadMemory.pMemBuf[0] % 4 == 0) ||
                (msgBody->DbgReadMemory.nBytes <= (PTT_READ_MEM_MAX * 4))
               )
            {
                tANI_U32 i;

                for (i = 0; i < msgBody->DbgReadMemory.nBytes / 4; i++)
                {
                    NTOHL(msgBody->DbgReadMemory.pMemBuf[i]);
                }
            }
            else
            {
                retVal = PTT_STATUS_FAILURE;
            }

            NTOHL(msgBody->DbgReadMemory.memAddr);
            NTOHL(msgBody->DbgReadMemory.nBytes);
            break;
        }

        case PTT_MSG_DBG_WRITE_MEMORY:
        {
            HTONL(msgBody->DbgWriteMemory.memAddr);
            HTONL(msgBody->DbgWriteMemory.nBytes);

            if (((tANI_U32)&msgBody->DbgWriteMemory.pMemBuf[0] % 4 == 0) ||
                (msgBody->DbgWriteMemory.nBytes <= (PTT_READ_MEM_MAX * 4))
               )
            {
                tANI_U32 i;

                for (i = 0; i < msgBody->DbgWriteMemory.nBytes / 4; i++)
                {
                    HTONL(msgBody->DbgWriteMemory.pMemBuf[i]);
                }
            }
            else
            {
                retVal = PTT_STATUS_FAILURE;
            }

            retVal = palWriteDeviceMemory(pMac->hHdd, msgBody->DbgWriteMemory.memAddr, (tANI_U8 *)&msgBody->DbgWriteMemory.pMemBuf[0], msgBody->DbgWriteMemory.nBytes);

/*
                {
                    //verify what was written
                    tANI_U32 *verifyBuf;

                    if (palAllocateMemory(pMac->hHdd, &verifyBuf, msgBody->DbgWriteMemory.nBytes) == eHAL_STATUS_SUCCESS)
                    {
                        palReadDeviceMemory(pMac->hHdd, msgBody->DbgWriteMemory.memAddr, (tANI_U8 *)verifyBuf, msgBody->DbgWriteMemory.nBytes);

                        if (memcmp(verifyBuf, (tANI_U8 *)&msgBody->DbgWriteMemory.pMemBuf[0], msgBody->DbgWriteMemory.nBytes) != 0)
                        {
                            //doesn't match -
                            phyLog(pMac, LOGE, "ERROR: Memory written not equal in readback verification\n");
                        }

                        if (msgBody->DbgWriteMemory.nBytes == (28 * 4))
                        {
                            //parse frame header contents
                            tANI_U32 frameMem[6];
                            tANI_U8 *byte;
                            tANI_U32 i;

                            for (i = 0; i < 6; i += 2)
                            {
                                frameMem[i/2] = verifyBuf[i + 1];
                            }

                            {
                                byte = (tANI_U8 *)&frameMem[3];

                                for (i = 6; i < 28; i += 2)
                                {
                                    *byte = (tANI_U8)verifyBuf[i + 1];
                                    byte++;
                                }
                            }

                            printFrameFields(pMac, (tPhyDbgFrame *)&frameMem[0], HAL_PHY_RATE_11B_LONG_1_MBPS);
                        }


                        phyLog(pMac, LOGE, "Memory at 0x%08X:\n", msgBody->DbgWriteMemory.memAddr);
                        {
                            tANI_U32 dwordData;
                            tANI_U32 dwordIndex;

                            for (dwordIndex = 0; dwordIndex < (msgBody->DbgWriteMemory.nBytes / 4); dwordIndex += 1)
                            {
                                dwordData = verifyBuf[dwordIndex];
                                phyLog(pMac, LOGE, "0x%04X: %08X\n", msgBody->DbgWriteMemory.memAddr + (sizeof(tANI_U32) * dwordIndex), dwordData);
                            }
                        }



                        palFreeMemory(pMac->hHdd, verifyBuf);
                    }
            }
*/

            NTOHL(msgBody->DbgWriteMemory.memAddr);
            NTOHL(msgBody->DbgWriteMemory.nBytes);
            break;
        }

        case PTT_MSG_ENABLE_CHAINS:
        {
            HTONL(msgBody->EnableChains.chainSelect);
            retVal = pttEnableChains(pMac, msgBody->EnableChains.chainSelect);
            NTOHL(msgBody->EnableChains.chainSelect);

            break;
        }

        case PTT_MSG_SET_CHANNEL:
        {
            HTONL(msgBody->SetChannel.chId);
            HTONL(msgBody->SetChannel.cbState);

            retVal = pttSetChannel(pMac, msgBody->SetChannel.chId,
                                msgBody->SetChannel.cbState);

            NTOHL(msgBody->SetChannel.chId);
            NTOHL(msgBody->SetChannel.cbState);

            break;
        }

        case PTT_MSG_SET_WAVEFORM:
        {
            tANI_U32 i;
            {
                /* Use HTON on message body params

                */

                HTONS(msgBody->SetWaveform.numSamples);

                for (i = 0; i < msgBody->SetWaveform.numSamples; i++)
                {
                    HTONS(msgBody->SetWaveform.waveform[i].I);
                    HTONS(msgBody->SetWaveform.waveform[i].Q);
                }
            }

            retVal = pttSetWaveform(pMac, (tWaveformSample *)&(msgBody->SetWaveform.waveform),
                                msgBody->SetWaveform.numSamples, msgBody->SetWaveform.clk80);

            NTOHS(msgBody->SetWaveform.numSamples);

            for (i = 0; i < msgBody->SetWaveform.numSamples; i++)
            {
                NTOHS(msgBody->SetWaveform.waveform[i].I);
                NTOHS(msgBody->SetWaveform.waveform[i].Q);
            }

            break;
        }

        case PTT_MSG_SET_TX_WAVEFORM_GAIN:
        {
            HTONL(msgBody->SetTxWaveformGain.txChain);

            retVal = pttSetTxWaveformGain(pMac, msgBody->SetTxWaveformGain.txChain, msgBody->SetTxWaveformGain.gain);

            NTOHL(msgBody->SetTxWaveformGain.txChain);
            break;
        }

        case PTT_MSG_SET_RX_WAVEFORM_GAIN:
        {
            HTONL(msgBody->SetRxWaveformGain.rxChain);

            retVal = pttSetRxWaveformGain(pMac, msgBody->SetRxWaveformGain.rxChain, msgBody->SetRxWaveformGain.gain);

            NTOHL(msgBody->SetRxWaveformGain.rxChain);
            break;
        }

        case PTT_MSG_GET_WAVEFORM_POWER_ADC:
        {
            retVal = pttGetWaveformPowerAdc(pMac, &(msgBody->GetWaveformPowerAdc.txPowerAdc));
            break;
        }

        case PTT_MSG_START_WAVEFORM:
        {
#ifdef VERIFY_HALPHY_SIMV_MODEL
            HTONL(msgBody->StartWaveform.numSamples);
            HTONL(msgBody->StartWaveform.startIndex);

            retVal = pttStartWaveformPlayback(pMac, msgBody->StartWaveform.startIndex,
                                                        msgBody->StartWaveform.numSamples);

            NTOHL(msgBody->StartWaveform.numSamples);
            NTOHL(msgBody->StartWaveform.startIndex);
#else
            retVal = pttStartWaveform(pMac);
#endif
            break;
        }

        case PTT_MSG_STOP_WAVEFORM:
        {
            retVal = pttStopWaveform(pMac);
            break;
        }

        case PTT_MSG_CONFIG_TX_PACKET_GEN:
        {
            HTONL(msgBody->ConfigTxPacketGen.frameParams.numTestPackets);
            HTONL(msgBody->ConfigTxPacketGen.frameParams.interFrameSpace);
            HTONL(msgBody->ConfigTxPacketGen.frameParams.rate);
            HTONL(msgBody->ConfigTxPacketGen.frameParams.payloadContents);
            HTONS(msgBody->ConfigTxPacketGen.frameParams.payloadLength);
            HTONL(msgBody->ConfigTxPacketGen.frameParams.preamble);

            retVal = pttConfigTxPacketGen(pMac, msgBody->ConfigTxPacketGen.frameParams);

            NTOHL(msgBody->ConfigTxPacketGen.frameParams.numTestPackets);
            NTOHL(msgBody->ConfigTxPacketGen.frameParams.interFrameSpace);
            NTOHL(msgBody->ConfigTxPacketGen.frameParams.rate);
            NTOHL(msgBody->ConfigTxPacketGen.frameParams.payloadContents);
            NTOHS(msgBody->ConfigTxPacketGen.frameParams.payloadLength);
            NTOHL(msgBody->ConfigTxPacketGen.frameParams.preamble);
            break;
        }

        case PTT_MSG_START_STOP_TX_PACKET_GEN:
        {
            retVal = pttStartStopTxPacketGen(pMac, msgBody->StartStopTxPacketGen.startStop);
            break;
        }

        case PTT_MSG_QUERY_TX_STATUS:
        {
            retVal = pttQueryTxStatus(pMac, &(msgBody->QueryTxStatus.numFrames), &(msgBody->QueryTxStatus.status));

            NTOHL(msgBody->QueryTxStatus.numFrames.legacy);
            NTOHL(msgBody->QueryTxStatus.numFrames.gfSimo20);
            NTOHL(msgBody->QueryTxStatus.numFrames.gfMimo20);
            NTOHL(msgBody->QueryTxStatus.numFrames.mmSimo20);
            NTOHL(msgBody->QueryTxStatus.numFrames.mmMimo20);
            NTOHL(msgBody->QueryTxStatus.numFrames.txbShort);
            NTOHL(msgBody->QueryTxStatus.numFrames.txbLong);
#ifndef VERIFY_HALPHY_SIMV_MODEL
            NTOHL(msgBody->QueryTxStatus.numFrames.txbSlr);
#endif

            break;
}

        case PTT_MSG_CLOSE_TPC_LOOP:
        {
            retVal = pttCloseTpcLoop(pMac, msgBody->CloseTpcLoop.tpcClose);
            break;
        }

        case PTT_MSG_SET_PACKET_TX_GAIN_TABLE:
        {
            HTONL(msgBody->SetPacketTxGainTable.txChain);

            retVal = pttSetPacketTxGainTable(pMac, msgBody->SetPacketTxGainTable.txChain,
                                msgBody->SetPacketTxGainTable.minIndex,
                                msgBody->SetPacketTxGainTable.maxIndex,
                                msgBody->SetPacketTxGainTable.gainTable);
            NTOHL(msgBody->SetPacketTxGainTable.txChain);

            break;
        }

        case PTT_MSG_GET_PACKET_TX_GAIN_TABLE:
        {
            HTONL(msgBody->GetPacketTxGainTable.txChain);

            retVal = pttGetPacketTxGainTable(pMac, msgBody->GetPacketTxGainTable.txChain, msgBody->GetPacketTxGainTable.gainTable);
            NTOHL(msgBody->GetPacketTxGainTable.txChain);

            break;
        }

        case PTT_MSG_SET_PACKET_TX_GAIN_INDEX:
        {
            retVal = pttSetPacketTxGainIndex(pMac, msgBody->SetPacketTxGainIndex.index);
            break;
        }

        case PTT_MSG_FORCE_PACKET_TX_GAIN:
        {
            HTONL(msgBody->ForcePacketTxGain.txChain);

            retVal = pttForcePacketTxGain(pMac, msgBody->ForcePacketTxGain.txChain, msgBody->ForcePacketTxGain.gain);
            NTOHL(msgBody->ForcePacketTxGain.txChain);

            break;
        }



        case PTT_MSG_SET_PWR_INDEX_SOURCE:
        {
            HTONL(msgBody->SetPwrIndexSource.indexSource);

            retVal = pttSetPwrIndexSource(pMac, msgBody->SetPwrIndexSource.indexSource);
            NTOHL(msgBody->SetPwrIndexSource.indexSource);

            break;
        }

        case PTT_MSG_UPDATE_TPC_SPLIT_LUT:
        {
            HTONL(msgBody->UpdateTpcSplitLut.pwrRange);
            HTONL(msgBody->UpdateTpcSplitLut.splitIdx);

            retVal = pttUpdateTpcSplitLut(pMac, msgBody->UpdateTpcSplitLut.pwrRange, msgBody->UpdateTpcSplitLut.splitIdx);
            NTOHL(msgBody->UpdateTpcSplitLut.pwrRange);
            NTOHL(msgBody->UpdateTpcSplitLut.splitIdx);

            break;
        }

        case PTT_MSG_SET_TX_POWER:
        {
            HTONS(msgBody->SetTxPower.dbmPwr);

            retVal = pttSetTxPower(pMac, msgBody->SetTxPower.dbmPwr);
            NTOHS(msgBody->SetTxPower.dbmPwr);

            break;
        }

        case PTT_MSG_GET_TX_POWER_REPORT:
        {
            retVal = pttGetTxPowerReport(pMac, &(msgBody->GetTxPowerReport));

            NTOHL(msgBody->GetTxPowerReport.rate);
            NTOHL(msgBody->GetTxPowerReport.cbState);
            NTOHS(msgBody->GetTxPowerReport.txChains[PHY_TX_CHAIN_0].indexMinMatch.abs.reported);
            NTOHS(msgBody->GetTxPowerReport.txChains[PHY_TX_CHAIN_0].indexMaxMatch.abs.reported);
            NTOHS(msgBody->GetTxPowerReport.txChains[PHY_TX_CHAIN_0].output.abs.reported);
            break;
        }

        case PTT_MSG_SET_POWER_LUT:
        {
            HTONL(msgBody->SetPowerLut.txChain);

            retVal = pttSetPowerLut(pMac, msgBody->SetPowerLut.txChain,
                                msgBody->SetPowerLut.minIndex,
                                msgBody->SetPowerLut.maxIndex,
                                msgBody->SetPowerLut.powerLut);
            NTOHL(msgBody->SetPowerLut.txChain);

            break;
        }

        case PTT_MSG_GET_POWER_LUT:
        {
            HTONL(msgBody->GetPowerLut.txChain);

            retVal = pttGetPowerLut(pMac, msgBody->GetPowerLut.txChain, msgBody->GetPowerLut.powerLut);
            NTOHL(msgBody->GetPowerLut.txChain);

            break;
        }

        case PTT_MSG_DISABLE_AGC_TABLES:
        {
            retVal = pttDisableAgcTables(pMac, msgBody->DisableAgcTables.gains);
            break;
        }

        case PTT_MSG_ENABLE_AGC_TABLES:
        {
            retVal = pttEnableAgcTables(pMac, msgBody->EnableAgcTables.enables);
            break;
        }

        case PTT_MSG_GET_RX_RSSI:
        {
            pttGetRxRssi(pMac, &(msgBody->GetRxRssi.rssi));
            break;
        }

        case PTT_MSG_GET_UNI_CAST_MAC_PKT_RX_RSSI:
        {
            pttGetUnicastMacPktRxRssi(pMac, &(msgBody->GetUnicastMacPktRxRssi.rssi));
            break;
        }

        case PTT_MSG_SET_RX_DISABLE_MODE:
        {
            retVal = pttSetRxDisableMode(pMac, msgBody->SetRxDisableMode.disabled);
            break;
        }

        case PTT_MSG_GET_RX_PKT_COUNTS:
        {
            pttGetRxPktCounts(pMac, &(msgBody->GetRxPktCounts.counters));

            NTOHL(msgBody->GetRxPktCounts.counters.totalRxPackets);
            NTOHL(msgBody->GetRxPktCounts.counters.totalMacRxPackets);
            NTOHL(msgBody->GetRxPktCounts.counters.totalMacFcsErrPackets);
            break;
        }

        case PTT_MSG_RESET_RX_PACKET_STATISTICS:
        {
            pttResetRxPacketStatistics(pMac);
            break;
        }

        case PTT_MSG_GRAB_RAM:
        {
            {
                /* Use HTON on message body params */
                HTONL(msgBody->GrabRam.startSample);
                HTONL(msgBody->GrabRam.numSamples);
                HTONL(msgBody->GrabRam.sampleType);
            }

            retVal = pttGrabRam(pMac, msgBody->GrabRam.startSample,
                                 msgBody->GrabRam.numSamples,
                                 msgBody->GrabRam.sampleType,
                                 msgBody->GrabRam.grabRam
                               );

            if (retVal == eHAL_STATUS_SUCCESS)
            {
                /* Use NTOH on message body in response */
                tANI_U32 i;
                for (i = 0; i < msgBody->GrabRam.numSamples; i++)
                {
                    NTOHS(msgBody->GrabRam.grabRam[i].rx0.I);
                    NTOHS(msgBody->GrabRam.grabRam[i].rx0.Q);
                }
                NTOHL(msgBody->GrabRam.startSample);
                NTOHL(msgBody->GrabRam.numSamples);
                NTOHL(msgBody->GrabRam.sampleType);
            }
            break;
        }

        case PTT_MSG_RX_IQ_CAL:
        {
            HTONL(msgBody->RxIqCal.gain);

            retVal = pttRxIqCal(pMac, &(msgBody->RxIqCal.calValues), msgBody->RxIqCal.gain);

            NTOHL(msgBody->RxIqCal.gain);
            NTOHS(msgBody->RxIqCal.calValues.iq[PHY_RX_CHAIN_0].center);
            NTOHS(msgBody->RxIqCal.calValues.iq[PHY_RX_CHAIN_0].offCenter);
            NTOHS(msgBody->RxIqCal.calValues.iq[PHY_RX_CHAIN_0].imbalance);
            break;
        }

        case PTT_MSG_TX_IQ_CAL:
        {
            HTONL(msgBody->TxIqCal.gain);

            retVal = pttTxIqCal(pMac, &(msgBody->TxIqCal.calValues), msgBody->TxIqCal.gain);

            NTOHL(msgBody->TxIqCal.gain);
            NTOHS(msgBody->TxIqCal.calValues.iq[PHY_TX_CHAIN_0].center);
            NTOHS(msgBody->TxIqCal.calValues.iq[PHY_TX_CHAIN_0].offCenter);
            NTOHS(msgBody->TxIqCal.calValues.iq[PHY_TX_CHAIN_0].imbalance);
            break;
        }

        case PTT_MSG_RX_DCO_CAL:
        {
            retVal = pttRxDcoCal(pMac, &(msgBody->RxDcoCal.calValues), msgBody->RxDcoCal.gain);

            break;
        }

        case PTT_MSG_RX_IM2_CAL:
        {
            HTONL(msgBody->RxIm2Cal.im2CalOnly);

            retVal = pttRxIm2Cal(pMac, &(msgBody->RxIm2Cal.calValues), msgBody->RxIm2Cal.im2CalOnly);
            NTOHL(msgBody->RxIm2Cal.im2CalOnly);

            break;
        }

        case PTT_MSG_TX_CARRIER_SUPPRESS_CAL:
        {
            HTONL(msgBody->TxCarrierSuppressCal.gain);

            retVal = pttTxCarrierSuppressCal(pMac, &(msgBody->TxCarrierSuppressCal.calValues), msgBody->TxCarrierSuppressCal.gain);
            NTOHL(msgBody->TxCarrierSuppressCal.gain);

            break;
        }

        case PTT_MSG_EXECUTE_INITIAL_CALS:
        {
            retVal = pttExecuteInitialCals(pMac);
            break;
        }

        case PTT_MSG_HDET_CAL:
        {
            retVal = pttHdetCal(pMac, &(msgBody->HdetCal.hdetCalValues));

            break;
        }

        case PTT_MSG_SET_TX_CARRIER_SUPPRESS_CORRECT:
        {
            HTONL(msgBody->SetTxCarrierSuppressCorrect.gain);

            retVal = pttSetTxCarrierSuppressCorrect(pMac, msgBody->SetTxCarrierSuppressCorrect.calValues, msgBody->SetTxCarrierSuppressCorrect.gain);
            NTOHL(msgBody->SetTxCarrierSuppressCorrect.gain);

            break;
        }

        case PTT_MSG_GET_TX_CARRIER_SUPPRESS_CORRECT:
        {
            HTONL(msgBody->GetTxCarrierSuppressCorrect.gain);

            pttGetTxCarrierSuppressCorrect(pMac, &(msgBody->GetTxCarrierSuppressCorrect.calValues), msgBody->GetTxCarrierSuppressCorrect.gain);
            NTOHL(msgBody->GetTxCarrierSuppressCorrect.gain);
            break;
        }

        case PTT_MSG_SET_TX_IQ_CORRECT:
        {
            HTONL(msgBody->SetTxIqCorrect.gain);
            HTONS(msgBody->SetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].center);
            HTONS(msgBody->SetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].offCenter);
            HTONS(msgBody->SetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].imbalance);

            retVal = pttSetTxIqCorrect(pMac, msgBody->SetTxIqCorrect.calValues,
                                             msgBody->SetTxIqCorrect.gain
                                           );
            NTOHL(msgBody->SetTxIqCorrect.gain);
            NTOHS(msgBody->SetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].center);
            NTOHS(msgBody->SetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].offCenter);
            NTOHS(msgBody->SetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].imbalance);

            break;
        }

        case PTT_MSG_GET_TX_IQ_CORRECT:
        {
            HTONL(msgBody->GetTxIqCorrect.gain);

            retVal = pttGetTxIqCorrect(pMac, &(msgBody->GetTxIqCorrect.calValues), msgBody->GetTxIqCorrect.gain);

            NTOHL(msgBody->GetTxIqCorrect.gain);
            NTOHS(msgBody->GetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].center);
            NTOHS(msgBody->GetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].offCenter);
            NTOHS(msgBody->GetTxIqCorrect.calValues.iq[PHY_TX_CHAIN_0].imbalance);
            break;
        }

        case PTT_MSG_SET_RX_IQ_CORRECT:
        {
            HTONL(msgBody->SetRxIqCorrect.gain);
            HTONS(msgBody->SetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].center);
            HTONS(msgBody->SetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].offCenter);
            HTONS(msgBody->SetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].imbalance);

            retVal = pttSetRxIqCorrect(pMac, msgBody->SetRxIqCorrect.calValues,
                                             msgBody->SetRxIqCorrect.gain
                                      );
            NTOHL(msgBody->SetRxIqCorrect.gain);
            NTOHS(msgBody->SetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].center);
            NTOHS(msgBody->SetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].offCenter);
            NTOHS(msgBody->SetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].imbalance);
            break;
        }

        case PTT_MSG_GET_RX_IQ_CORRECT:
        {
            HTONL(msgBody->GetRxIqCorrect.gain);

            retVal = pttGetRxIqCorrect(pMac, &(msgBody->GetRxIqCorrect.calValues),
                                             msgBody->GetRxIqCorrect.gain
                                      );

            NTOHL(msgBody->GetRxIqCorrect.gain);
            NTOHS(msgBody->GetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].center);
            NTOHS(msgBody->GetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].offCenter);
            NTOHS(msgBody->GetRxIqCorrect.calValues.iq[PHY_RX_CHAIN_0].imbalance);
            break;
        }

        case PTT_MSG_SET_RX_DCO_CORRECT:
        {
            pttSetRxDcoCorrect(pMac, msgBody->SetRxDcoCorrect.calValues,
                                     msgBody->SetRxDcoCorrect.gain
                              );
            break;
        }

        case PTT_MSG_GET_RX_DCO_CORRECT:
        {
            pttGetRxDcoCorrect(pMac, &(msgBody->GetRxDcoCorrect.calValues),
                                     msgBody->GetRxDcoCorrect.gain
                              );
            break;
        }

        case PTT_MSG_SET_RX_IM2_CORRECT:
        {
            HTONL(msgBody->SetRxIm2Correct.dummy);

            pttSetRxIm2Correct(pMac, msgBody->SetRxIm2Correct.calValues,
                                     msgBody->SetRxIm2Correct.dummy
                              );
            NTOHL(msgBody->SetRxIm2Correct.dummy);
            break;
        }

        case PTT_MSG_GET_RX_IM2_CORRECT:
        {
            HTONL(msgBody->GetRxIm2Correct.dummy);

            pttGetRxIm2Correct(pMac, &(msgBody->GetRxIm2Correct.calValues),
                                     msgBody->GetRxIm2Correct.dummy
                              );
            NTOHL(msgBody->GetRxIm2Correct.dummy);
            break;
        }

        case PTT_MSG_GET_TEMP_ADC:
        {
            retVal = pttGetTempAdc(pMac, msgBody->GetTempAdc.tempSensor, &(msgBody->GetTempAdc.tempAdc));
            break;
        }

        case PTT_MSG_START_TONE_GEN:
        {
#ifdef VERIFY_HALPHY_SIMV_MODEL
            retVal = pttStartToneGen(pMac, msgBody->StartToneGen.lutIdx, msgBody->StartToneGen.band);
#endif
            break;
        }

        case PTT_MSG_STOP_TONE_GEN:
        {
#ifdef VERIFY_HALPHY_SIMV_MODEL
            HTONL(msgBody->StopToneGen.option);
            retVal = pttStopToneGen(pMac, msgBody->StopToneGen.option);
            NTOHL(msgBody->StopToneGen.option);
#endif
            break;
        }

        case PTT_MSG_READ_RF_REG:
        {
            retVal = pttReadRfField(pMac, msgBody->ReadRfField.addr,
                           msgBody->ReadRfField.mask, msgBody->ReadRfField.shift, &(msgBody->ReadRfField.value));
            break;
        }

        case PTT_MSG_WRITE_RF_REG:
        {
            retVal = pttWriteRfField(pMac, msgBody->WriteRfField.addr,
                           msgBody->WriteRfField.mask, msgBody->WriteRfField.shift, msgBody->WriteRfField.value);
            break;
        }

        case PTT_MSG_DEEP_SLEEP:
        {
            retVal = pttDeepSleep(pMac);
            break;
        }

        case PTT_MSG_READ_SIF_BAR4_REGISTER:
        {
            HTONL(msgBody->ReadSifBar4Register.sifRegAddr);
#ifdef FIXME_VOLANS
            retVal = palReadBAR4Register(pMac->hHdd, msgBody->ReadSifBar4Register.sifRegAddr,
                                &(msgBody->ReadSifBar4Register.sifRegValue));
#endif
#ifdef ANI_PHY_DEBUG
            {
                phyLog(pMac, LOGE, "0x%x : 0x%x\n", msgBody->ReadSifBar4Register.sifRegAddr, msgBody->ReadSifBar4Register.sifRegValue);
            }
#endif

            NTOHL(msgBody->ReadSifBar4Register.sifRegAddr);
            NTOHL(msgBody->ReadSifBar4Register.sifRegValue);

            break;
        }

        case PTT_MSG_WRITE_SIF_BAR4_REGISTER:
        {
            HTONL(msgBody->ReadSifBar4Register.sifRegAddr);
            HTONL(msgBody->ReadSifBar4Register.sifRegValue);
#ifdef FIXME_VOLANS
            retVal = palWriteBAR4Register(pMac->hHdd, msgBody->ReadSifBar4Register.sifRegAddr,
                                msgBody->ReadSifBar4Register.sifRegValue);
#endif
#ifdef ANI_PHY_DEBUG
            {
                tANI_U32 dumpVal = 0;
#ifdef FIXME_VOLANS
                palReadBAR4Register(pMac->hHdd, msgBody->ReadSifBar4Register.sifRegAddr, &dumpVal);
#endif
                phyLog(pMac, LOGE, "0x%x : 0x%x\n", msgBody->ReadSifBar4Register.sifRegAddr, dumpVal);
            }
#endif
            NTOHL(msgBody->ReadSifBar4Register.sifRegAddr);
            NTOHL(msgBody->ReadSifBar4Register.sifRegValue);

            break;
        }


        case PTT_MSG_SYSTEM_RESET:
        {
            retVal = pttSystemReset(pMac);
            break;
        }


        case PTT_MSG_SET_CALCONTROL_BITMAP:
        {
#ifdef VERIFY_HALPHY_SIMV_MODEL
            HTONL(msgBody->SetCalControlBitmap.option);

            calDebugSetCalControlBitmap();

            NTOHL(msgBody->SetCalControlBitmap.option);
#endif
            break;
        }

        case PTT_MSG_HALPHY_INIT:
        {
#ifdef VERIFY_HALPHY_SIMV_MODEL
            HTONL(msgBody->InitOption.option);

            HalPhy_Init();

            NTOHL(msgBody->InitOption.option);
#endif
            break;
        }

        case PTT_MSG_TEST_RXIQ_CAL:
        {
#ifdef VERIFY_HALPHY_SIMV_MODEL
            HTONL(msgBody->RxIQTest.option);

            testRxIqCal();

            NTOHL(msgBody->RxIQTest.option);
#endif
            break;
        }

        case PTT_MSG_TEST_DPD_CAL:
        {
#ifdef VERIFY_HALPHY_SIMV_MODEL
            HTONL(msgBody->DpdTest.txGain);

            testDpdCal(msgBody->DpdTest.txGain);

            NTOHL(msgBody->DpdTest.txGain);
#endif
            break;
        }

        case PTT_MSG_GET_BUILD_RELEASE_NUMBER:
        {
            msgBody->GetBuildReleaseNumber.relParams.drvMjr = QWLAN_VERSION_MAJOR;
            msgBody->GetBuildReleaseNumber.relParams.drvMnr = QWLAN_VERSION_MINOR;
            msgBody->GetBuildReleaseNumber.relParams.drvPtch = QWLAN_VERSION_PATCH;
            msgBody->GetBuildReleaseNumber.relParams.drvBld = QWLAN_VERSION_BUILD;
            vos_mem_copy((v_VOID_t*)&msgBody->GetBuildReleaseNumber.relParams.fwVer,(v_VOID_t*)&pMac->hal.FwParam.fwVersion, sizeof(FwVersionInfo));

            break;
        }

        case PTT_MSG_GET_RF_VERSION:
        {
            retVal = pttDbgReadRegister(pMac, QWLAN_RFAPB_REV_ID_REG,
                                &(msgBody->GetRFVersion.revId));

            NTOHL(msgBody->GetRFVersion.revId);

            break;
        }

        default:
            break;
    }

/* KEEP AROUND FOR DEBUGGING MESSAGES IN LINUX
    //log messages coming through
    {
        tANI_U32 nMsgs = sizeof(pttMsgDbgStrings) / sizeof(sPttMsgIdStr);
        tANI_U32 i;

        for (i = 0; i < nMsgs; i++)
        {
            if (pttMsg->msgId == pttMsgDbgStrings[i].id)
            {
                phyLog(pMac, LOG1, "%s returns %d\n", &pttMsgDbgStrings[i].string[0], retVal);
                break;
            }
        }
    }
*/
    pttMsg->msgResponse = retVal;
    NTOHL(pttMsg->msgResponse);

    pttSendMsgResponse(pMac, pttMsg);


}

#endif  //ifndef WLAN_FTM_STUB

