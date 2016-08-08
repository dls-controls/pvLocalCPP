/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Dave Hickin
 * @date 2016.06
 */

#include <stdexcept>
#include <vector>
#include <utility>

#define epicsExportSharedSymbols

#include <pv/pvaServer.h>

#include <pv/wildcard.h>
#include <pv/endpoint.h>
#include <pv/singleServiceEndpoint.h>
#include <pv/channelProviderLocal.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::string;

namespace epics {
namespace pvLocal {


PVAServer::PVAServer()
{
    // TODO factory is never deregistered, multiple PVAServer instances create multiple factories, etc.

    m_channelProviderImpl =  epics::pvLocal::getChannelProviderLocal();

    m_serverContext = ServerContextImpl::create();
    m_serverContext->setChannelProviderName(m_channelProviderImpl->getProviderName());

    m_serverContext->initialize(getChannelProviderRegistry());
}

PVAServer::~PVAServer()
{
    // multiple destroy call is OK
    destroy();
}

void PVAServer::printInfo()
{
    std::cout << m_serverContext->getVersion().getVersionString() << std::endl;
    m_serverContext->printInfo();
}

void PVAServer::run(int seconds)
{
    m_serverContext->run(seconds);
}

struct ThreadRunnerParam {
    PVAServer::shared_pointer server;
    int timeToRun;
};

static void threadRunner(void* usr)
{
    ThreadRunnerParam* pusr = static_cast<ThreadRunnerParam*>(usr);
    ThreadRunnerParam param = *pusr;
    delete pusr;

    param.server->run(param.timeToRun);
}

/// Method requires usage of std::tr1::shared_ptr<PVAServer>. This instance must be
/// owned by a shared_ptr instance.
void PVAServer::runInNewThread(int seconds)
{
    std::auto_ptr<ThreadRunnerParam> param(new ThreadRunnerParam());
    param->server = shared_from_this();
    param->timeToRun = seconds;

    epicsThreadCreate("PVAServer thread",
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackSmall),
                      threadRunner, param.get());

    // let the thread delete 'param'
    param.release();
}

void PVAServer::destroy()
{
    m_serverContext->destroy();
}

void PVAServer::registerService(std::string const & serviceName, epics::pvLocal::RPCService::shared_pointer const & service)
{
    std::tr1::dynamic_pointer_cast<epics::pvLocal::ChannelProviderLocal>(m_channelProviderImpl)->registerEndpoint(serviceName, SingleServiceEndpoint::create(service));
}

void PVAServer::registerService(std::string const & serviceName, epics::pvLocal::RPCServiceAsync::shared_pointer const & service)
{
    std::tr1::dynamic_pointer_cast<ChannelProviderLocal>(m_channelProviderImpl)->registerEndpoint(serviceName, SingleServiceEndpoint::create(service));
}

void PVAServer::unregisterService(std::string const & serviceName)
{
    // TODO
}


void PVAServer::registerEndpoint(std::string const & endpointName, epics::pvLocal::Endpoint::shared_pointer const & endpoint)
{
    std::tr1::dynamic_pointer_cast<epics::pvLocal::ChannelProviderLocal>(m_channelProviderImpl)->registerEndpoint(endpointName, endpoint);
}


}
}
