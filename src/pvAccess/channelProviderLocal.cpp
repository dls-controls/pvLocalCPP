/* channelProviderLocal.cpp */
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

#include <pv/bitSetUtil.h>
#include <pv/queue.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::cout;
using std::endl;
using std::string;



namespace epics { namespace pvLocal { 

const string providerName("local");

typedef Queue<MonitorElement> MonitorElementQueue;
typedef std::tr1::shared_ptr<MonitorElementQueue> MonitorElementQueuePtr;
typedef std::tr1::shared_ptr<MonitorRequester> MonitorRequesterPtr;

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
    endpointProvider = CompositeEndpointProvider::create();
    namedEndpointProvider = NamedEndpointProvider::create();
    addEndpointProvider(namedEndpointProvider);
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
    PVStringArrayPtr channels = getPVDataCreate()->createPVScalarArray<PVStringArray>();

    channelListRequester->channelListResult(Status::Ok, channelFinder, channels->view(), false);
    return channelFinder;
}



class ChannelGetLocal;
typedef std::tr1::shared_ptr<ChannelGetLocal> ChannelGetLocalPtr;

class ChannelPutLocal;
typedef std::tr1::shared_ptr<ChannelPutLocal> ChannelPutLocalPtr;

class ChannelMonitorLocal;
typedef std::tr1::shared_ptr<ChannelMonitorLocal> ChannelMonitorLocalPtr;

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

    channelGetRequester->getDone(
        Status::Ok,
        getPtrSelf(),
        service->getPVStructure(),
        service->getBitSet());
}



class ChannelPutLocal :
    public ChannelPut,
    public std::tr1::enable_shared_from_this<ChannelPutLocal>
{
public:
    POINTER_DEFINITIONS(ChannelPutLocal);
    virtual ~ChannelPutLocal()
    {
    }

    static ChannelPutLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        PutServicePtr const &service,
        ChannelPutRequester::shared_pointer const & channelPutRequester);

    virtual void put(PVStructurePtr const &pvStructure,BitSetPtr const &bitSet);
    virtual void get();
    virtual void destroy();
    virtual std::tr1::shared_ptr<Channel> getChannel()
        {return channelLocal;}
    virtual void cancel(){}
    virtual void lastRequest() {}
    virtual void lock() {} //pvRecord->lock();}
    virtual void unlock() {} //pvRecord->unlock();}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }

    ChannelPutLocal(ChannelLocalPtr const &channelLocal,
        PutServicePtr const & service,
        ChannelPutRequester::shared_pointer const & channelPutRequester)
    : channelLocal(channelLocal),
      service(service),
      channelPutRequester(channelPutRequester)
    {
    }

    ChannelLocalPtr channelLocal;
    PutServicePtr service;
    ChannelPutRequester::shared_pointer channelPutRequester;
};

ChannelPutLocalPtr ChannelPutLocal::create(
    ChannelLocalPtr const &channelLocal,
    PutServicePtr const &service,
    ChannelPutRequester::shared_pointer const & channelPutRequester)
{
    ChannelPutLocalPtr put(new ChannelPutLocal(channelLocal, service,
        channelPutRequester));

    channelPutRequester->channelPutConnect(
        Status::Ok, put,
        service->getPVStructure()->getStructure());

    return put;
}


void ChannelPutLocal::destroy()
{
    /*{
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }*/
    channelLocal.reset();
}


void ChannelPutLocal::get()
{
    service->get();

    channelPutRequester->getDone(
        Status::Ok,
        getPtrSelf(),
        service->getPVStructure(),
        service->getBitSet());
}

void ChannelPutLocal::put(
    PVStructurePtr const &pvStructure,BitSetPtr const &bitSet)
{
    service->put(pvStructure,bitSet);
    channelPutRequester->putDone(Status::Ok,getPtrSelf());
}

class ChannelMonitorLocal :
		public Monitor,
        public MonitorServiceListener,
		public std::tr1::enable_shared_from_this<ChannelMonitorLocal>
{
public:
    ChannelMonitorLocal(ChannelLocalPtr const &channelLocal,
        PVStructurePtr const & pvRequest,
        MonitorService::shared_pointer const & service,
        MonitorRequester::shared_pointer const & monitorRequester)
    : channelLocal(channelLocal),
      service(service),
      monitorRequester(monitorRequester),
      state(idle)//,
      //isGroupPut(false),
      //dataChanged(false)
    {
        init(pvRequest);
    }

    virtual Status start()
    {
        Lock xx(mutex);

        //if(state==destroyed) return wasDestroyedStatus;
        //if(state==active) return alreadyStartedStatus;

        state = active;
        queue->clear();
        //isGroupPut = false;
        activeElement = queue->getFree();
        activeElement->changedBitSet->clear();
        activeElement->overrunBitSet->clear();
        activeElement->changedBitSet->set(0);
        releaseActiveElement();
        return Status::Ok;
    }

    void init()
    {
        service->addListener(shared_from_this());
    }

    virtual Status stop()
    {
    	return Status::Ok;
    }

    virtual MonitorElementPtr poll()
    {
        Lock xx(queueMutex);
        if (state != active)
            return MonitorElementPtr();
        return queue->getUsed();
    }

    virtual void release(MonitorElementPtr const & monitorElement)
    {
        Lock xx(queueMutex);
        if (state != active)
            return;
        queue->releaseUsed(monitorElement);
    }

    virtual void destroy()
    {
    	channelLocal.reset();
    }




void update()
{
    if(state!=active) return;
    {
        Lock xx(mutex);
        //size_t offset = pvCopy->getCopyOffset(pvRecordField->getPVField());
        BitSetPtr const &changedBitSet = activeElement->changedBitSet;
        BitSetPtr const &overrunBitSet = activeElement->overrunBitSet;
        //bool isSet = changedBitSet->get(offset);
        //changedBitSet->set(offset);
        //if(isSet) overrunBitSet->set(offset);
        //dataChanged = true;

        changedBitSet->set(0);
        overrunBitSet->clear();
    }

    /*if(!isGroupPut) {
        releaseActiveElement();
        dataChanged = false;
    }*/
    releaseActiveElement();
}



    bool init(PVStructurePtr const & pvRequest)
    {
        PVFieldPtr pvField;
        size_t queueSize = 2;
        PVStructurePtr pvOptions = pvRequest->getSubField<PVStructure>("record._options");
        MonitorRequesterPtr requester = monitorRequester.lock();
        if(!requester) return false;
            if(pvOptions) {
            PVStringPtr pvString  = pvOptions->getSubField<PVString>("queueSize");
            if(pvString) {
                try {
                    int32 size;
                    std::stringstream ss;
                    ss << pvString->get();
                    ss >> size;
                    queueSize = size;
                } catch (...) {
                     requester->message("queueSize " +pvString->get() + " illegal",errorMessage);
                     return false;
                }
            }
        }

        pvCopy = PVCopy::create(
            service->getPVStructure(),
            CreateRequest::create()->createRequest(""),"");

    if(queueSize<2) queueSize = 2;
    std::vector<MonitorElementPtr> monitorElementArray;
    monitorElementArray.reserve(queueSize);

    for(size_t i=0; i<queueSize; i++) {
         PVStructurePtr pvStructure = pvCopy->createPVStructure();
         MonitorElementPtr monitorElement(
             new MonitorElement(pvStructure));
         monitorElementArray.push_back(monitorElement);
    }

    queue = MonitorElementQueuePtr(new MonitorElementQueue(monitorElementArray));

    return true;
}

private:

    ChannelLocalPtr channelLocal;
    MonitorService::shared_pointer service;

    enum MonitorState {idle,active, destroyed};

    ChannelMonitorLocalPtr getPtrSelf()
    {
        return shared_from_this();
    }

    void releaseActiveElement()
    {
        {
            Lock xx(queueMutex);

            if(state!=active) return;
            MonitorElementPtr newActive = queue->getFree();
            if(!newActive) return;
            pvCopy->updateCopyFromBitSet(activeElement->pvStructurePtr,activeElement->changedBitSet);
            BitSetUtil::compress(activeElement->changedBitSet,activeElement->pvStructurePtr);
            BitSetUtil::compress(activeElement->overrunBitSet,activeElement->pvStructurePtr);
            queue->setUsed(activeElement);
            activeElement = newActive;
            activeElement->changedBitSet->clear();
            activeElement->overrunBitSet->clear();
        }

        MonitorRequesterPtr requester = monitorRequester.lock();
        if(!requester) return;
        requester->monitorEvent(getPtrSelf());
        return;
    }

    MonitorRequester::weak_pointer monitorRequester;
    MonitorState state;
    PVCopyPtr pvCopy;
    MonitorElementQueuePtr queue;
    MonitorElementPtr activeElement;
    //bool isGroupPut;
    //bool dataChanged;
    Mutex mutex;
    Mutex queueMutex;
};


class epicsShareClass ChannelRPCLocal :
    public std::tr1::enable_shared_from_this<ChannelRPCLocal>,
    public ChannelRPC,
    public RPCResponseCallback
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
                        epics::pvData::PVStructure::shared_pointer const & pvArgument);
/*    {
        epics::pvData::PVStructure::shared_pointer result;
        Status status = Status::Ok;
        bool ok = true;
        
        printf("*** RPC creating thread called\n");
        std::auto_ptr<RPCThreadRunnerParam> param(new RPCThreadRunnerParam());
        param->rpcPtr = this;
        param->service = service;
        param->pvArgument = pvArgument;
        
            epicsThreadCreate("PVA Rpc thread",
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackSmall),
                      threadRunner, param.get());
     }*/

     void processThread(RPCService::shared_pointer const & service,
                        epics::pvData::PVStructure::shared_pointer const & pvArgument)
     {
        epics::pvData::PVStructure::shared_pointer result;
        Status status = Status::Ok;
        bool ok = true;
        
        //printf("*** RPC processRequest 1 called\n");
        try
        {
        //printf("*** RPC processRequest 2 called\n");
            result = service->request(pvArgument);
        //printf("*** RPC processRequest 3 called\n");
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

        //printf("*** RPC processRequest 4 called\n");
        m_channelRPCRequester->requestDone(status, shared_from_this(), result);
        //printf("*** RPC processRequest 5 called\n");

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
       //printf("*** RPC processRequest 2 called\n");
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

struct RPCThreadRunnerParam 
{
  std::tr1::shared_ptr<ChannelRPCLocal> rpcPtr;
  RPCService::shared_pointer service;
  epics::pvData::PVStructure::shared_pointer pvArgument;
};

static void rpcThreadRunner(void* usr)
{
    RPCThreadRunnerParam* pusr = static_cast<RPCThreadRunnerParam*>(usr);
    RPCThreadRunnerParam param = *pusr;
    //delete pusr;
    param.rpcPtr->processThread(param.service, param.pvArgument);
}

void ChannelRPCLocal::processRequest(RPCService::shared_pointer const & service,
                        epics::pvData::PVStructure::shared_pointer const & pvArgument)
    {
        epics::pvData::PVStructure::shared_pointer result;
        Status status = Status::Ok;
        
        printf("*** RPC creating thread called\n");
        std::auto_ptr<RPCThreadRunnerParam> param(new RPCThreadRunnerParam());
        param->rpcPtr = shared_from_this();
        param->service = service;
        param->pvArgument = pvArgument;
        
        printf("*** RPC thread creation\n");
            epicsThreadCreate("PVA Rpc thread",
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackSmall),
                      rpcThreadRunner, param.get());
                      
         param.release();
     }


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

    epics::pvAccess::ChannelPut::shared_pointer ChannelLocal::createChannelPut(
            epics::pvAccess::ChannelPutRequester::shared_pointer const &channelPutRequester,
            PVStructure::shared_pointer const &pvRequest)
    {
        using namespace epics::pvAccess;

        if (!m_endpoint.get())
        {
            ChannelPut::shared_pointer nullPtr;
            channelPutRequester->channelPutConnect(
                Status(Status::STATUSTYPE_FATAL,
                       "Endpoint null!"),
                nullPtr, StructureConstPtr());

            return Channel::createChannelPut(channelPutRequester, pvRequest);
        }

        EndpointPutPtr epput = m_endpoint->getEndpointPut();
        if (!epput.get())
        {
            ChannelPut::shared_pointer nullPtr;
            channelPutRequester->channelPutConnect(
                Status(Status::STATUSTYPE_FATAL,
                       "Put not supported for this channel"),
                nullPtr, StructureConstPtr());

            return Channel::createChannelPut(channelPutRequester, pvRequest);
        }
        PutServicePtr service = epput->getPutService(pvRequest);

        if (!service.get())
        {
            ChannelPut::shared_pointer nullPtr;
            channelPutRequester->channelPutConnect(
                Status(Status::STATUSTYPE_FATAL,
                       "Request is not valid for Channel Put for this channel"),
                nullPtr, StructureConstPtr());
            return nullPtr;
        }

        ChannelPutLocalPtr channelPut =
            ChannelPutLocal::create(
                shared_from_this(),
                service,
                channelPutRequester);
        return channelPut;
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


    epics::pvAccess::Monitor::shared_pointer ChannelLocal::createMonitor(
    		epics::pvAccess::MonitorRequester::shared_pointer const & monitorRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest)
    {
        using namespace epics::pvAccess;
        if (!m_endpoint.get())
        {
            Monitor::shared_pointer nullPtr;
            monitorRequester->monitorConnect(
            		Status(Status::STATUSTYPE_FATAL, "Endpoint null!"),
					nullPtr,
					StructureConstPtr());

            return Channel::createMonitor(monitorRequester, pvRequest);
        }

        EndpointMonitorPtr epmon = m_endpoint->getEndpointMonitor();
        if (!epmon.get())
        {
            Monitor::shared_pointer nullPtr;
            monitorRequester->monitorConnect(
            		Status(Status::STATUSTYPE_FATAL, "Monitor not supported for this channel"),
					nullPtr,
					StructureConstPtr());

            return Channel::createMonitor(monitorRequester, pvRequest);
        }

        MonitorService::shared_pointer monService = epmon->getMonitorService(pvRequest);

        if (!monService.get())
        {
            Monitor::shared_pointer nullPtr;
            monitorRequester->monitorConnect(
                Status(Status::STATUSTYPE_FATAL,
                       "Request is not a valid monitor for this channel"),
                nullPtr, StructureConstPtr());
            return nullPtr;
        }

        std::tr1::shared_ptr<ChannelMonitorLocal> channelMonitorImpl(
                    new ChannelMonitorLocal(shared_from_this(), pvRequest, monService, monitorRequester)
                );
        channelMonitorImpl->init();

        // Call into python through the monitor service and ask for the structure
		PVStructurePtr structure = monService->getPVStructure();

		// Notify requester we are ready, pass on the structure
		monitorRequester->monitorConnect(Status::Ok, channelMonitorImpl, structure->getStructure());

        return channelMonitorImpl;
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
