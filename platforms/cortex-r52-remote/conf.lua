-- Virtual platform configuration for Cortex-R52 with GICv3
-- Address map: see addr-map.xlsx / fw/cortex-r52/addr_map.h

function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
end

EXECUTABLE_PATH = GET("executable_path")

if EXECUTABLE_PATH == nil then
    print("Error: executable_path is not set")
end

function formatExecutable(executable)
    local isWindows = package.config:sub(1,1) == '\\'

    if isWindows then
        return "\\" .. executable .. ".exe"
    else
        return "/" .. executable
    end
end

-- Memory map constants (must match addr-map.xlsx and fw/cortex-r52/addr_map.h)
local RAM_BASE           = 0x00000000
local RAM_SIZE           = 0x00020000
local DDR_BASE           = 0x80000000
local DDR_SIZE           = 0x20000000   -- 512 MB
local GICD_BASE          = 0x2F000000
local GICD_SIZE          = 0x00010000
local GICR_BASE          = 0x2F100000
local GICR_SIZE          = 0x00020000
local UART_BASE          = 0xC0000000
local IRQ_TEST_BASE      = 0xC0001000

platform = {
    moduletype = "ContainerDeferModulesConstruct";
    quantum_ns = 10000000;

    router = {
        moduletype = "router";
        log_level = 0;
    },

    ram_0 = {
        moduletype = "gs_memory",
        target_socket = {address = RAM_BASE, size = RAM_SIZE, bind = "&router.initiator_socket"},
        shared_memory = true,
        load = {bin_file = top() .. "fw/cortex-r52/cortex-r52.bin", offset = 0},
    },

    ddr_0 = {
        moduletype = "gs_memory",
        target_socket = {address = DDR_BASE, size = DDR_SIZE, bind = "&router.initiator_socket"},
        shared_memory = true,
    },

    keep_alive_0 = {
        moduletype = "keep_alive";
    },

    charbackend_stdio_0 = {
        moduletype = "char_backend_stdio";
        read_write = true;
        ansi_highlight = "";
    };

    -- Console UART (PL011 @ UART_BASE), SPI 0 -> GIC INTID 32
    pl011_uart_0 = {
        moduletype = "Pl011",
        dylib_path = "uart-pl011",
        target_socket = {address = UART_BASE, size = 0x1000, bind = "&router.initiator_socket"},
        irq = {bind = "&plugin_0.target_signal_socket_0"},
        backend_socket = {bind = "&charbackend_stdio_0.biflow_socket"},
    },

    irq_generator = {
        target_socket = {address = IRQ_TEST_BASE, size = 0x1000, bind = "&router.initiator_socket"},
        spi_test = {bind = "&plugin_0.target_signal_socket_1"},
    },

    plugin_0 = {
        moduletype = "RemotePass",
        exec_path = EXECUTABLE_PATH .. formatExecutable("remote_cpu_r52"),
        remote_argv = {"--param", "log_level=1"},
        tlm_initiator_ports_num = 1,
        tlm_target_ports_num = 0,
        target_signals_num = 2,
        initiator_signals_num = 0,
        initiator_socket_0 = {bind = "&router.target_socket"},

        plugin_pass = {
            moduletype = "RemotePass",
            tlm_initiator_ports_num = 0,
            tlm_target_ports_num = 1,
            target_signals_num = 0,
            initiator_signals_num = 2,
            -- Single passthrough socket: RemotePass address lookup only works reliably
            -- for the first target socket. GIC MMIO is handled locally on the remote
            -- router with higher priority (lower numeric value).
            target_socket_0 = {
                address = RAM_BASE,
                size = 0x100000000,
                priority = 100,
                bind = "&router.initiator_socket",
            },
            initiator_signal_socket_0 = {bind = "&gic_0.spi_in_0"},
            initiator_signal_socket_1 = {bind = "&gic_0.spi_in_1"},
        },

        qemu_inst_mgr = {
            moduletype = "QemuInstanceManager",
        },

        qemu_inst = {
            moduletype = "QemuInstance",
            args = {"&qemu_inst_mgr", "AARCH64"},
            sync_policy = "multithread-freerunning",
        },

        router = {
            moduletype = "router",
            log_level = 0,
        },

        global_peripheral_initiator_arm_0 = {
            moduletype = "global_peripheral_initiator",
            args = {"&qemu_inst", "&cpu_0"},
            global_initiator = {bind = "&router.target_socket"},
        },

        gic_0 = {
            moduletype = "arm_gicv3",
            args = {"&qemu_inst"},
            dylib_path = "arm_gicv3",
            dist_iface = {address = GICD_BASE, size = GICD_SIZE, bind = "&router.initiator_socket"},
            redist_iface_0 = {address = GICR_BASE, size = GICR_SIZE, bind = "&router.initiator_socket"},
            num_cpus = 1,
            num_spi = 32,
            redist_region = {1},
            irq_out_0 = {bind = "&cpu_0.irq_in"},
            fiq_out_0 = {bind = "&cpu_0.fiq_in"},
            virq_out_0 = {bind = "&cpu_0.virq_in"},
            vfiq_out_0 = {bind = "&cpu_0.vfiq_in"},
        },

        cpu_0 = {
            moduletype = "cpu_arm_cortexR52",
            args = {"&qemu_inst"},
            dylib_path = "cpu_arm_cortexR52",
            mem = {bind = "&router.target_socket"},
            rvbar = RAM_BASE,
            has_el2 = false,
            psci_conduit = "disabled",
            irq_timer_phys_out = {bind = "&gic_0.ppi_in_cpu_0_30"},
        },
    },
}
