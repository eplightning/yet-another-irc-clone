#pragma once

#include <core/global.h>
#include <components/users.h>

#include <common/types.h>

#include <atomic>
#include <mutex>

YAIC_NAMESPACE

class ChannelUser {
public:
    ChannelUser(SharedPtr<User> &user, s32 flags = 0);

    s32 flags() const;
    SharedPtr<User> &user();

    void setFlags(s32 flag);

protected:
    SharedPtr<User> m_user;
    std::atomic<s32> m_flags;
};

class Channel {
public:
    Channel(u64 id, String name, bool local = true);

    u64 id() const;
    bool isLocal() const;
    std::mutex &mutex();
    const String &name() const;

    HashMap<u64, SharedPtr<ChannelUser>> &users();

    void addUser(SharedPtr<User> &user, s32 flags = 0);
    SharedPtr<ChannelUser> user(u64 id);
    void removeUser(SharedPtr<User> &user);

protected:
    u64 m_id;
    bool m_local;
    String m_name;
    std::mutex m_mutex;
    HashMap<u64, SharedPtr<ChannelUser>> m_users;
};

class Channels {
public:
    Channels();
    ~Channels();

    SharedPtr<Channel> findByid(u64 id);
    SharedPtr<Channel> findByName(const String &name);
    SharedPtr<Channel> create(const String &name, SharedPtr<User> &user);

    HashMap<u64, SharedPtr<Channel>> &list();
    std::mutex &mutex();

    void setSlaveId(u32 id);

    u64 getFullId(u32 id) const;

protected:
    HashMap<u64, SharedPtr<Channel>> m_list;
    std::mutex m_mutex;
    u32 m_nextId;
    u32 m_slaveId;
};

END_NAMESPACE
