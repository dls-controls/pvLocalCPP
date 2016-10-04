/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Dave Hickin
 * @date 2016.06
 */

#include <stdexcept>

#include <pv/sharedPtr.h>

#include <pv/endpointProvider.h>


#include <shareLib.h>

namespace epics {
namespace pvLocal {

bool NamedEndpointProvider::hasEndpoint(const std::string & name)
{
    return (m_endpoints.find(name) != m_endpoints.end());
}


EndpointPtr NamedEndpointProvider::getEndpoint(const std::string & name)
{
    EndpointPtr endpoint;
    EndpointMap::const_iterator iter;
    {
        epics::pvData::Lock guard(mutex);
        iter = m_endpoints.find(name);
    }
    if (iter != m_endpoints.end())
        endpoint = iter->second;

    return endpoint;
}

void NamedEndpointProvider::registerEndpoint(const std::string & name, Endpoint::shared_pointer const & endpoint)
{
    epics::pvData::Lock guard(mutex);
    m_endpoints[name] = endpoint;
}



bool CompositeEndpointProvider::hasEndpoint(const std::string & name)
{
    epics::pvData::Lock guard(mutex);

    for (ProviderList::iterator it = providers.begin();
        it != providers.end(); ++it)
    {
        if ((*it)->hasEndpoint(name))
            return true;          
    }
    return false;
}

EndpointPtr CompositeEndpointProvider::getEndpoint(const std::string & name)
{
    epics::pvData::Lock guard(mutex);

    for (ProviderList::iterator it = providers.begin();
        it != providers.end(); ++it)
    {
        EndpointPtr endpoint = (*it)->getEndpoint(name);
        if (endpoint.get())
            return endpoint;
           
    }
    return EndpointPtr();
}


void CompositeEndpointProvider::addProvider(EndpointProviderPtr const & provider)
{
    epics::pvData::Lock guard(mutex);
    ProviderList::iterator found = find(providers.begin(), providers.end(), provider);
    if (found == providers.end())
        providers.push_back(provider);
}




}
}

