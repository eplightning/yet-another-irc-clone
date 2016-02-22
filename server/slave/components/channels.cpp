#include <components/channels.h>

#include <components/users.h>
#include <core/global.h>

#include <common/types.h>
#include <common/packets/slave_user.h>

YAIC_NAMESPACE

ChannelUser::ChannelUser(SharedPtr<User> &user, s32 flags) :
    m_user(user), m_flags(flags)
{

}

s32 ChannelUser::flags() const
{
    return m_flags;
}

const SharedPtr<User> &ChannelUser::user() const
{
    return m_user;
}

void ChannelUser::setFlags(s32 flag)
{
    m_flags = flag;
}

Channel::Channel(u64 id, String name, bool local) :
    m_id(id), m_local(local), m_name(name)
{

}

u64 Channel::id() const
{
    return m_id;
}

bool Channel::isLocal() const
{
    return m_local;
}

const String &Channel::name() const
{
    return m_name;
}

u32 Channel::slaveId() const
{
    return m_id >> 32;
}

HashMap<u64, SharedPtr<ChannelUser> > &Channel::users()
{
    return m_users;
}

const HashMap<u64, SharedPtr<ChannelUser> > &Channel::users() const
{
    return m_users;
}

void Channel::addUser(SharedPtr<User> &user, s32 flags)
{
    m_users[user->id()] = std::make_shared<ChannelUser>(user, flags);
}

SharedPtr<ChannelUser> Channel::user(u64 id)
{
    auto it = m_users.find(id);

    if (it == m_users.end())
        return nullptr;

    return it->second;
}

void Channel::removeUser(SharedPtr<User> &user)
{
    m_users.erase(user->id());
}

Channels::Channels() :
    m_nextId(1)
{

}

Channels::~Channels()
{

}

SharedPtr<Channel> Channels::findById(u64 id)
{
    auto it = m_list.find(id);

    if (it == m_list.end())
        return nullptr;

    return it->second;
}

SharedPtr<Channel> Channels::findByName(const String &name)
{
    for (auto &x : m_list) {
        if (x.second->name() == name)
            return x.second;
    }

    return nullptr;
}

SharedPtr<Channel> Channels::create(const String &name, SharedPtr<User> &user)
{
    u64 id = getFullId(m_nextId);

    SharedPtr<Channel> chan = std::make_shared<Channel>(id, name, true);

    chan->addUser(user, SlaveUserPackets::ChanUser::FlagOperator);

    m_list[id] = chan;
    m_nextId++;

    return chan;
}

SharedPtr<Channel> Channels::create(u64 id, const String &name)
{
    SharedPtr<Channel> chan = std::make_shared<Channel>(id, name, false);

    m_list[id] = chan;

    return chan;
}

void Channels::remove(u64 id)
{
    m_list.erase(id);
}

HashMap<u64, SharedPtr<Channel> > &Channels::list()
{
    return m_list;
}

const HashMap<u64, SharedPtr<Channel> > &Channels::list() const
{
    return m_list;
}

void Channels::setSlaveId(u32 id)
{
    m_slaveId = id;
}

u64 Channels::getFullId(u32 id) const
{
    u64 fullid = m_slaveId;
    fullid <<= 32;
    fullid |= id;

    return fullid;
}

END_NAMESPACE
