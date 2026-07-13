/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gicv3.h"

static void gic_redist_wake(void)
{
    uint32_t waker;
    unsigned int retries = 1000000U;

    waker = gic_read32(GICR_WAKER);
    waker &= ~(1U << 1);
    waker &= ~(1U << 2);
    gic_write32(GICR_WAKER, waker);

    while ((gic_read32(GICR_WAKER) & (1U << 2)) && retries-- > 0U) {
    }

    gic_write32(GICR_CTLR, 0x1U);
    while (gic_read32(GICR_CTLR) & (1U << 3)) {
    }
}

void gic_enable_spi(uint32_t intid)
{
    uint32_t spi = intid - 32U;
    uint32_t enable_reg = spi / 32U;
    uint32_t enable_bit = spi % 32U;
    uint32_t group_reg = intid / 32U;
    uint32_t group_bit = intid % 32U;
    uint32_t group;

    gic_write8(GICD_IPRIORITYR + intid, 0xA0U);
    gic_route_spi_to_cpu0(spi);

    group = gic_read32(GICD_IGROUPR + (group_reg * 4U));
    group |= (1U << group_bit);
    gic_write32(GICD_IGROUPR + (group_reg * 4U), group);

    gic_write32(GICD_ISENABLER + (enable_reg * 4U), 1U << enable_bit);
}

void gic_init(void)
{
    gic_redist_wake();
    gic_write32(GICD_CTLR, 0U);

    icc_sre_write(icc_sre_read() | 0x7U);
    icc_pmr_write(0xFFU);
    icc_igrpen1_write(0U);
}

void gic_enable_cpu_interface(void)
{
    gic_write32(GICD_CTLR, GICD_CTLR_ENABLE_G1A | GICD_CTLR_ARE_NS | GICD_CTLR_ARE);
    icc_igrpen1_write(1U);
}

uint32_t gic_acknowledge_irq(void)
{
    return icc_iar1_read();
}

void gic_end_of_interrupt(uint32_t intid)
{
    icc_eoir1_write(intid);
}
