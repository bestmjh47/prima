WLAN_BLD_DIR := $(call my-dir)

#Build/Package Libra Mono only in case of 7627 target
ifneq (, $(filter msm7627_surf msm7627_ffa, $(QCOM_TARGET_PRODUCT)))
        include $(WLAN_BLD_DIR)/libra/CORE/HDD/src/Android.mk
        include $(WLAN_BLD_DIR)/utils/ptt/Android.mk
        include $(WLAN_BLD_DIR)/utils/asf/src/Android.mk
endif

#Build/Package Libra and Volans Module in case of msm7x30_surf target
ifneq (, $(filter msm7630_surf, $(QCOM_TARGET_PRODUCT)))
        include $(WLAN_BLD_DIR)/volans/CORE/HDD/src/Android.mk
        include $(WLAN_BLD_DIR)/libra/CORE/HDD/src/Android.mk
        include $(WLAN_BLD_DIR)/utils/ptt/Android.mk
        include $(WLAN_BLD_DIR)/utils/asf/src/Android.mk
endif

#Build/Package Volans Module only in case of 8660 target variants
ifeq "$(findstring msm8660,$(QCOM_TARGET_PRODUCT))" "msm8660"
        include $(WLAN_BLD_DIR)/volans/CORE/HDD/src/Android.mk
        include $(WLAN_BLD_DIR)/utils/ptt/Android.mk
        include $(WLAN_BLD_DIR)/utils/asf/src/Android.mk
endif

#Build/Package Prima Module only in case of 8960 target variants
ifeq "$(findstring msm8960,$(QCOM_TARGET_PRODUCT))" "msm8960"
        include $(WLAN_BLD_DIR)/prima/CORE/HDD/src/Android.mk
endif

#Build/Package Volans Module only in case of 7627a target
ifneq (, $(filter msm7627a, $(QCOM_TARGET_PRODUCT)))
        include $(WLAN_BLD_DIR)/volans/CORE/HDD/src/Android.mk
        include $(WLAN_BLD_DIR)/utils/ptt/Android.mk
        include $(WLAN_BLD_DIR)/utils/asf/src/Android.mk
endif
