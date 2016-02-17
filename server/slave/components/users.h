#pragma once

#include <core/global.h>

#include <common/types.h>

#include <mutex>

YAIC_NAMESPACE

class User {
public:
    User(u64 id, const String &nick, SharedPtr<Client> &client);
    User(u64 id, const String &nick);

    u64 id() const;
    SharedPtr<Client> &client();
    bool isLocal() const;
    u32 slaveId() const;

    String nick();
    void setNick(const String &nick);

protected:
    u64 m_id;
    SharedPtr<Client> m_client;
    String m_nick;
    std::mutex m_mutex;
};

class Users {
public:
    Users(uint capacity = 100);
    ~Users();

    SharedPtr<User> addUser(u32 id, const String &nick, SharedPtr<Client> &client);
    SharedPtr<User> addUser(u64 id, const String &nick);

    SharedPtr<User> findById(u64 id);
    SharedPtr<User> findByNick(const String &nick);

    u64 getFullId(u32 id) const;

    void removeUser(u32 id);
    void removeUser(u64 id);

    void setSlaveId(u32 id);
    void setCapacity(uint capacity);

protected:
    SharedPtr<User> findByNickLocked(const String &nick);

    HashMap<u64, SharedPtr<User>> m_list;
    u32 m_slaveid;
    std::mutex m_mutex;
    uint m_capacity;
};

END_NAMESPACE
