//------------------------------------------------------------------------------
// Copyright (c) 2011 Atheros Communications Inc.
// All rights reserved.
//
// $ATH_LICENSE_HOSTSDK0_C$
//------------------------------------------------------------------------------

#ifndef __ATH_PCI_H__
#define __ATH_PCI_H__

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/interrupt.h>

#define CONFIG_COPY_ENGINE_SUPPORT /* TBDXXX: here for now */
#define ATH_DBG_DEFAULT   0
#include <osdep.h>
#include <ol_if_athvar.h>
#include <athdefs.h>
#include "osapi_linux.h"
#include "hif.h"
#include "cepci.h"

/* Maximum number of Copy Engine's supported */
#define CE_COUNT_MAX 8
#define CE_HTT_H2T_MSG_SRC_NENTRIES 2048

struct CE_state;
struct ol_softc;

/* An address (e.g. of a buffer) in Copy Engine space. */
typedef ath_dma_addr_t CE_addr_t;

struct hif_pci_softc {
    void __iomem *mem; /* PCI address. */
                       /* For efficiency, should be first in struct */

    struct device *dev;
    struct pci_dev *pdev;
    struct _NIC_DEV aps_osdev;
    struct ol_softc *ol_sc;
    int num_msi_intrs; /* number of MSI interrupts granted */
			/* 0 --> using legacy PCI line interrupts */
    struct tasklet_struct intr_tq;    /* tasklet */

    int irq;
    int irq_event;
    int cacheline_sz;
    /*
    * Guard changes to Target HW state and to software
    * structures that track hardware state.
    */
    adf_os_spinlock_t target_lock;

    unsigned int ce_count; /* Number of Copy Engines supported */
    struct CE_state *CE_id_to_state[CE_COUNT_MAX]; /* Map CE id to CE_state */
    HIF_DEVICE *hif_device;

    bool force_break;  /* Flag to indicate whether to break out the DPC context */
    unsigned int receive_count; /* count Num Of Receive Buffers handled for one interrupt DPC routine */
    u16 devid;
    struct targetdef_s *targetdef;
    struct hostdef_s *hostdef;
};
#define TARGID(sc) ((A_target_id_t)(&(sc)->mem))
#define TARGID_TO_HIF(targid) (((struct hif_pci_softc *)((char *)(targid) - (char *)&(((struct hif_pci_softc *)0)->mem)))->hif_device)

int athdiag_procfs_init(struct hif_pci_softc *scn);
void athdiag_procfs_remove(void);

bool hif_pci_targ_is_awake(struct hif_pci_softc *sc, void *__iomem *mem);

bool hif_pci_targ_is_present(A_target_id_t targetid, void *__iomem *mem);

bool hif_max_num_receives_reached(unsigned int count);

int HIF_PCIDeviceProbed(hif_handle_t hif_hdl);
irqreturn_t HIF_fw_interrupt_handler(int irq, void *arg);

/* routine to modify the initial buffer count to be allocated on an os
 * platform basis. Platform owner will need to modify this as needed
 */
adf_os_size_t initBufferCount(adf_os_size_t maxSize);

/* Function to set the TXRX handle in the ol_sc context */
void hif_init_pdev_txrx_handle(void *ol_sc, void *txrx_handle);

#ifndef REMOVE_PKT_LOG
extern int pktlogmod_init(void *context);
extern void pktlogmod_exit(void *context);
#endif

/*
 * A firmware interrupt to the Host is indicated by the
 * low bit of SCRATCH_3_ADDRESS being set.
 */
#define FW_EVENT_PENDING_REG_ADDRESS SCRATCH_3_ADDRESS

/*
 * Typically, MSI Interrupts are used with PCIe. To force use of legacy
 * "ABCD" PCI line interrupts rather than MSI, define FORCE_LEGACY_PCI_INTERRUPTS.
 * Even when NOT forced, the driver may attempt to use legacy PCI interrupts 
 * MSI allocation fails
 */
#define LEGACY_INTERRUPTS(sc) ((sc)->num_msi_intrs == 0)
#endif /* __ATH_PCI_H__ */
