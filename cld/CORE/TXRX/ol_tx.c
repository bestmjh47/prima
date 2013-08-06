/*
 * Copyright (c) 2011-2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/* OS abstraction libraries */
#include <adf_nbuf.h>         /* adf_nbuf_t, etc. */
#include <adf_os_atomic.h>    /* adf_os_atomic_read, etc. */
#include <adf_os_util.h>      /* adf_os_unlikely */

/* APIs for other modules */
#include <htt.h>              /* HTT_TX_EXT_TID_MGMT */
#include <ol_htt_tx_api.h>    /* htt_tx_desc_tid */
#include <ol_txrx_api.h>      /* ol_txrx_vdev_handle */
#include <ol_txrx_ctrl_api.h> /* ol_txrx_sync */

/* internal header files relevant for all systems */
#include <ol_txrx_internal.h> /* TXRX_ASSERT1 */
#include <ol_txrx_types.h>    /* pdev stats */
#include <ol_tx_desc.h>       /* ol_tx_desc */
#include <ol_tx_send.h>       /* ol_tx_send */
#include <ol_txrx.h>

/* internal header files relevant only for HL systems */
#include <ol_tx_classify.h>   /* ol_tx_classify, ol_tx_classify_mgmt */
#include <ol_tx_queue.h>      /* ol_tx_enqueue */
#include <ol_tx_sched.h>      /* ol_tx_sched */

/* internal header files relevant only for specific systems (Pronto) */
#include <ol_txrx_encap.h>    /* OL_TX_ENCAP, etc */

#define ENABLE_TX_SCHED 1

#define ol_tx_prepare_ll(tx_desc, vdev, msdu, msdu_info) \
    do {                                                                      \
        struct ol_txrx_pdev_t *pdev = vdev->pdev;                             \
        /* 
         * The TXRX module doesn't accept tx frames unless the target has 
         * enough descriptors for them.
         * For LL, the TXRX descriptor pool is sized to match the target's
         * descriptor pool.  Hence, if the descriptor allocation in TXRX
         * succeeds, that guarantees that the target has room to accept
         * the new tx frame.
         */                                                                   \
        (msdu_info)->htt.info.frame_type = pdev->htt_pkt_type;                \
        tx_desc = ol_tx_desc_ll(pdev, vdev, msdu, msdu_info);                 \
        if (adf_os_unlikely(! tx_desc)) {                                     \
            TXRX_STATS_MSDU_LIST_INCR(                                        \
                pdev, tx.dropped.host_reject, msdu);                          \
            return msdu; /* the list of unaccepted MSDUs */                   \
        }                                                                     \
    } while (0)

adf_nbuf_t
ol_tx_ll(ol_txrx_vdev_handle vdev, adf_nbuf_t msdu_list)
{
    adf_nbuf_t msdu = msdu_list;
    struct ol_txrx_msdu_info_t msdu_info;

    msdu_info.htt.info.l2_hdr_type = vdev->pdev->htt_pkt_type;
    /*
     * The msdu_list variable could be used instead of the msdu var, 
     * but just to clarify which operations are done on a single MSDU
     * vs. a list of MSDUs, use a distinct variable for single MSDUs
     * within the list.
     */
    while (msdu) {
        adf_nbuf_t next;
        struct ol_tx_desc_t *tx_desc;

        msdu_info.htt.info.ext_tid = adf_nbuf_get_tid(msdu);
        msdu_info.peer = NULL;
        ol_tx_prepare_ll(tx_desc, vdev, msdu, &msdu_info);

        /*
         * If debug display is enabled, show the meta-data being
         * downloaded to the target via the HTT tx descriptor.
         */
        htt_tx_desc_display(tx_desc->htt_tx_desc);
        /*
         * The netbuf may get linked into a different list inside the
         * ol_tx_send function, so store the next pointer before the
         * tx_send call.
         */
        next = adf_nbuf_next(msdu);
        ol_tx_send(vdev->pdev, tx_desc, msdu);
        msdu = next;
    }
    return NULL; /* all MSDUs were accepted */
}

static inline int
OL_TXRX_TX_IS_RAW(enum ol_txrx_osif_tx_spec tx_spec)
{
    return
        tx_spec &
        (ol_txrx_osif_tx_spec_raw |
         ol_txrx_osif_tx_spec_no_aggr |
         ol_txrx_osif_tx_spec_no_encrypt);
}

static inline u_int8_t
OL_TXRX_TX_RAW_SUBTYPE(enum ol_txrx_osif_tx_spec tx_spec)
{
    u_int8_t sub_type = 0x1; /* 802.11 MAC header present */

    if (tx_spec & ol_txrx_osif_tx_spec_no_aggr) {
        sub_type |= 0x1 << HTT_TX_MSDU_DESC_RAW_SUBTYPE_NO_AGGR_S;
    }
    if (tx_spec & ol_txrx_osif_tx_spec_no_encrypt) {
        sub_type |= 0x1 << HTT_TX_MSDU_DESC_RAW_SUBTYPE_NO_ENCRYPT_S;
    }
    if (tx_spec & ol_txrx_osif_tx_spect_nwifi_no_encrypt) {
        sub_type |= 0x1 << HTT_TX_MSDU_DESC_RAW_SUBTYPE_NO_ENCRYPT_S;
    }
    return sub_type;
}

adf_nbuf_t
ol_tx_non_std_ll(
    ol_txrx_vdev_handle vdev,
    enum ol_txrx_osif_tx_spec tx_spec,
    adf_nbuf_t msdu_list)
{
    adf_nbuf_t msdu = msdu_list;
    htt_pdev_handle htt_pdev = vdev->pdev->htt_pdev;
    struct ol_txrx_msdu_info_t msdu_info;

    msdu_info.htt.info.l2_hdr_type = vdev->pdev->htt_pkt_type;

    /*
     * The msdu_list variable could be used instead of the msdu var, 
     * but just to clarify which operations are done on a single MSDU
     * vs. a list of MSDUs, use a distinct variable for single MSDUs
     * within the list.
     */
    while (msdu) {
        adf_nbuf_t next;
        struct ol_tx_desc_t *tx_desc;

        msdu_info.htt.info.ext_tid = adf_nbuf_get_tid(msdu);
        msdu_info.peer = NULL;

        ol_tx_prepare_ll(tx_desc, vdev, msdu, &msdu_info);

        /*
         * The netbuf may get linked into a different list inside the
         * ol_tx_send function, so store the next pointer before the
         * tx_send call.
         */
        next = adf_nbuf_next(msdu);

        if (tx_spec != ol_txrx_osif_tx_spec_std) {
            if (tx_spec & ol_txrx_osif_tx_spec_tso) {
                tx_desc->pkt_type = ol_tx_frm_tso;
            } else if (tx_spec & ol_txrx_osif_tx_spect_nwifi_no_encrypt) {
                u_int8_t sub_type = OL_TXRX_TX_RAW_SUBTYPE(tx_spec);
                htt_tx_desc_type(
                    htt_pdev, tx_desc->htt_tx_desc,
                    htt_pkt_type_native_wifi, sub_type);
            } else if (OL_TXRX_TX_IS_RAW(tx_spec)) {
                /* different types of raw frames */
                u_int8_t sub_type = OL_TXRX_TX_RAW_SUBTYPE(tx_spec);
                htt_tx_desc_type(
                    htt_pdev, tx_desc->htt_tx_desc,
                    htt_pkt_type_raw, sub_type);
            }
        }
        /*
         * If debug display is enabled, show the meta-data being
         * downloaded to the target via the HTT tx descriptor.
         */
        htt_tx_desc_display(tx_desc->htt_tx_desc);
        ol_tx_send(vdev->pdev, tx_desc, msdu);
        msdu = next;
    }
    return NULL; /* all MSDUs were accepted */
}

#ifdef QCA_SUPPORT_SW_TXRX_ENCAP
#define OL_TX_ENCAP_WRAPPER(pdev, vdev, tx_desc, msdu, tx_msdu_info) \
    do { \
        if (OL_TX_ENCAP(vdev, tx_desc, msdu, &tx_msdu_info) != A_OK) { \
            adf_os_atomic_inc(&pdev->tx_queue.rsrc_cnt); \
            ol_tx_desc_frame_free_nonstd(pdev, tx_desc, 1); \
            if (tx_msdu_info.peer) { \
                /* remove the peer reference added above */ \
                ol_txrx_peer_unref_delete(tx_msdu_info.peer); \
            } \
            goto MSDU_LOOP_BOTTOM; \
        } \
    } while (0)
#else
#define OL_TX_ENCAP_WRAPPER(pdev, vdev, tx_desc, msdu, tx_msdu_info) /* no-op */
#endif

#ifdef QCA_WIFI_ISOC
#define TX_FILTER_CHECK(tx_msdu_info) \
    ((tx_msdu_info)->peer && \
     ((tx_msdu_info)->peer->tx_filter(tx_msdu_info) != A_OK))
#else
/* tx filtering is handled within the target FW */
#define TX_FILTER_CHECK(tx_msdu_info) 0 /* don't filter */
#endif

#if ENABLE_TX_SCHED
static inline adf_nbuf_t
ol_tx_hl_base(
    ol_txrx_vdev_handle vdev,
    enum ol_txrx_osif_tx_spec tx_spec,
    adf_nbuf_t msdu_list)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    adf_nbuf_t msdu = msdu_list;
    struct ol_txrx_msdu_info_t tx_msdu_info;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;
    tx_msdu_info.peer = NULL;

    /*
     * The msdu_list variable could be used instead of the msdu var,
     * but just to clarify which operations are done on a single MSDU
     * vs. a list of MSDUs, use a distinct variable for single MSDUs
     * within the list.
     */
    while (msdu) {
        adf_nbuf_t next;
        struct ol_tx_frms_queue_t *txq;
        struct ol_tx_desc_t *tx_desc;

        /*
         * The netbuf will get stored into a (peer-TID) tx queue list
         * inside the ol_tx_classify_store function or else dropped,
         * so store the next pointer immediately.
         */
        next = adf_nbuf_next(msdu);

        tx_desc = ol_tx_desc_hl(pdev, vdev, msdu, &tx_msdu_info);
        if (! tx_desc) {
            /*
             * If we're out of tx descs, there's no need to try to allocate
             * tx descs for the remaining MSDUs.
             */
            TXRX_STATS_MSDU_LIST_INCR(pdev, tx.dropped.host_reject, msdu);
            return msdu; /* the list of unaccepted MSDUs */
        }

//        OL_TXRX_PROT_AN_LOG(pdev->prot_an_tx_sent, msdu);

        if (tx_spec != ol_txrx_osif_tx_spec_std) {
            if (tx_spec & ol_txrx_osif_tx_spec_tso) {
                tx_desc->pkt_type = ol_tx_frm_tso;
            }
            if (OL_TXRX_TX_IS_RAW(tx_spec)) {
                // CHECK THIS: does this need to happen after htt_tx_desc_init?
                /* different types of raw frames */
                u_int8_t sub_type = OL_TXRX_TX_RAW_SUBTYPE(tx_spec);
                htt_tx_desc_type(
                    htt_pdev, tx_desc->htt_tx_desc,
                    htt_pkt_type_raw, sub_type);
            }
        }

        tx_msdu_info.htt.info.ext_tid = adf_nbuf_get_tid(msdu);
        tx_msdu_info.htt.info.vdev_id = vdev->vdev_id;
        tx_msdu_info.htt.info.frame_type = htt_frm_type_data;
        tx_msdu_info.htt.info.l2_hdr_type = pdev->htt_pkt_type;

        txq = ol_tx_classify(vdev, tx_desc, msdu, &tx_msdu_info);

        if ((!txq) || TX_FILTER_CHECK(&tx_msdu_info)) {
            /* drop this frame, but try sending subsequent frames */
            //TXRX_STATS_MSDU_LIST_INCR(pdev, tx.dropped.no_txq, msdu);
            adf_os_atomic_inc(&pdev->tx_queue.rsrc_cnt);
            ol_tx_desc_frame_free_nonstd(pdev, tx_desc, 1);
            if (tx_msdu_info.peer) {
                /* remove the peer reference added above */
                ol_txrx_peer_unref_delete(tx_msdu_info.peer);
            }
            goto MSDU_LOOP_BOTTOM;
        }

        if(tx_msdu_info.peer) {
			/*If the state is not associated then drop all the data packets recieved for that peer*/
		    if(tx_msdu_info.peer->state == ol_txrx_peer_state_disc) {
                 adf_os_atomic_inc(&pdev->tx_queue.rsrc_cnt);
                 ol_tx_desc_frame_free_nonstd(pdev, tx_desc, 1);
                 ol_txrx_peer_unref_delete(tx_msdu_info.peer);
                 msdu = next;
                 continue;
		    }
            else if (tx_msdu_info.peer->state != ol_txrx_peer_state_auth) {

                if (tx_msdu_info.htt.info.ethertype != ETHERTYPE_PAE && tx_msdu_info.htt.info.ethertype != ETHERTYPE_WAI) {
                    adf_os_atomic_inc(&pdev->tx_queue.rsrc_cnt);
                    ol_tx_desc_frame_free_nonstd(pdev, tx_desc, 1);
                    ol_txrx_peer_unref_delete(tx_msdu_info.peer);
                    msdu = next;
                    continue;
                 }
            }
        }
        /*
         * Initialize the HTT tx desc l2 header offset field.
         * htt_tx_desc_mpdu_header  needs to be called to make sure,
         * the l2 header size is initialized correctly to handle cases
         * where TX ENCAP is disabled or Tx Encap fails to perform Encap
         */
        htt_tx_desc_mpdu_header(tx_desc->htt_tx_desc, 0);

        /*
         * Note: when the driver is built without support for SW tx encap,
         * the following macro is a no-op.   When the driver is built with
         * support for SW tx encap, it performs encap, and if an error is
         * encountered, jumps to the MSDU_LOOP_BOTTOM label.
         */
        OL_TX_ENCAP_WRAPPER(pdev, vdev, tx_desc, msdu, tx_msdu_info);

        /* initialize the HW tx descriptor */
        htt_tx_desc_init(
            pdev->htt_pdev, tx_desc->htt_tx_desc,
	    tx_desc->htt_tx_desc_paddr,
            ol_tx_desc_id(pdev, tx_desc),
            msdu,
            &tx_msdu_info.htt);
        /*
         * If debug display is enabled, show the meta-data being
         * downloaded to the target via the HTT tx descriptor.
         */
        htt_tx_desc_display(tx_desc->htt_tx_desc);

        ol_tx_enqueue(pdev, txq, tx_desc, &tx_msdu_info);
        if (tx_msdu_info.peer) {
            OL_TX_PEER_STATS_UPDATE(tx_msdu_info.peer, msdu);
            /* remove the peer reference added above */
            ol_txrx_peer_unref_delete(tx_msdu_info.peer);
        }
MSDU_LOOP_BOTTOM:
        msdu = next;
    }
    ol_tx_sched(pdev);

    return NULL; /* all MSDUs were accepted */
}

#else /* ENABLE_TX_SCHED == 0 */

static inline adf_nbuf_t
ol_tx_hl_base(
    ol_txrx_vdev_handle vdev,
    enum ol_txrx_osif_tx_spec tx_spec,
    adf_nbuf_t msdu_list)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    adf_nbuf_t msdu = msdu_list;
    struct ol_txrx_msdu_info_t tx_msdu_info;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;
    tx_msdu_info.peer = NULL;

    /*
     * The msdu_list variable could be used instead of the msdu var,
     * but just to clarify which operations are done on a single MSDU
     * vs. a list of MSDUs, use a distinct variable for single MSDUs
     * within the list.
     */
    while (msdu) {
        adf_nbuf_t next;
        struct ol_tx_frms_queue_t *txq;
        struct ol_tx_desc_t *tx_desc;
        if (adf_os_atomic_read(&vdev->pdev->target_tx_credit) <= 0) {
            return msdu;
        }
        next = adf_nbuf_next(msdu);

        tx_desc = ol_tx_desc_hl(pdev, vdev, msdu, &tx_msdu_info);
        if (! tx_desc) {
            /*
             * If we're out of tx descs, there's no need to try to allocate
             * tx descs for the remaining MSDUs.
             */
            TXRX_STATS_MSDU_LIST_INCR(pdev, tx.dropped.host_reject, msdu);
            return msdu; /* the list of unaccepted MSDUs */
        }
        OL_TXRX_PROT_AN_LOG(pdev->prot_an_tx_sent, msdu);

        if (tx_spec != ol_txrx_osif_tx_spec_std) {
            if (tx_spec & ol_txrx_osif_tx_spec_tso) {
                tx_desc->pkt_type = ol_tx_frm_tso;
            }
            if (OL_TXRX_TX_IS_RAW(tx_spec)) {
                // CHECK THIS: does this need to happen after htt_tx_desc_init?
                /* different types of raw frames */
                u_int8_t sub_type = OL_TXRX_TX_RAW_SUBTYPE(tx_spec);
                htt_tx_desc_type(
                    htt_pdev, tx_desc->htt_tx_desc,
                    htt_pkt_type_raw, sub_type);
            }
        }

        tx_msdu_info.htt.info.ext_tid = adf_nbuf_get_tid(msdu);
        tx_msdu_info.htt.info.vdev_id = vdev->vdev_id;
        tx_msdu_info.htt.info.frame_type = htt_frm_type_data;
        tx_msdu_info.htt.info.l2_hdr_type = pdev->htt_pkt_type;
        txq = ol_tx_classify(vdev, tx_desc, msdu, &tx_msdu_info);
        if (!txq) {
            adf_os_atomic_inc(&pdev->tx_queue.rsrc_cnt);
            //TXRX_STATS_MSDU_LIST_INCR(vdev->pdev, tx.dropped.no_txq, msdu);
            ol_tx_desc_free(vdev->pdev, tx_desc);
            if (tx_msdu_info.peer) {
                /* remove the peer reference added above */
                ol_txrx_peer_unref_delete(tx_msdu_info.peer);
            }
            return msdu; /* the list of unaccepted MSDUs */
        }

        /* Before authentication, we'll drop packets except eapol/wai frame only */
        if (tx_msdu_info.peer && tx_msdu_info.peer->state != ol_txrx_peer_state_auth)
        {
            if (tx_msdu_info.htt.info.ethertype != ETHERTYPE_PAE && tx_msdu_info.htt.info.ethertype != ETHERTYPE_WAI)
            {
                adf_os_atomic_inc(&pdev->tx_queue.rsrc_cnt);
                ol_tx_desc_free(vdev->pdev, tx_desc);
                /* remove the peer reference added above */
                ol_txrx_peer_unref_delete(tx_msdu_info.peer);
                return msdu; /* the list of unaccepted MSDUs */
            }
        }

        /* initialize the HW tx descriptor */
        htt_tx_desc_init(
            pdev->htt_pdev, tx_desc->htt_tx_desc,
            ol_tx_desc_id(pdev, tx_desc),
            msdu,
            &tx_msdu_info.htt);
        /*
         * If debug display is enabled, show the meta-data being
         * downloaded to the target via the HTT tx descriptor.
         */
        htt_tx_desc_display(tx_desc->htt_tx_desc);

        ol_tx_send(pdev, tx_desc, msdu);
MSDU_LOOP_BOTTOM:
        msdu = next;
    }
    return NULL; /* all MSDUs were accepted */
}

#endif

adf_nbuf_t
ol_tx_hl(ol_txrx_vdev_handle vdev, adf_nbuf_t msdu_list)
{
    return ol_tx_hl_base(vdev, ol_txrx_osif_tx_spec_std, msdu_list);
}

adf_nbuf_t
ol_tx_non_std_hl(
    ol_txrx_vdev_handle vdev,
    enum ol_txrx_osif_tx_spec tx_spec,
    adf_nbuf_t msdu_list)
{
    return ol_tx_hl_base(vdev, tx_spec, msdu_list);
}

void
ol_txrx_mgmt_tx_cb_set(
    ol_txrx_pdev_handle pdev,
    u_int8_t type,
    ol_txrx_mgmt_tx_cb download_cb,
    ol_txrx_mgmt_tx_cb ota_ack_cb,
    void *ctxt)
{
    TXRX_ASSERT1(type < OL_TXRX_MGMT_NUM_TYPES);
    pdev->tx_mgmt.callbacks[type].download_cb = download_cb;
    pdev->tx_mgmt.callbacks[type].ota_ack_cb = ota_ack_cb;
    pdev->tx_mgmt.callbacks[type].ctxt = ctxt;
}

int
ol_txrx_mgmt_send(
    ol_txrx_vdev_handle vdev,
    adf_nbuf_t tx_mgmt_frm,
    u_int8_t type,
    u_int8_t use_6mbps)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    struct ol_tx_desc_t *tx_desc;
    struct ol_txrx_msdu_info_t tx_msdu_info;

    tx_msdu_info.htt.action.use_6mbps = use_6mbps;
    tx_msdu_info.htt.info.ext_tid = HTT_TX_EXT_TID_MGMT;
    tx_msdu_info.htt.info.vdev_id = vdev->vdev_id;
    tx_msdu_info.htt.action.do_tx_complete =
        pdev->tx_mgmt.callbacks[type].ota_ack_cb ? 1 : 0;

    /*
     * FIX THIS: l2_hdr_type should only specify L2 header type
     * The Peregrine/Rome HTT layer provides the FW with a "pkt type"
     * that is a combination of L2 header type and 802.11 frame type.
     * If the 802.11 frame type is "mgmt", then the HTT pkt type is "mgmt".
     * But if the 802.11 frame type is "data", then the HTT pkt type is
     * the L2 header type (more or less): 802.3 vs. Native WiFi (basic 802.11).
     * (Or the header type can be "raw", which is any version of the 802.11
     * header, and also implies that some of the offloaded tx data processing
     * steps may not apply.)
     * For efficiency, the Peregrine/Rome HTT uses the msdu_info's l2_hdr_type
     * field to program the HTT pkt type.  Thus, this txrx SW needs to overload
     * the l2_hdr_type to indicate whether the frame is data vs. mgmt, as well
     * as 802.3 L2 header vs. 802.11 L2 header.
     * To fix this, the msdu_info's l2_hdr_type should be left specifying just
     * the L2 header type.  For mgmt frames, there should be a separate function
     * to patch the HTT pkt type to store a "mgmt" value rather than the
     * L2 header type.  Then the HTT pkt type can be programmed efficiently
     * for data frames, and the msdu_info's l2_hdr_type field won't be
     * confusingly overloaded to hold the 802.11 frame type rather than the
     * L2 header type.
     */
    /*
     * FIX THIS: remove duplication of htt_frm_type_mgmt and htt_pkt_type_mgmt
     * The htt module expects a "enum htt_pkt_type" value.
     * The htt_dxe module expects a "enum htt_frm_type" value.
     * This needs to be cleaned up, so both versions of htt use a
     * consistent method of specifying the frame type.
     */
#ifdef QCA_SUPPORT_INTEGRATED_SOC
    /* tx mgmt frames always come with a 802.11 header */
    tx_msdu_info.htt.info.l2_hdr_type = htt_pkt_type_native_wifi;
    tx_msdu_info.htt.info.frame_type = htt_frm_type_mgmt;
#else
    tx_msdu_info.htt.info.l2_hdr_type = htt_pkt_type_mgmt;
    tx_msdu_info.htt.info.frame_type = htt_pkt_type_mgmt;
#endif

    tx_msdu_info.peer = NULL;

    adf_nbuf_map_single(pdev->osdev, tx_mgmt_frm, ADF_OS_DMA_TO_DEVICE);
    if (pdev->cfg.is_high_latency) {
        tx_desc = ol_tx_desc_hl(pdev, vdev, tx_mgmt_frm, &tx_msdu_info);
    } else {
        tx_desc = ol_tx_desc_ll(pdev, vdev, tx_mgmt_frm, &tx_msdu_info);
        /* FIX THIS -
         * The FW currently has trouble using the host's fragments table
         * for management frames.  Until this is fixed, rather than
         * specifying the fragment table to the FW, specify just the
         * address of the initial fragment.
         */
        if (tx_desc) {
            /*
             * Following the call to ol_tx_desc_ll, frag 0 is the HTT
             * tx HW descriptor, and the frame payload is in frag 1.
             */
            htt_tx_desc_frags_table_set(
                pdev->htt_pdev, tx_desc->htt_tx_desc,
                adf_nbuf_get_frag_paddr_lo(tx_mgmt_frm, 1), 0);
        }
    }
    if (! tx_desc) {
        adf_nbuf_unmap_single(pdev->osdev, tx_mgmt_frm, ADF_OS_DMA_TO_DEVICE);
        return 1; /* can't accept the tx mgmt frame */
    }
    TXRX_STATS_MSDU_INCR(pdev, tx.mgmt, tx_mgmt_frm);
    TXRX_ASSERT1(type < OL_TXRX_MGMT_NUM_TYPES);
    tx_desc->pkt_type = type + OL_TXRX_MGMT_TYPE_BASE;
    
    if (pdev->cfg.is_high_latency) {
	struct ol_tx_frms_queue_t *txq;
        /*
         * 1.  Look up the peer and queue the frame in the peer's mgmt queue.
         * 2.  Invoke the download scheduler.
         */
        txq = ol_tx_classify_mgmt(vdev, tx_desc, tx_mgmt_frm, &tx_msdu_info);
        if (!txq) {
            //TXRX_STATS_MSDU_LIST_INCR(vdev->pdev, tx.dropped.no_txq, msdu);
            adf_os_atomic_inc(&pdev->tx_queue.rsrc_cnt);
            ol_tx_desc_frame_free_nonstd(vdev->pdev, tx_desc, 1 /* error */);
            if (tx_msdu_info.peer) {
                /* remove the peer reference added above */
                ol_txrx_peer_unref_delete(tx_msdu_info.peer);
            }
            return 1; /* can't accept the tx mgmt frame */
        }
         /* Initialize the HTT tx desc l2 header offset field.
         * Even though tx encap does not apply to mgmt frames,
         * htt_tx_desc_mpdu_header still needs to be called,
         * to specifiy that there was no L2 header added by tx encap,
         * so the frame's length does not need to be adjusted to account for
         * an added L2 header.
         */
        htt_tx_desc_mpdu_header(tx_desc->htt_tx_desc, 0);
        htt_tx_desc_init(
            pdev->htt_pdev, tx_desc->htt_tx_desc,
	    tx_desc->htt_tx_desc_paddr,
            ol_tx_desc_id(pdev, tx_desc),
            tx_mgmt_frm,
            &tx_msdu_info.htt);
        htt_tx_desc_display(tx_desc->htt_tx_desc);

	ol_tx_enqueue(vdev->pdev, txq, tx_desc, &tx_msdu_info);
	if (tx_msdu_info.peer) {
	     /* remove the peer reference added above */
	     ol_txrx_peer_unref_delete(tx_msdu_info.peer);
	}
        ol_tx_sched(vdev->pdev);
    } else {
        ol_tx_send_nonstd(pdev, tx_desc, tx_mgmt_frm, htt_pkt_type_mgmt);
    }

    return 0; /* accepted the tx mgmt frame */
}

void
ol_txrx_sync(ol_txrx_pdev_handle pdev, u_int8_t sync_cnt)
{
    htt_h2t_sync_msg(pdev->htt_pdev, sync_cnt);
}

adf_nbuf_t ol_tx_reinject(
    struct ol_txrx_vdev_t *vdev,
    adf_nbuf_t msdu, u_int16_t peer_id)
{
    struct ol_tx_desc_t *tx_desc;
    struct ol_txrx_msdu_info_t msdu_info;

    msdu_info.htt.info.l2_hdr_type = vdev->pdev->htt_pkt_type;
    msdu_info.htt.info.ext_tid = HTT_TX_EXT_TID_INVALID;
    msdu_info.peer = NULL;

    ol_tx_prepare_ll(tx_desc, vdev, msdu, &msdu_info);
    HTT_TX_DESC_POSTPONED_SET(*((u_int32_t *)(tx_desc->htt_tx_desc)), TRUE);

    htt_tx_desc_set_peer_id((u_int32_t *)(tx_desc->htt_tx_desc), peer_id);

    ol_tx_send(vdev->pdev, tx_desc, msdu);

    return NULL;
}
