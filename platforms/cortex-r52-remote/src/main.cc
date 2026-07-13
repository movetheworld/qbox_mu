/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <systemc>

#include <chrono>
#include <string>
#include <vector>
#include <cci_configuration>
#include <cciutils.h>
#include <tlm>
#include <ports/initiator-signal-socket.h>
#include <argparser.h>
#include "remote_cpu.h"
#include <keep_alive/include/keep_alive.h>
#include <module_factory_container.h>

#if SC_VERSION_MAJOR < 3
#warning PLEASE UPDATE TO SYSTEMC 3.0, OLDER VERSIONS ARE DEPRECATED AND MAY NOT WORK
#endif

#if defined(__linux__)
#include <unistd.h>
#include <limits.h>

std::string getExecutablePath()
{
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count > 0) {
        return std::string(result, count);
    } else {
#warning "Could not get executable path for Linux"
        return std::string();
    }
}

#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#include <limits.h>

std::string getExecutablePath()
{
    char result[PATH_MAX];
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) == 0) {
        char resolved_path[PATH_MAX];
        realpath(result, resolved_path);
        return std::string(resolved_path);
    } else {
#warning "Could not get executable path for MacOS"
        return std::string();
    }
}

#elif defined(_WIN32)
#include <windows.h>
std::string getExecutablePath()
{
    std::string path;
    std::vector<char> buf;
    size_t size;
    buf.resize(MAX_PATH);
    size = GetModuleFileNameA(NULL, buf.data(), buf.size());
    if (size <= MAX_PATH) {
        path.assign(buf.data(), size);
    } else {
        std::cerr << "Executable path is bigger than MAX_PATH: " << path << std::endl;
    }

    return path;
}
#else
#error "Unsupported platform"
#endif

std::string getDirectory(const std::string& path)
{
    return path.substr(0, path.find_last_of("/\\"));
}

class IrqGenerator : public sc_core::sc_module
{
    SCP_LOGGER(());

private:
    cci::cci_broker_handle m_broker;

public:
    tlm_utils::simple_target_socket<IrqGenerator> target_socket;
    InitiatorSignalSocket<bool> m_spi_uart;
    InitiatorSignalSocket<bool> m_spi_test;
    sc_core::sc_event ev;

    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        uint64_t addr = trans.get_address();
        tlm::tlm_command cmd = trans.get_command();
        unsigned char* data = trans.get_data_ptr();
        if (cmd == tlm::TLM_WRITE_COMMAND) {
            switch (addr) {
            case 0x0:
                if (*(data) == 0x1UL) {
                    SCP_INFO(()) << "Clear test SPI";
                    m_spi_test->write(false);
                    ev.notify(sc_core::SC_ZERO_TIME);
                }
                break;
            case 0x4:
                if (*(data) == 0x1UL) {
                    SCP_INFO(()) << "Start IRQ generation";
                    ev.notify(sc_core::SC_ZERO_TIME);
                }
                break;
            default:
                SCP_INFO(()) << "No register found at address: 0x" << std::hex << addr;
                trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
                sc_core::sc_stop();
            }
        }
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    IrqGenerator(sc_core::sc_module_name const& my_name)
        : sc_core::sc_module(my_name)
        , m_broker(cci::cci_get_broker())
        , target_socket("target_socket")
        , m_spi_uart("spi_uart")
        , m_spi_test("spi_test")
    {
        SCP_INFO(()) << "IrqGenerator: Constructor called";
        SC_THREAD(test_thread);
        sensitive << ev;
        target_socket.register_b_transport(this, &IrqGenerator::b_transport);
    }

    SC_HAS_PROCESS(IrqGenerator);

private:
    void test_thread()
    {
        sc_core::wait(ev);
        m_spi_test->write(true);
    }
};

class GreenSocsPlatform : public gs::ModuleFactory::ContainerDeferModulesConstruct
{
    SCP_LOGGER(());

protected:
    cci::cci_param<int> p_quantum_ns;
    IrqGenerator irq_generator;

public:
    GreenSocsPlatform(const sc_core::sc_module_name& n)
        : gs::ModuleFactory::ContainerDeferModulesConstruct(n)
        , p_quantum_ns("quantum_ns", 1000000, "TLM-2.0 global quantum in ns")
        , irq_generator("irq_generator")
    {
        using tlm_utils::tlm_quantumkeeper;

        SCP_DEBUG(()) << "Constructor";

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);
        ModulesConstruct();
        name_bind(&irq_generator);
    }

    ~GreenSocsPlatform() {}
};

int sc_main(int argc, char* argv[])
{
    if (sc_core::sc_version_major < 3) {
        SCP_WARN()
        ("\n********************************\nWARNING, USING A DEPRECATED VERSION OF SYSTEMC, PLEASE "
         "UPGRADE\n********************************");
    }

    scp::LoggingGuard logging_guard(scp::LogConfig()
                                        .fileInfoFrom(sc_core::SC_ERROR)
                                        .logAsync(false)
                                        .logLevel(scp::log::DBGTRACE)
                                        .msgTypeFieldWidth(30));

    std::string executable_path = getDirectory(getExecutablePath());
    SCP_INFO() << "Executable Path: " << executable_path;

    gs::ConfigurableBroker m_broker{};
    cci::cci_originator orig{ "sc_main" };
    auto broker_h = m_broker.create_broker_handle(orig);
    cci::cci_param<std::string> p_executable_path{ "executable_path", executable_path, "expected build directory path",
                                                   cci::CCI_ABSOLUTE_NAME, orig };
    cci::cci_param<int> p_log_level{ "log_level", 3, "Default log level", cci::CCI_ABSOLUTE_NAME, orig };
    ArgParser ap{ broker_h, argc, argv };

    GreenSocsPlatform platform("platform");

    auto start = std::chrono::system_clock::now();
    try {
        SCP_INFO() << "SC_START";
        sc_core::sc_start();
    } catch (std::runtime_error const& e) {
        std::cerr << argv[0] << "Error: '" << e.what() << std::endl;
        exit(1);
    } catch (const std::exception& exc) {
        std::cerr << argv[0] << " Error: '" << exc.what() << std::endl;
        exit(2);
    } catch (...) {
        SCP_ERR() << "Local platform Unknown error (main.cc)!";
        exit(3);
    }
    auto end = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Simulation Time: " << sc_core::sc_time_stamp().to_seconds() << "SC_SEC" << std::endl;
    std::cout << "Simulation Duration: " << elapsed.count() << "s (Wall Clock)" << std::endl;

    return 0;
}
