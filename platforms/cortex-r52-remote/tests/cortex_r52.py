#!/usr/bin/env python3
# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause

import argparse
import os
import platform
from pathlib import Path
import sys
from sys import exit, stdout
import pexpect
import subprocess
import signal

IS_WINDOWS = platform.system() == "Windows"

if IS_WINDOWS:
    from pexpect.popen_spawn import PopenSpawn


def vp_test():
    test = False

    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--exe", metavar="", help="Path to vp executable")
    parser.add_argument("-l", "--lua", metavar="", help="Path to luafile")
    args = parser.parse_args()
    if not args.exe:
        print("vp executable file not found")
        return test
    if not args.lua:
        print("luafile is required")
        return test
    vp_path = Path(args.exe)
    lua_path = Path(args.lua)

    if IS_WINDOWS:
        vp_cmd = f'"{vp_path.as_posix()}" --gs_luafile "{args.lua}"'
        child = PopenSpawn(vp_cmd)
        child.logfile = stdout.buffer
    else:
        child = pexpect.spawn(vp_path.as_posix(), ["--gs_luafile", args.lua])
        child.logfile = stdout.buffer

    child.expect("hello world")
    child.expect("Test program is running. Listening for interrupts.")
    child.expect("SPI 33 happened")

    if IS_WINDOWS:
        child.kill(signal.SIGTERM)
    else:
        child.kill(signal.SIGQUIT)
        child.wait()

    test = True
    return test


assert(vp_test())
