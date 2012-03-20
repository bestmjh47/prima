/*
* Copyright (c) 2011-2012 Qualcomm Atheros Inc. All Rights Reserved.
* Qualcomm Atheros Proprietary and Confidential.
*/

#ifndef _NL80211_DRV_H_
#define _NL80211_DRV_H_

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/socket.h>

#include <linux/nl80211.h>
#include "libtcmd.h"

int nl80211_init(struct tcmd_cfg *cfg);
int nl80211_tcmd_tx(struct tcmd_cfg *cfg, void *buf, int len);
int nl80211_tcmd_rx(struct tcmd_cfg *cfg);
#endif /* _NL80211_DRV_H_ */
