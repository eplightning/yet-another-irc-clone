#pragma once

#include <core/global.h>
#include <components/users.h>

#include <common/types.h>

#include <mutex>

YAIC_NAMESPACE

class ChannelUser {
public:
    ChannelUser(SharedPtr<User> &user, s32 flags = 0);

    s32 flags() const;
    const SharedPtr<User> &user() const;

    void setFlags(s32 flag);

protected:
    SharedPtr<User> m_user;
    s32 m_flags;
};

class Channel {
public:
    Channel(u64 id, String name, bool local = true);

    u64 id() const;
    bool isLocal() const;
    const String &name() const;

    HashMap<u64, SharedPtr<ChannelUser>> &users();
    const HashMap<u64, SharedPtr<ChannelUser>> &users() const;

    void addUser(SharedPtr<User> &user, s32 flags = 0);
    SharedPtr<ChannelUser> user(u64 id);
    void removeUser(SharedPtr<User> &user);

protected:
    u64 m_id;
    bool m_local;
    String m_name;
    HashMap<u64, SharedPtr<ChannelUser>> m_users;
};

class Channels {
public:
    Channels();
    ~Channels();

    SharedPtr<Channel> findById(u64 id);
    SharedPtr<Channel> findByName(const String &name);
    SharedPtr<Channel> create(const String &name, SharedPtr<User> &user);

    HashMap<u64, SharedPtr<Channel>> &list();
    const HashMap<u64, SharedPtr<Channel>> &list() const;

    void setSlaveId(u32 id);

    u64 getFullId(u32 id) const;

protected:
    HashMap<u64, SharedPtr<Channel>> m_list;
    u32 m_nextId;
    u32 m_slaveId;
};

END_NAMESPACE
