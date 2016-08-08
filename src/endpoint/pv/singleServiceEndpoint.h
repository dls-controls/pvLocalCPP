/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Dave Hickin
 * @date 2016.06
 */
#ifndef SINGLESERVICEENDPOINT_H
#define SINGLESERVICEENDPOINT_H

#include <stdexcept>

#include <pv/sharedPtr.h>

#include <pv/pvAccess.h>

#include <pv/endpoint.h>

#include <shareLib.h>

namespace epics {
namespace pvLocal {


class SingleServiceEndpoint : public Endpoint, public EndpointRPC,
    public std::tr1::enable_shared_from_this<SingleServiceEndpoint>
{
public:
    POINTER_DEFINITIONS(SingleServiceEndpoint);

    static SingleServiceEndpoint::shared_pointer create(Service::shared_pointer const & rpcService)
    {
        return SingleServiceEndpoint::shared_pointer(new SingleServiceEndpoint(rpcService));
    }

    virtual EndpointRPCPtr getEndpointRPC()
    {
        return shared_from_this();
    }

    virtual Service::shared_pointer getRpcService(epics::pvData::PVStructure::shared_pointer const &)
    {
        return m_rpcService;
    }

private:
    SingleServiceEndpoint(Service::shared_pointer const & rpcService)
    : m_rpcService(rpcService)
    {
    }


    Service::shared_pointer m_rpcService;
};

}
}

#endif  /* SINGLESERVICEENDPOINT_H */
