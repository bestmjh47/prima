
#ifndef WLAN_HDD_MISC_H
#define WLAN_HDD_MISC_H

#ifdef MSM_PLATFORM
#define INI_FILE                    "wlan/volans/WCN1314_qcom_cfg.ini"
#define LIBRA_CFG_FILE              "wlan/volans/WCN1314_cfg.dat"
#define LIBRA_FW_FILE               "wlan/volans/WCN1314_qcom_fw.bin"
#define LIBRA_NV_FILE               "wlan/volans/WCN1314_qcom_wlan_nv.bin"
#define LIBRA_COUNTRY_INFO_FILE     "wlan/volans/WCN1314_wlan_country_info.dat"
#define LIBRA_HO_CFG_FILE           "wlan/volans/WCN1314_wlan_ho_config"
#else
#define INI_FILE                    "wlan/qcom_cfg.ini"
#define LIBRA_CFG_FILE              "wlan/cfg.dat"
#define LIBRA_FW_FILE               "wlan/qcom_fw.bin"
#define LIBRA_NV_FILE               "wlan/qcom_wlan_nv.bin"
#define LIBRA_COUNTRY_INFO_FILE     "wlan/wlan_country_info.dat"
#define LIBRA_HO_CFG_FILE           "wlan/wlan_ho_config"
#endif


VOS_STATUS hdd_request_firmware(char *pfileName,v_VOID_t *pCtx,v_VOID_t **ppfw_data, v_SIZE_t *pSize);

VOS_STATUS hdd_release_firmware(char *pFileName,v_VOID_t *pCtx);

VOS_STATUS hdd_get_cfg_file_size(v_VOID_t *pCtx, char *pFileName, v_SIZE_t *pBufSize);

VOS_STATUS hdd_read_cfg_file(v_VOID_t *pCtx, char *pFileName, v_VOID_t *pBuffer, v_SIZE_t *pBufSize);
#if 0

VOS_STATUS hdd_release_firmware(char *pFileName,v_VOID_t *pCtx);

VOS_STATUS hdd_get_cfg_file_size(v_VOID_t *pCtx, char *pFileName, v_SIZE_t *pBufSize);

VOS_STATUS hdd_read_cfg_file(v_VOID_t *pCtx, char *pFileName, v_VOID_t *pBuffer, v_SIZE_t *pBufSize);

#endif

#ifdef WLAN_SOFTAP_FEATURE

VOS_CON_MODE hdd_get_conparam ( void );
#endif

#endif /* WLAN_HDD_MISC_H */

