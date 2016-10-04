/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Dave Hickin
 * @date 2016.06
 */

#ifndef CHANNELPROVIDERLOCAL_H
#define CHANNELPROVIDERLOCAL_H

#ifdef epicsExportSharedSymbols
#   define channelProviderLocalEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <set>

#include <pv/lock.h>
#include <pv/pvType.h>
#include <pv/pvData.h>
#include <pv/pvCopy.h>
#include <pv/pvAccess.h>
#include <pv/status.h>
#include <pv/serverContext.h>

#include <pv/rpcServer.h>

#ifdef channelProviderLocalEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef channelProviderLocalEpicsExportSharedSymbols
#endif

#include <shareLib.h>

#include <pv/endpoint.h>
#include <pv/endpointProvider.h>

namespace epics { namespace pvLocal { 



class ChannelProviderLocal;
typedef std::tr1::shared_ptr<ChannelProviderLocal> ChannelProviderLocalPtr;
class ChannelLocal;
typedef std::tr1::shared_ptr<ChannelLocal> ChannelLocalPtr;



epicsShareFunc ChannelProviderLocalPtr getChannelProviderLocal();

/**
 * @brief An implementation of a local channel provider
 */
class epicsShareClass ChannelProviderLocal :
    public epics::pvAccess::ChannelProvider,
    public std::tr1::enable_shared_from_this<ChannelProviderLocal>
{
public:
    POINTER_DEFINITIONS(ChannelProviderLocal);

    /**
     * Destructor
     */
    virtual ~ChannelProviderLocal();

    /**
     * Destroys the channel provider.
     * Probably never called.
     */
    virtual void destroy();

    /**
     * Returns the channel provider name.
     *
     * @return <b>local</b>
     */
    virtual  std::string getProviderName();

    /**
     * Returns either channelFind for channel searched for.
     * The channelFindResult method of channelFindRequester is called before the
     * method returns.
     *
     * @param channelName the name of the channel to find.
     * @param channelFindRequester the client callback.
     * @return the ChannelFind or null.
     */
    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(
        std::string const &channelName,
        epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester);

    /** 
     * Provides a list of the channel names.
     *
     * @param channelListRequester the client callback.
     * @return the ChannelFind.
     */ 
    virtual epics::pvAccess::ChannelFind::shared_pointer channelList(
        epics::pvAccess::ChannelListRequester::shared_pointer const & channelListRequester);

    /**
     * Creates a channel.
     *
     * @param channelName the name of the channel to create.
     * @param channelRequester the client callback.
     * @param priority the priority.
     * @return the created channel or null.
     */
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        std::string const &channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const &channelRequester,
        short priority);

    /**
     * Creates a channel.
     *
     * @param channelName the name of the channel to create.
     * @param channelRequester the callback to call with the result.
     * @param priority the priority.
     * @param address the address.
     * @return created channel or null.
     */
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        std::string const &channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const &channelRequester,
        short priority,
        std::string const &address);

    void registerEndpoint(std::string const & endpointName, Endpoint::shared_pointer const & endpoint)
    {
        namedEndpointProvider->registerEndpoint(endpointName, endpoint);
    }

    void addEndpointProvider(EndpointProvider::shared_pointer const & provider)
    {
        endpointProvider->addProvider(provider);
    }

private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }

    ChannelProviderLocal();

    friend epicsShareFunc ChannelProviderLocalPtr getChannelProviderLocal();

    epics::pvData::Mutex mutex;
    bool beingDestroyed;    
    epics::pvAccess::ChannelFind::shared_pointer channelFinder;

    CompositeEndpointProviderPtr endpointProvider;
    NamedEndpointProviderPtr namedEndpointProvider;

    friend class ChannelProviderLocalRun;
};

class Endpoint;
typedef std::tr1::shared_ptr<Endpoint> EndpointPtr;

class ChannelLocal :
    public epics::pvAccess::Channel,
    public std::tr1::enable_shared_from_this<ChannelLocal>
{
private:

    epics::pvAccess::AtomicBoolean m_destroyed;

    epics::pvAccess::ChannelProvider::shared_pointer m_provider;
    std::string m_channelName;
    epics::pvAccess::ChannelRequester::shared_pointer m_channelRequester;

protected:
    ChannelLocal(
        epics::pvAccess::ChannelProvider::shared_pointer const & provider,
        std::string const & channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const & channelRequester,
        EndpointPtr const & endpoint) :
        m_provider(provider),
        m_channelName(channelName),
        m_channelRequester(channelRequester),
        m_endpoint(endpoint)
    {
    }

    EndpointPtr m_endpoint;

public:
    POINTER_DEFINITIONS(ChannelLocal);

    static ChannelLocal::shared_pointer create(
        epics::pvAccess::ChannelProvider::shared_pointer const & provider,
        std::string const & channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const & channelRequester,
        EndpointPtr const & endpoint)
    {
        std::tr1::shared_ptr<ChannelLocal> channel(
            new ChannelLocal(provider, channelName, channelRequester,
                endpoint)
        );
        return channel;
    }

    virtual ~ChannelLocal()
    {
        destroy();
    }

    virtual epics::pvAccess::ChannelProvider::shared_pointer getProvider()
    {
        return m_provider;
    }

    virtual std::string getRemoteAddress()
    {
        // local
        return getChannelName();
    }

    virtual ConnectionState getConnectionState()
    {
        return (!m_destroyed.get()) ?
               Channel::CONNECTED :
               Channel::DESTROYED;
    }

    virtual std::string getChannelName()
    {
        return m_channelName;
    }

    virtual epics::pvAccess::ChannelRequester::shared_pointer getChannelRequester()
    {
        return m_channelRequester;
    }

    virtual epics::pvAccess::AccessRights getAccessRights(epics::pvData::PVField::shared_pointer const & /*pvField*/)
    {
        return epics::pvAccess::none;
    }

    virtual void getField(epics::pvAccess::GetFieldRequester::shared_pointer const & requester,std::string const & /*subField*/)
    {
        requester->getDone(epics::pvData::Status(epics::pvData::Status::STATUSTYPE_ERROR, "Not implemented"),
                           epics::pvData::Field::shared_pointer());
    }

    virtual epics::pvAccess::ChannelGet::shared_pointer createChannelGet(
            epics::pvAccess::ChannelGetRequester::shared_pointer const &channelGetRequester,
            epics::pvData::PVStructure::shared_pointer const &pvRequest);

    virtual epics::pvAccess::ChannelPut::shared_pointer createChannelPut(
            epics::pvAccess::ChannelPutRequester::shared_pointer const &channelPutRequester,
            epics::pvData::PVStructure::shared_pointer const &pvRequest);

    virtual epics::pvAccess::ChannelRPC::shared_pointer createChannelRPC(
        epics::pvAccess::ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        epics::pvData::PVStructure::shared_pointer const & pvRequest);

    virtual epics::pvAccess::Monitor::shared_pointer createMonitor(
    		epics::pvAccess::MonitorRequester::shared_pointer const & monitorRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);

    virtual void printInfo(std::ostream& out)
    {
        out << "ChannelLocal: ";
        out << getChannelName();
        out << " [";
        out << Channel::ConnectionStateNames[getConnectionState()];
        out << "]";
    }

    virtual std::string getRequesterName()
    {
        return getChannelName();
    }

    virtual void destroy()
    {
        m_destroyed.set();
    }
};



extern const std::string providerName;

class LocalChannelProviderFactory;
typedef std::tr1::shared_ptr<LocalChannelProviderFactory> LocalChannelProviderFactoryPtr;


class LocalChannelProviderFactory : public epics::pvAccess::ChannelProviderFactory
{
    
public:
    POINTER_DEFINITIONS(LocalChannelProviderFactory);
    virtual std::string getFactoryName() { return providerName;}
    static LocalChannelProviderFactoryPtr create(
        ChannelProviderLocalPtr const &channelProvider)
    {
        LocalChannelProviderFactoryPtr xxx(
            new LocalChannelProviderFactory(channelProvider));
        registerChannelProviderFactory(xxx);
        return xxx;
    }
    virtual  epics::pvAccess::ChannelProvider::shared_pointer sharedInstance()
    {
        return channelProvider;
    }
    virtual  epics::pvAccess::ChannelProvider::shared_pointer newInstance()
    {
        return channelProvider;
    }
private:
    LocalChannelProviderFactory(
        ChannelProviderLocalPtr const &channelProvider)
    : channelProvider(channelProvider)
    {}
    ChannelProviderLocalPtr channelProvider;
};

ChannelProviderLocalPtr getChannelProviderLocal();


}}
#endif  /* CHANNELPROVIDERLOCAL_H */
