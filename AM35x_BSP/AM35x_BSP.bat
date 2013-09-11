REM
REM              Texas Instruments OMAP(TM) Platform Software
REM  (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
REM 
REM  Use of this software is controlled by the terms and conditions found
REM  in the license agreement under which this software has been supplied.
REM

REM --------------------------------------------------------------------------
REM Build Environment
REM --------------------------------------------------------------------------

REM Always copy binaries to flat release directory
set WINCEREL=1
REM Generate .cod, .lst files
set WINCECOD=1

REM ----OS SPECIFIC VERSION SETTINGS----------

REM if "%SG_OUTPUT_ROOT%" == "" (set SG_OUTPUT_ROOT=%_PROJECTROOT%\cesysgen) 

REM set _PLATCOMMONLIB=%_PLATFORMROOT%\common\lib
REM set _TARGETPLATLIB=%_TARGETPLATROOT%\lib
REM set _EBOOTLIBS=%SG_OUTPUT_ROOT%\oak\lib
REM set _KITLLIBS=%SG_OUTPUT_ROOT%\oak\lib

set _RAWFILETGT=%_TARGETPLATROOT%\target\%_TGTCPU%

set BSP_WCE=1


REM --------------------------------------------------------------------------
REM Initial Operating Point - VDD1 voltage, MPU (CPU) and IVA speeds
REM --------------------------------------------------------------------------

REM Select initial operating point (CPU speed, VDD1 voltage).
REM Note that this controls the operating point selected by the bootloader.
REM If the power management subsystem is enabled, the initial operating point 
REM it uses is controlled by registry entries.
REM Use 5 for MPU[600Mhz @ 1.200V] 
REM Use 4 for MPU[550Mhz @ 1.200V] 
REM Use 3 for MPU[500Mhz @ 1.200V] 
REM Use 2 for MPU[250Mhz @ 1.200V] 
REM Use 1 for MPU[125Mhz @ 1.200V]
set BSP_OPM_SELECT=5

REM DVFS off, Smart Reflex off (default)
set BSP_NODVFS=1
set BSP_NOCPUPOLICY=1
set BSP_NOINTRLAT=1
set BSP_NOSYSSTATEPOLICY=1
set BSP_NOSMARTREFLEXPOLICY=1


REM --------------------------------------------------------------------------
REM Misc. settings
REM --------------------------------------------------------------------------

REM TI BSP builds its own ceddk.dll. Setting this IMG var excludes default CEDDK from the OS image.
set IMGNODFLTDDK=1

REM Use this to read the default MAC address of the EMAC from the EFUSE.
set BSP_READ_MAC_FROM_FUSE=1

REM Use this to enable 4 bit mmc cards
set BSP_EMMCFEATURE=1

REM TI OMAP shell extensions command line utility (do.exe)
REM Usage: Open a command prompt from the start menu.
REM        Type "do" followed by the command you want to use. 
REM        Example: "do ?" will show the help and available commands.
REM Note: If a KITL connection is available, the TI OMAP shell extensions can be used from the target shell.
set BSP_SHELL_EXTENSIONS_DO_CMD=1

:EXIT
