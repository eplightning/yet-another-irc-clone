#include <components/users.h>

#include <core/global.h>

#include <common/types.h>

YAIC_NAMESPACE

User::User(u64 id, const String &nick, SharedPtr<Client> &client) :
    m_id(id), m_client(client), m_nick(nick)
{

}

User::User(u64 id, const String &nick) :
    m_id(id), m_client(nullptr), m_nick(nick)
{

}

u64 User::id() const
{
    return m_id;
}

SharedPtr<Client> &User::client()
{
    return m_client;
}

bool User::isLocal() const
{
    return m_client.get() != nullptr;
}

u32 User::slaveId() const
{
    return m_id >> 32;
}

String User::nick()
{
    MutexLock lock(m_mutex);
    return m_nick;
}

void User::setNick(const String &nick)
{
    MutexLock lock(m_mutex);
    m_nick.assign(nick);
}

Users::Users()
{

}

Users::~Users()
{

}

SharedPtr<User> Users::addUser(u32 id, const String &nick, SharedPtr<Client> &client)
{
    u64 fullid = getFullId(id);

    MutexLock lock(m_mutex);

    if (findByNickLocked(nick))
        return nullptr;

    m_list[fullid] = std::make_shared<User>(fullid, nick, client);

    return m_list[fullid];
}

SharedPtr<User> Users::addUser(u64 id, const String &nick)
{
    MutexLock lock(m_mutex);

    if (findByNickLocked(nick))
        return nullptr;

    m_list[id] = std::make_shared<User>(id, nick);

    return m_list[id];
}

uint Users::count()
{
    MutexLock lock(m_mutex);

    return static_cast<uint>(m_list.size());
}

SharedPtr<User> Users::findById(u64 id)
{
    MutexLock lock(m_mutex);

    auto it = m_list.find(id);

    if (it == m_list.end())
        return nullptr;

    return it->second;
}

SharedPtr<User> Users::findByNick(const String &nick)
{
    MutexLock lock(m_mutex);

    return findByNickLocked(nick);
}

void Users::removeUser(u32 id)
{
    u64 fullid = getFullId(id);

    MutexLock lock(m_mutex);
    m_list.erase(fullid);
}

void Users::removeUser(u64 id)
{
    MutexLock lock(m_mutex);
    m_list.erase(id);
}

void Users::setSlaveId(u32 id)
{
    m_slaveId = id;
}

SharedPtr<User> Users::findByNickLocked(const String &nick)
{
    for (auto x : m_list) {
        if (x.second->nick() == nick)
            return x.second;
    }

    return nullptr;
}

u64 Users::getFullId(u32 id) const
{
    u64 fullid = m_slaveId;
    fullid <<= 32;
    fullid |= id;

    return fullid;
}

std::mutex &Users::mutex()
{
    return m_mutex;
}

END_NAMESPACE
