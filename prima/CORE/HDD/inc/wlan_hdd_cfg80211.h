/*
* Copyright (c) 2012-2013 Qualcomm Atheros, Inc.
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*/

#if !defined( HDD_CFG80211_H__ )
#define HDD_CFG80211_H__


/**===========================================================================
  
  \file  wlan_hdd_cfg80211.h
  
  \brief cfg80211 functions declarations
    
               Copyright 2008 (c) Qualcomm Technologies, Inc.
               All Rights Reserved.
               Qualcomm Technologies Confidential and Proprietary.
  
  ==========================================================================*/
  
/* $HEADER$ */


//value for initial part of frames and number of bytes to be compared
#define GAS_INITIAL_REQ "\x04\x0a"  
#define GAS_INITIAL_REQ_SIZE 2

#define GAS_INITIAL_RSP "\x04\x0b"
#define GAS_INITIAL_RSP_SIZE 2

#define GAS_COMEBACK_REQ "\x04\x0c"
#define GAS_COMEBACK_REQ_SIZE 2

#define GAS_COMEBACK_RSP "\x04\x0d"
#define GAS_COMEBACK_RSP_SIZE 2

#define P2P_PUBLIC_ACTION_FRAME "\x04\x09\x50\x6f\x9a\x09" 
#define P2P_PUBLIC_ACTION_FRAME_SIZE 6

#define P2P_ACTION_FRAME "\x7f\x50\x6f\x9a\x09"
#define P2P_ACTION_FRAME_SIZE 5

#define HDD_P2P_WILDCARD_SSID "DIRECT-" //TODO Put it in proper place;
#define HDD_P2P_WILDCARD_SSID_LEN 7

#define WPA_OUI_TYPE   "\x00\x50\xf2\x01"
#define BLACKLIST_OUI_TYPE   "\x00\x50\x00\x00"
#define WHITELIST_OUI_TYPE   "\x00\x50\x00\x01"
#define WPA_OUI_TYPE_SIZE  4

#define WLAN_BSS_MEMBERSHIP_SELECTOR_HT_PHY 127
#define BASIC_RATE_MASK   0x80
#define RATE_MASK         0x7f

#ifdef WLAN_ENABLE_AGEIE_ON_SCAN_RESULTS
/* GPS application requirement */
#define QCOM_VENDOR_IE_ID 221
#define QCOM_OUI1         0x00
#define QCOM_OUI2         0xA0
#define QCOM_OUI3         0xC6
#define QCOM_VENDOR_IE_AGE_TYPE  0x100
#define QCOM_VENDOR_IE_AGE_LEN   4

#ifdef FEATURE_WLAN_TDLS
#define WLAN_IS_TDLS_SETUP_ACTION(action) \
         ((SIR_MAC_TDLS_SETUP_REQ <= action) && (SIR_MAC_TDLS_SETUP_CNF >= action))
#endif

typedef struct {
   u8 element_id;
   u8 len;
   u8 oui_1;
   u8 oui_2;
   u8 oui_3;
   u32 type;
   u32 age;
}__attribute__((packed)) qcom_ie_age ;
#endif

struct cfg80211_bss* wlan_hdd_cfg80211_update_bss_db( hdd_adapter_t *pAdapter,
                                      tCsrRoamInfo *pRoamInfo
                                      );

#ifdef FEATURE_WLAN_LFR
int wlan_hdd_cfg80211_pmksa_candidate_notify(
                    hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo,
                    int index, bool preauth );
#endif

#ifdef FEATURE_WLAN_WAPI
void wlan_hdd_cfg80211_set_key_wapi(hdd_adapter_t* pAdapter,
              u8 key_index, const u8 *mac_addr, u8 *key , int key_Len);
#endif
struct wiphy *wlan_hdd_cfg80211_init(int priv_size);

int wlan_hdd_cfg80211_scan( struct wiphy *wiphy,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0))
                            struct net_device *dev,
#endif
                            struct cfg80211_scan_request *request);

int wlan_hdd_cfg80211_register(struct device *dev,
                               struct wiphy *wiphy,
                               hdd_config_t *pCfg
                                         );

void wlan_hdd_cfg80211_post_voss_start(hdd_adapter_t* pAdapter);

void wlan_hdd_cfg80211_pre_voss_stop(hdd_adapter_t* pAdapter);

int wlan_hdd_crda_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request);
int wlan_hdd_get_crda_regd_entry(struct wiphy *wiphy, hdd_config_t *pCfg);
extern v_VOID_t hdd_connSetConnectionState( hdd_station_ctx_t *pHddStaCtx,
                                        eConnectionState connState );
#ifdef FEATURE_WLAN_TDLS
int wlan_hdd_cfg80211_send_tdls_discover_req(struct wiphy *wiphy,
                            struct net_device *dev, u8 *peer);
#endif


#endif
