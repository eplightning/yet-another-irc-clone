#include <components/channels.h>

#include <components/users.h>
#include <core/global.h>

#include <common/types.h>

YAIC_NAMESPACE

ChannelUser::ChannelUser(SharedPtr<User> &user, s32 flags) :
    m_user(user), m_flags(flags)
{

}

s32 ChannelUser::flags() const
{
    return m_flags.load();
}

SharedPtr<User> &ChannelUser::user()
{
    return m_user;
}

void ChannelUser::setFlags(s32 flag)
{
    m_flags.store(flag);
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

std::mutex &Channel::mutex()
{
    return m_mutex;
}

const String &Channel::name() const
{
    return m_name;
}

HashMap<u64, SharedPtr<ChannelUser> > &Channel::users()
{
    return m_users;
}

void Channel::addUser(SharedPtr<User> &user, u32 flags)
{
    MutexLock lock(m_mutex);

    m_users[user->id()] = std::make_shared<ChannelUser>(user, flags);
}

void Channel::removeUser(SharedPtr<User> &user)
{
    MutexLock lock(m_mutex);

    m_users.erase(user->id());
}

Channels::Channels() :
    m_nextId(1)
{

}

Channels::~Channels()
{

}

SharedPtr<Channel> Channels::findByid(u64 id)
{
    MutexLock lock(m_mutex);

    auto it = m_list.find(id);

    if (it == m_list.end())
        return nullptr;

    return it->second;
}

SharedPtr<Channel> Channels::findByName(const String &name)
{
    MutexLock lock(m_mutex);

    for (auto &x : m_list) {
        if (x.second->name() == name)
            return x.second;
    }

    return nullptr;
}

SharedPtr<Channel> Channels::create(const String &name, SharedPtr<User> &user)
{
    MutexLock lock(m_mutex);

    for (auto &x : m_list) {
        if (x.second->name() == name)
            return nullptr;
    }

    u64 id = getFullId(m_nextId);

    SharedPtr<Channel> chan = std::make_shared<Channel>(id, name, true);

    // TODO: Operator
    chan->addUser(user, 0);

    m_list[id] = chan;
    m_nextId++;

    return chan;
}

HashMap<u64, SharedPtr<Channel> > &Channels::list()
{
    return m_list;
}

std::mutex &Channels::mutex()
{
    return m_mutex;
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
