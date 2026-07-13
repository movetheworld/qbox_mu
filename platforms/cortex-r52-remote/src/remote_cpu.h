/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __REMOTE_CORTEX_R52__
#define __REMOTE_CORTEX_R52__

#include <systemc>
#include <tlm>
#include <cci_configuration>
#include <libgsutils.h>
#include <cpu_arm/cpu_arm_cortex_r52/include/cortex-r52.h>
#include <irq-ctrl/arm_gicv3/include/arm_gicv3.h>
#include <qemu-instance.h>
#include "router/include/router.h"
#include "remote.h"
#include "pass/include/pass.h"
#include <module_factory_registery.h>

class RemoteR52CPU : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    gs::router<> m_router;
    arm_gicv3 m_gic;
    cpu_arm_cortexR52 m_cpu;

    RemoteR52CPU(const sc_core::sc_module_name& n, sc_core::sc_object* obj)
        : RemoteR52CPU(n, *(dynamic_cast<QemuInstance*>(obj)))
    {
    }

    RemoteR52CPU(const sc_core::sc_module_name& n, QemuInstance& qemu_inst)
        : sc_core::sc_module(n)
        , m_broker(cci::cci_get_broker())
        , m_gdb_port("gdb_port", 0, "GDB port")
        , m_qemu_inst(qemu_inst)
        , m_router("router")
        , m_gic("gic", qemu_inst, 1)
        , m_cpu("cpu", qemu_inst)
    {
        if (!m_gdb_port.is_default_value()) {
            m_cpu.p_gdb_port = m_gdb_port;
        }

        m_cpu.socket.bind(m_router.target_socket);

        m_gic.irq_out[0].bind(m_cpu.irq_in);
        m_gic.fiq_out[0].bind(m_cpu.fiq_in);
        m_gic.virq_out[0].bind(m_cpu.virq_in);
        m_gic.vfiq_out[0].bind(m_cpu.vfiq_in);

        static const unsigned ARCH_TIMER_NS_EL1_IRQ = 16 + 14;
        m_cpu.irq_timer_phys_out.bind(m_gic.ppi_in[0][ARCH_TIMER_NS_EL1_IRQ]);
    }

private:
    cci::cci_broker_handle m_broker;
    cci::cci_param<int> m_gdb_port;
    QemuInstance& m_qemu_inst;
};

GSC_MODULE_REGISTER(RemoteR52CPU, sc_core::sc_object*);

#endif
