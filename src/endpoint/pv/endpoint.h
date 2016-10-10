/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Dave Hickin
 * @date 2016.06
 */
#ifndef LOCAL_ENDPOINT_H
#define LOCAL_ENDPOINT_H

#include <stdexcept>

#include <pv/sharedPtr.h>

#include <pv/pvAccess.h>
#include <pv/rpcService.h>

#include <shareLib.h>

namespace epics {
namespace pvLocal {

typedef epics::pvAccess::Service Service;
typedef epics::pvAccess::RPCServiceAsync RPCServiceAsync;
typedef epics::pvAccess::RPCService RPCService;
typedef epics::pvAccess::RPCService RPCService;
typedef epics::pvAccess::RPCResponseCallback RPCResponseCallback;
typedef epics::pvAccess::RPCResponseCallback RPCResponseCallback;
typedef epics::pvAccess::RPCRequestException RPCRequestException;

class epicsShareClass GetService
{
public:
    POINTER_DEFINITIONS(GetService);

    virtual void get() = 0;
    virtual epics::pvData::PVStructurePtr getPVStructure() = 0;
    virtual epics::pvData::BitSetPtr getBitSet()
    {
        using namespace epics::pvData;
        BitSetPtr bitSet = BitSet::create(1);
        bitSet->set(0);
        return bitSet;
    }
    virtual ~GetService() {}
};

typedef std::tr1::shared_ptr<GetService> GetServicePtr;

class epicsShareClass EndpointGet
{
public:

    virtual GetService::shared_pointer getGetService(epics::pvData::PVStructure::shared_pointer const &)
    {
        return GetService::shared_pointer();
    }
    virtual ~EndpointGet() {}
};

typedef std::tr1::shared_ptr<EndpointGet> EndpointGetPtr;



class epicsShareClass PutService
{
public:
    POINTER_DEFINITIONS(PutService);

    virtual void put(epics::pvData::PVStructurePtr const &pvStructure,
        epics::pvData::BitSetPtr const &bitSet) = 0;
    virtual void get() = 0;
    virtual epics::pvData::PVStructurePtr getPVStructure() = 0;
    virtual epics::pvData::BitSetPtr getBitSet()
    {
        using namespace epics::pvData;
        BitSetPtr bitSet = BitSet::create(1);
        bitSet->set(0);
        return bitSet;
    }
    virtual ~PutService() {}
};

typedef std::tr1::shared_ptr<PutService> PutServicePtr;

class epicsShareClass EndpointPut
{
public:

    virtual PutService::shared_pointer getPutService(epics::pvData::PVStructure::shared_pointer const &)
    {
        return PutService::shared_pointer();
    }
    virtual ~EndpointPut() {}
};

typedef std::tr1::shared_ptr<EndpointPut> EndpointPutPtr;


class epicsShareClass EndpointRPC
{
public:
    virtual Service::shared_pointer getRpcService(epics::pvData::PVStructure::shared_pointer const &) = 0;

    virtual ~EndpointRPC() {}
};

typedef std::tr1::shared_ptr<EndpointRPC> EndpointRPCPtr;


class epicsShareClass MonitorServiceListener
{
public:
    POINTER_DEFINITIONS(MonitorServiceListener);

    virtual void update() = 0;
    virtual ~MonitorServiceListener() {}
};

typedef std::tr1::shared_ptr<MonitorServiceListener> MonitorServiceListenerPtr;

class epicsShareClass MonitorService
{
public:
    POINTER_DEFINITIONS(MonitorService);

    virtual epics::pvData::PVStructurePtr getPVStructure() = 0;
    virtual void addListener(MonitorServiceListenerPtr const &) = 0;
    virtual ~MonitorService() {}
};

typedef std::tr1::shared_ptr<MonitorService> MonitorServicePtr;

class epicsShareClass EndpointMonitor
{
public:
    virtual MonitorService::shared_pointer getMonitorService(epics::pvData::PVStructure::shared_pointer const &) = 0;

    virtual ~EndpointMonitor() {}
};

typedef std::tr1::shared_ptr<EndpointMonitor> EndpointMonitorPtr;

class epicsShareClass Endpoint
{
public:
    POINTER_DEFINITIONS(Endpoint);

    virtual EndpointGetPtr getEndpointGet() { return EndpointGetPtr(); }
    virtual EndpointPutPtr getEndpointPut() { return EndpointPutPtr(); }
    virtual EndpointRPCPtr getEndpointRPC() { return EndpointRPCPtr(); }
    virtual EndpointMonitorPtr getEndpointMonitor() { return EndpointMonitorPtr(); }

    virtual ~Endpoint() {}
};

typedef std::tr1::shared_ptr<Endpoint> EndpointPtr;

}
}

#endif  /* LOCAL_ENDPOINT_H */
