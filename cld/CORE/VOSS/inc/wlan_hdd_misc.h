/*
* Copyright (c) 2012 Qualcomm Atheros, Inc.
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*/


#ifndef WLAN_HDD_MISC_H
#define WLAN_HDD_MISC_H

#ifdef MSM_PLATFORM
#ifdef QC_WLAN_CHIPSET_QCA_CLD
#define WLAN_INI_FILE              "wlan/qca_cld/WCNSS_qcom_cfg.ini"
#define WLAN_CFG_FILE              "wlan/qca_cld/WCNSS_cfg.dat"
#define WLAN_FW_FILE               ""
#define WLAN_NV_FILE               "wlan/qca_cld/WCNSS_qcom_wlan_nv.bin"
#define WLAN_COUNTRY_INFO_FILE     "wlan/qca_cld/WCNSS_wlan_country_info.dat"
#define WLAN_HO_CFG_FILE           "wlan/qca_cld/WCNSS_wlan_ho_config"
#else
#define WLAN_INI_FILE              "wlan/volans/WCN1314_qcom_cfg.ini"
#define WLAN_CFG_FILE              "wlan/volans/WCN1314_cfg.dat"
#define WLAN_FW_FILE               "wlan/volans/WCN1314_qcom_fw.bin"
#define WLAN_NV_FILE               "wlan/volans/WCN1314_qcom_wlan_nv.bin"
#define WLAN_COUNTRY_INFO_FILE     "wlan/volans/WCN1314_wlan_country_info.dat"
#define WLAN_HO_CFG_FILE           "wlan/volans/WCN1314_wlan_ho_config"
#endif // ANI_CHIPSET
#else
#define WLAN_INI_FILE              "wlan/qcom_cfg.ini"
#define WLAN_CFG_FILE              "wlan/cfg.dat"
#define WLAN_FW_FILE               "wlan/qcom_fw.bin"
#define WLAN_NV_FILE               "wlan/qcom_wlan_nv.bin"
#define WLAN_COUNTRY_INFO_FILE     "wlan/wlan_country_info.dat"
#define WLAN_HO_CFG_FILE           "wlan/wlan_ho_config"
#endif // MSM_PLATFORM


VOS_STATUS hdd_request_firmware(char *pfileName,v_VOID_t *pCtx,v_VOID_t **ppfw_data, v_SIZE_t *pSize);

VOS_STATUS hdd_release_firmware(char *pFileName,v_VOID_t *pCtx);

VOS_STATUS hdd_get_cfg_file_size(v_VOID_t *pCtx, char *pFileName, v_SIZE_t *pBufSize);

VOS_STATUS hdd_read_cfg_file(v_VOID_t *pCtx, char *pFileName, v_VOID_t *pBuffer, v_SIZE_t *pBufSize);
#if 0

VOS_STATUS hdd_release_firmware(char *pFileName,v_VOID_t *pCtx);

VOS_STATUS hdd_get_cfg_file_size(v_VOID_t *pCtx, char *pFileName, v_SIZE_t *pBufSize);

VOS_STATUS hdd_read_cfg_file(v_VOID_t *pCtx, char *pFileName, v_VOID_t *pBuffer, v_SIZE_t *pBufSize);

#endif

tVOS_CONCURRENCY_MODE hdd_get_concurrency_mode ( void );

#endif /* WLAN_HDD_MISC_H */

