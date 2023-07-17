/* Copyright (C) 2023 Aviatrix
 *
 */

/**
 * \ingroup lipcppacket
 *
 * @{
 */

/**
 * \file
 *
 * \author Alan M. Carroll (amc@apache.org)
 *
 * Local IPC based packet runmode
 *
 */

#include "runmodes.h"
#include "util-runmodes.h"

#include "source-lipc.h"

extern uint16_t max_pending_packets;

static int LIPCConfigGeThreadsCount(void *conf)
{
    return ((LIPCIfaceConfig *)conf)->threads;
}

static void* ParseLIPCConfig(char const * data) {
    if (NULL == data) { return NULL; }

    LIPCIFaceConfig * cfg = SCMalloc(sizeof(*cfg));
    if (NULL == cfg) {
        return NULL;
    }
    SC_ATOMIC_INIT(cfg->ref);
    cfg->DerefFunc = LIPCDerefConfig;
    cfg->threads = 1;
    cfg->req_buffer_size = 4096;

    return cfg;
}

int RunModLIPCSingle(void) {
    SCEnter();

    RunModInitialize();
    TimeModeSetLive();

    SCLogDebug("RunModeLIPCSingle initialised");
    int r = RunModeSetLiveLiveCaptureSingle(
            ParseLIPCConfig,
            LIPCConfigGeThreadsCount,
            "ReceiveLIPC", "DecodeLIPC",
            thread_name_single, "lipc"
            );

    if (r != 0) {
        FatalError("Runmode start failed [%d]", r);
    }

    SCLogDebug("RunModeLIPCSingle initialised");
    SCReturnInt(0);
}

int RunModeIdsPcapAutoFp(void)
{
    SCEnter();
    RunModeInitialize();
    TimeModeSetLive();

    int r = RunModeSetLiveCaptureAutoFp(
            ParseLIPCConfig,
            LIPCConfigGeThreadsCount,
            "ReceiveLIPC", "DecodeLIPC",
            thread_name_autofp, "lipc"
            );

    if (r != 0) {
        FatalError("Runmode start failed [%d]", r);
    }

    SCLogDebug("RunModeLIPCAutoFp initialised");
    SCReturnInt(0);
}

int RunModeLIPCWorkers(void) {
    SCEnter();

    RunModeInitialize();
    TimeModeSetLive();

    int r = RunModeSetLiveCaptureWorkers(
            ParseLIPCConfig, LIPCConfigGeThreadsCount,
            "ReceiveLIPC", "DecodeLIPC", "lipc"
            );

    if (r != 0) {
        FatalError("Runmode start failed [%d]", r);
    }

    SCLogDebug("RunModeLIPCWorkers initialised");
    SCReturnInt(0);
}

void RunModeIdsLIPCRegister(void) {
    RunModeRegisterNewRunMode(RUNMODE_LIPC_DEV, "single", "Single threaded IPC mode",
            RunModeLIPCSingle, NULL);
    RunModeRegisterNewRunMode(RUNMODE_LIPC_DEV, "autofp",
            "Multi threaded IPC live mode.  Packets from "
            "each flow are assigned to a single detect thread, "
            "unlike \"lipc_live_auto\" where packets from "
            "the same flow can be processed by any detect "
            "thread",
            RunModeLIPCAutoFp, NULL);
    RunModeRegisterNewRunMode(RUNMODE_LIPC_DEV, "workers",
            "Workers IPC live mode, each thread does all"
            " tasks from acquisition to logging",
            RunModeLIPCWorkers, NULL);

    return;
}

static void LIPCDerefConfig(void * data) {
    LIPCIfaceConfig * cfg = (LIPCIfaceConfig*) data;
    if (SC_ATOMIC_SUB(cfg->ref, 1) == 1) {
        SCFree(cfg);
    }
}

/**
 * @}
 */
