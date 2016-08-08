/* channelChannelProviderLocal.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Dave Hickin
 * @date 2016.06
 */

#include <pv/serverContext.h>
#include <pv/syncChannelFind.h>

#define epicsExportSharedSymbols

#include <pv/channelProviderLocal.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::cout;
using std::endl;
using std::string;

namespace epics { namespace pvLocal { 

const string providerName("local");



ChannelProviderLocalPtr getChannelProviderLocal()
{
    static ChannelProviderLocalPtr channelProviderLocal;
    static Mutex mutex;
    Lock xx(mutex);
    if(!channelProviderLocal) {
        channelProviderLocal = ChannelProviderLocalPtr(
            new ChannelProviderLocal());
        ChannelProvider::shared_pointer xxx =
            dynamic_pointer_cast<ChannelProvider>(channelProviderLocal);
        channelProviderLocal->channelFinder =
            SyncChannelFind::shared_pointer(new SyncChannelFind(xxx));
        LocalChannelProviderFactoryPtr factory(LocalChannelProviderFactory::create(channelProviderLocal));
        registerChannelProviderFactory(factory);
    }
    return channelProviderLocal;
}

ChannelProviderLocal::ChannelProviderLocal()
:   beingDestroyed(false)
{
    endpointProvider = NamedEndpointProvider::create();
}

ChannelProviderLocal::~ChannelProviderLocal()
{
    destroy();
}

void ChannelProviderLocal::destroy()
{
    Lock xx(mutex);
    if(beingDestroyed) return;
    beingDestroyed = true;
}

string ChannelProviderLocal::getProviderName()
{
    return providerName;
}

ChannelFind::shared_pointer ChannelProviderLocal::channelFind(
    string const & channelName,
    ChannelFindRequester::shared_pointer  const &channelFindRequester)
{
    Lock xx(mutex);
    
    {
        bool found = endpointProvider->hasEndpoint(channelName);
 
        if (found)
        {
            channelFindRequester->channelFindResult(
                Status::Ok,
                channelFinder,
                true);
        }
        else
        {
            Status notFoundStatus(Status::STATUSTYPE_ERROR,"pv not found");
            channelFindRequester->channelFindResult(
                notFoundStatus,
                channelFinder,
                false);
        }
        return channelFinder;
    }
}

ChannelFind::shared_pointer ChannelProviderLocal::channelList(
    ChannelListRequester::shared_pointer const & channelListRequester)
{
    PVStringArrayPtr records;

    channelListRequester->channelListResult(Status::Ok, channelFinder, records->view(), false);
    return channelFinder;
}



class ChannelGetLocal;
typedef std::tr1::shared_ptr<ChannelGetLocal> ChannelGetLocalPtr;


class ChannelLocal;
typedef std::tr1::shared_ptr<ChannelLocal> ChannelLocalPtr;

class ChannelGetLocal :
    public ChannelGet,
    public std::tr1::enable_shared_from_this<ChannelGetLocal>
{
public:
    POINTER_DEFINITIONS(ChannelGetLocal);
    virtual ~ChannelGetLocal()
    {
    }
    static ChannelGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        GetServicePtr const &service,
        ChannelGetRequester::shared_pointer const & channelGetRequester);

    virtual void get();
    virtual void destroy();
    virtual std::tr1::shared_ptr<Channel> getChannel()
        { return channelLocal;}
    virtual void cancel(){}
    virtual void lastRequest() {}
    virtual void lock() {}
    virtual void unlock() {}

private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }

    ChannelGetLocal(ChannelLocalPtr const &channelLocal,
        GetServicePtr const & service,
        ChannelGetRequester::shared_pointer const & channelGetRequester)
    : channelLocal(channelLocal),
      service(service),
      channelGetRequester(channelGetRequester)
    {
    }

    ChannelLocalPtr channelLocal;
    GetServicePtr service;
    ChannelGetRequester::shared_pointer channelGetRequester;
};

ChannelGetLocalPtr ChannelGetLocal::create(
    ChannelLocalPtr const &channelLocal,
    GetServicePtr const &service,
    ChannelGetRequester::shared_pointer const & channelGetRequester)
{
    ChannelGetLocalPtr get(new ChannelGetLocal(channelLocal, service,
        channelGetRequester));

    channelGetRequester->channelGetConnect(
        Status::Ok, get,
        service->getPVStructure()->getStructure());

    return get;
}



void ChannelGetLocal::destroy()
{
    /*{
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }*/
    channelLocal.reset();
}

void ChannelGetLocal::get()
{
    service->get();

    BitSetPtr bitSet = BitSet::create(1);
    bitSet->set(0);

    channelGetRequester->getDone(
        Status::Ok,
        getPtrSelf(),
        service->getPVStructure(),
        service->getBitSet());
}


class ChannelRPCLocal :
    public ChannelRPC,
    public RPCResponseCallback,
    public std::tr1::enable_shared_from_this<ChannelRPCLocal>
{
private:
    Channel::shared_pointer m_channel;
    ChannelRPCRequester::shared_pointer m_channelRPCRequester;
    Service::shared_pointer m_rpcService;
    AtomicBoolean m_lastRequest;

public:
    ChannelRPCLocal(
        Channel::shared_pointer const & channel,
        ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        Service::shared_pointer const & rpcService) :
        m_channel(channel),
        m_channelRPCRequester(channelRPCRequester),
        m_rpcService(rpcService),
        m_lastRequest()
    {
    }

    virtual ~ChannelRPCLocal()
    {
        destroy();
    }

    void processRequest(RPCService::shared_pointer const & service,
                        epics::pvData::PVStructure::shared_pointer const & pvArgument)
    {
        epics::pvData::PVStructure::shared_pointer result;
        Status status = Status::Ok;
        bool ok = true;
        try
        {
            result = service->request(pvArgument);
        }
        catch (RPCRequestException& rre)
        {
            status = Status(rre.getStatus(), rre.what());
            ok = false;
        }
        catch (std::exception& ex)
        {
            status = Status(Status::STATUSTYPE_FATAL, ex.what());
            ok = false;
        }
        catch (...)
        {
            // handle user unexpected errors
            status = Status(Status::STATUSTYPE_FATAL, "Unexpected exception caught while calling RPCService.request(PVStructure).");
            ok = false;
        }

        // check null result
        if (ok && result.get() == 0)
        {
            status = Status(Status::STATUSTYPE_FATAL, "RPCService.request(PVStructure) returned null.");
        }

        m_channelRPCRequester->requestDone(status, shared_from_this(), result);

        if (m_lastRequest.get())
            destroy();

    }

    virtual void requestDone(
        epics::pvData::Status const & status,
        epics::pvData::PVStructure::shared_pointer const & result
    )
    {
        m_channelRPCRequester->requestDone(status, shared_from_this(), result);

        if (m_lastRequest.get())
            destroy();
    }

    void processRequest(RPCServiceAsync::shared_pointer const & service,
                        epics::pvData::PVStructure::shared_pointer const & pvArgument)
    {
        try
        {
            service->request(pvArgument, shared_from_this());
        }
        catch (std::exception& ex)
        {
            // handle user unexpected errors
            Status errorStatus(Status::STATUSTYPE_FATAL, ex.what());

            m_channelRPCRequester->requestDone(errorStatus, shared_from_this(), PVStructure::shared_pointer());

            if (m_lastRequest.get())
                destroy();
        }
        catch (...)
        {
            // handle user unexpected errors
            Status errorStatus(Status::STATUSTYPE_FATAL,
                               "Unexpected exception caught while calling RPCServiceAsync.request(PVStructure, RPCResponseCallback).");

            m_channelRPCRequester->requestDone(errorStatus, shared_from_this(), PVStructure::shared_pointer());

            if (m_lastRequest.get())
                destroy();
        }

        // we wait for callback to be called
    }

    virtual void request(epics::pvData::PVStructure::shared_pointer const & pvArgument)
    {
        RPCService::shared_pointer rpcService =
            std::tr1::dynamic_pointer_cast<RPCService>(m_rpcService);
        if (rpcService)
        {
            processRequest(rpcService, pvArgument);
            return;
        }

        RPCServiceAsync::shared_pointer rpcServiceAsync =
            std::tr1::dynamic_pointer_cast<RPCServiceAsync>(m_rpcService);
        if (rpcServiceAsync)
        {
            processRequest(rpcServiceAsync, pvArgument);
            return;
        }
    }

    void lastRequest()
    {
        m_lastRequest.set();
    }

    virtual Channel::shared_pointer getChannel()
    {
        return m_channel;
    }

    virtual void cancel()
    {
        // noop
    }

    virtual void destroy()
    {
        // noop
    }

    virtual void lock()
    {
        // noop
    }

    virtual void unlock()
    {
        // noop
    }
};


    epics::pvAccess::ChannelGet::shared_pointer ChannelLocal::createChannelGet(
            epics::pvAccess::ChannelGetRequester::shared_pointer const &channelGetRequester,
            PVStructure::shared_pointer const &pvRequest)
    {
        using namespace epics::pvAccess;

        if (!m_endpoint.get())
        {
            ChannelGet::shared_pointer nullPtr;
            channelGetRequester->channelGetConnect(
                Status(Status::STATUSTYPE_FATAL,
                       "Endpoint null!"),
                nullPtr, StructureConstPtr());

            return Channel::createChannelGet(channelGetRequester, pvRequest);
        }

        EndpointGetPtr epget = m_endpoint->getEndpointGet();
        if (!epget.get())
        {
            ChannelGet::shared_pointer nullPtr;
            channelGetRequester->channelGetConnect(
                Status(Status::STATUSTYPE_FATAL,
                       "Get not supported for this channel"),
                nullPtr, StructureConstPtr());

            return Channel::createChannelGet(channelGetRequester, pvRequest);
        }

        GetServicePtr service = epget->getGetService(pvRequest);

        if (!service.get())
        {
            ChannelGet::shared_pointer nullPtr;
            channelGetRequester->channelGetConnect(
                Status(Status::STATUSTYPE_FATAL,
                       "Request is not valid for Channel Get for this channel"),
                nullPtr, StructureConstPtr());
            return nullPtr;
        }

        ChannelGetLocalPtr channelGet =
            ChannelGetLocal::create(
                shared_from_this(),
                service,
                channelGetRequester);
        return channelGet;
    }



    epics::pvAccess::ChannelRPC::shared_pointer ChannelLocal::createChannelRPC(
        epics::pvAccess::ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        epics::pvData::PVStructure::shared_pointer const & pvRequest)
    {
        using namespace epics::pvAccess;

        if (channelRPCRequester.get() == 0)
            throw std::invalid_argument("channelRPCRequester == null");

        if (m_destroyed.get())
        {
            ChannelRPC::shared_pointer nullPtr;
            channelRPCRequester->channelRPCConnect(epics::pvData::Status(epics::pvData::Status::STATUSTYPE_ERROR, "channel destroyed"), nullPtr);
            return nullPtr;
        }

        // TODO use std::make_shared
        EndpointRPCPtr eprpc = m_endpoint->getEndpointRPC();
        if (!eprpc.get())
        {
            ChannelRPC::shared_pointer nullPtr;
            channelRPCRequester->channelRPCConnect(epics::pvData::Status(epics::pvData::Status::STATUSTYPE_ERROR, "RPC not supported for this channel"), nullPtr);
            return nullPtr;
        }

        Service::shared_pointer rpcService = eprpc->getRpcService(pvRequest);

        if (!rpcService.get())
        {
            ChannelRPC::shared_pointer nullPtr;
            channelRPCRequester->channelRPCConnect(epics::pvData::Status(epics::pvData::Status::STATUSTYPE_ERROR, "Request is not valid for Channel RPC for this channel"), nullPtr);
            return nullPtr;
        }

        std::tr1::shared_ptr<ChannelRPCLocal> channelRPCImpl(
            new ChannelRPCLocal(shared_from_this(), channelRPCRequester, rpcService)
        );

        channelRPCRequester->channelRPCConnect(Status::Ok, channelRPCImpl);
        return channelRPCImpl;
    }





    Channel::shared_pointer ChannelProviderLocal::createChannel(
        std::string const & channelName,
        ChannelRequester::shared_pointer const & channelRequester,
        short /*priority*/)
    {
        EndpointPtr endpoint = endpointProvider->getEndpoint(channelName);

        if (endpoint.get())
        {
            std::tr1::shared_ptr<ChannelLocal> rpcChannel = ChannelLocal::create(
                shared_from_this(),
                channelName,
                channelRequester,
                endpoint);

            channelRequester->channelCreated(Status::Ok, rpcChannel);
            return rpcChannel;
        }

        Status notFoundStatus(Status::STATUSTYPE_ERROR, "pv not found");
        channelRequester->channelCreated(
            notFoundStatus,
            Channel::shared_pointer());
        return Channel::shared_pointer();
    }

    Channel::shared_pointer ChannelProviderLocal::createChannel(
        std::string const & /*channelName*/,
        ChannelRequester::shared_pointer const & /*channelRequester*/,
        short /*priority*/,
        std::string const & /*address*/)
    {
        // this will never get called by the pvAccess server
        throw std::runtime_error("not supported");
    }

}}
