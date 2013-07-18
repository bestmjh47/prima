/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _OL_FW_H_
#define _OL_FW_H_

#ifdef QCA_WIFI_FTM
#include "vos_types.h"
#endif

#define AR9888_REV1_VERSION          0x4000002c
#define AR9888_REV2_VERSION          0x4100016c
#define QCA_VERSION                  0x4100270f
#define AR6320_REV1_VERSION	     0x5000000
#define QCA_FIRMWARE_FILE            "athwlan.bin"
#define QCA_UTF_FIRMWARE_FILE        "utf.bin"
#define QCA_BOARD_DATA_FILE      "fakeBoardData_AR6004.bin"
#define QCA_OTP_FILE                 "otp.bin"

/* Configuration for statistics pushed by firmware */
#define PDEV_DEFAULT_STATS_UPDATE_PERIOD    500
#define VDEV_DEFAULT_STATS_UPDATE_PERIOD    500
#define PEER_DEFAULT_STATS_UPDATE_PERIOD    500

int ol_download_firmware(struct ol_softc *scn);
int ol_configure_target(struct ol_softc *scn);
void ol_target_failure(void *instance, A_STATUS status);
#ifdef QCA_WIFI_FTM
extern tVOS_CON_MODE hdd_get_conparam(void);
#endif
#endif /* _OL_FW_H_ */
