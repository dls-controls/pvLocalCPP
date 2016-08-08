/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Dave Hickin
 * @date 2016.06
 */

#ifndef LOCAL_ENDPOINTPROVIDER_H
#define LOCAL_ENDPOINTPROVIDER_H

#include <stdexcept>

#include <pv/sharedPtr.h>

#include <pv/endpoint.h>


#include <shareLib.h>


namespace epics {
namespace pvLocal {

class epicsShareClass EndpointProvider
{
public:
    POINTER_DEFINITIONS(EndpointProvider);

    virtual bool hasEndpoint(const std::string & name) {return false;}

    virtual EndpointPtr getEndpoint(const std::string & name) { return EndpointPtr(); }

    virtual ~EndpointProvider() {}
};

typedef std::tr1::shared_ptr<EndpointProvider> EndpointProviderPtr;



class epicsShareClass NamedEndpointProvider : public EndpointProvider
{
public:
    POINTER_DEFINITIONS(NamedEndpointProvider);

    static NamedEndpointProvider::shared_pointer create()
    {
        return NamedEndpointProvider::shared_pointer(new NamedEndpointProvider());
    }

    virtual bool hasEndpoint(const std::string & name); 

    virtual EndpointPtr getEndpoint(const std::string & name);

    virtual void registerEndpoint(const std::string & name, Endpoint::shared_pointer const & endpoint);

    virtual ~NamedEndpointProvider() {}

private:
    typedef std::map<std::string, Endpoint::shared_pointer> EndpointMap;
    EndpointMap m_endpoints;

    epics::pvData::Mutex mutex;

    NamedEndpointProvider() {}
};

typedef std::tr1::shared_ptr<NamedEndpointProvider> NamedEndpointProviderPtr;



class epicsShareClass CompositeEndpointProvider : public EndpointProvider
{
public:
    POINTER_DEFINITIONS(CompositeEndpointProvider);

    virtual EndpointPtr getEndpoint(const std::string & name) { return EndpointPtr(); }

    virtual ~CompositeEndpointProvider() {}
};

typedef std::tr1::shared_ptr<CompositeEndpointProvider> CompositeEndpointProviderPtr;



}
}

#endif  /* LOCAL_ENDPOINTPROVIDER_H */
