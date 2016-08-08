/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Dave Hickin
 * @date 2016.06
 */

#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#ifdef epicsExportSharedSymbols
#   define rpcServerEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/sharedPtr.h>

#ifdef rpcServerEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef rpcServerEpicsExportSharedSymbols
#endif

#include <pv/pvAccess.h>
#include <pv/endpoint.h>

#include <pv/serverContext.h>

#include <shareLib.h>

namespace epics {
namespace pvLocal {

class epicsShareClass PVAServer :
    public std::tr1::enable_shared_from_this<PVAServer>
{
private:

    pvAccess::ServerContextImpl::shared_pointer m_serverContext;
    pvAccess::ChannelProviderFactory::shared_pointer m_channelProviderFactory;
    pvAccess::ChannelProvider::shared_pointer m_channelProviderImpl;

    // TODO no thread poll implementation

public:
    POINTER_DEFINITIONS(PVAServer);

    PVAServer();

    virtual ~PVAServer();

    void registerService(std::string const & serviceName, epics::pvLocal::RPCService::shared_pointer const & service);

    void registerService(std::string const & serviceName, epics::pvLocal::RPCServiceAsync::shared_pointer const & service);

    void unregisterService(std::string const & serviceName);

    void registerEndpoint(std::string const & endpointName, 
        epics::pvLocal::Endpoint::shared_pointer const & endpoint);

    void run(int seconds = 0);

    /// Method requires usage of std::tr1::shared_ptr<PVAServer>. This instance must be
    /// owned by a shared_ptr instance.
    void runInNewThread(int seconds = 0);

    void destroy();

    /**
     * Display basic information about the context.
     */
    void printInfo();

};

epicsShareFunc pvAccess::Channel::shared_pointer createRPCChannel(pvAccess::ChannelProvider::shared_pointer const & provider,
        std::string const & channelName,
        pvAccess::ChannelRequester::shared_pointer const & channelRequester,
        epics::pvLocal::Service::shared_pointer const & rpcService);

}
}

#endif  /* LOCALSERVER_H */
