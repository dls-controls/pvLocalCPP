/*registerChannelProviderLocal.cpp*/
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>

#include <cantProceed.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <iocsh.h>
#include <shareLib.h>
#include <epicsExit.h>

#include <pv/pvAccess.h>
#include <pv/serverContext.h>

// this declared epicsExportSharedSymbols
#include <epicsExport.h>

#include <pv/channelProviderLocal.h>

using std::cout;
using std::endl;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvLocal;


static void channelProviderLocalExitHandler(void* /*pPrivate*/) {
    getChannelProviderLocal()->destroy();
}

static void registerChannelProviderLocal(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        getChannelProviderLocal();
        epicsAtExit(channelProviderLocalExitHandler, NULL);
    }
}

extern "C" {
    epicsExportRegistrar(registerChannelProviderLocal);
}
