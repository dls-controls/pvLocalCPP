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


class epicsShareClass EndpointRPC
{
public:
    virtual Service::shared_pointer getRpcService(epics::pvData::PVStructure::shared_pointer const &) = 0;

    virtual ~EndpointRPC() {}
};

typedef std::tr1::shared_ptr<EndpointRPC> EndpointRPCPtr;

class epicsShareClass Endpoint
{
public:
    POINTER_DEFINITIONS(Endpoint);

    virtual EndpointGetPtr getEndpointGet() { return EndpointGetPtr(); }
    virtual EndpointRPCPtr getEndpointRPC() { return EndpointRPCPtr(); }

    virtual ~Endpoint() {}
};

typedef std::tr1::shared_ptr<Endpoint> EndpointPtr;

}
}

#endif  /* LOCAL_ENDPOINT_H */
