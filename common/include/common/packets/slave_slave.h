#pragma once

#include <common/packet.h>
#include <common/types.h>

#define SSPACKETS_NAMESPACE namespace SlaveSlavePackets {

YAIC_NAMESPACE SSPACKETS_NAMESPACE

class Hello : public Packet {
public:
    Hello();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u32 id() const;
    void setId(u32 id);

    const String &name() const;
    void setName(const String &name);

    u64 authPassword() const;
    void setAuthPassword(u64 authPassword);

protected:
    u32 m_id;
    String m_name;
    u64 m_authPassword;
};

class HelloResponse : public Packet {
public:
    HelloResponse();
    HelloResponse(u32 id);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u32 id() const;

    u64 authPassword() const;
    void setAuthPassword(u64 authPassword);

protected:
    u32 m_id;
    u64 m_authPassword;
};

class Heartbeat : public Packet {
public:
    Heartbeat();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class SyncUser {
public:
    SyncUser(u64 uid, const String &name);

    u64 id;
    String nick;
};

class SyncUsers : public Packet {
public:
    SyncUsers();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    const Vector<SyncUser> &users() const;
    void addUser(u64 id, const String &nick);

protected:
    Vector<SyncUser> m_users;
};

class SyncChannelUser {
public:
    SyncChannelUser(u64 id, s32 flags);

    u64 id;
    s32 flags;
};

class SyncChannel {
public:
    SyncChannel(u64 cid, const String &cname, const Vector<SyncChannelUser> &users);

    u64 id;
    String name;
    Vector<SyncChannelUser> users;
};

class SyncChannels : public Packet {
public:
    SyncChannels();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    const Vector<SyncChannel> &channels() const;
    void addChannel(u64 id, const String &name, const Vector<SyncChannelUser> &users);

protected:
    Vector<SyncChannel> m_channels;
};

class UserConnect : public Packet {
public:
    UserConnect();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;
    void setId(u64 id);

    const String &nick() const;
    void setNick(const String &value);

protected:
    u64 m_id;
    String m_nick;
};

class UserDisconnect : public Packet {
public:
    UserDisconnect(u64 id);
    UserDisconnect();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;

protected:
    u64 m_id;
};

class ChannelNew : public Packet {
public:
    ChannelNew();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;
    void setId(u64 id);

    const String &name() const;
    void setName(const String &name);

protected:
    u64 m_id;
    String m_name;
};

class ChannelRemove : public Packet {
public:
    ChannelRemove(u64 id);
    ChannelRemove();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;

protected:
    u64 m_id;
};

class ChannelUser : public Packet {
public:
    ChannelUser();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 channel() const;
    void setChannel(u64 channel);

    u64 user() const;
    void setUser(u64 user);

    s32 flags() const;
    void setFlags(s32 flags);

protected:
    u64 m_channel;
    u64 m_user;
    s32 m_flags;
};

class ChannelUserPart : public Packet {
public:
    ChannelUserPart();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 channel() const;
    void setChannel(u64 channel);

    u64 user() const;
    void setUser(u64 user);

protected:
    u64 m_channel;
    u64 m_user;
};

class ChannelMessage : public Packet {
public:
    ChannelMessage();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 channel() const;
    void setChannel(u64 channel);

    u64 user() const;
    void setUser(u64 user);

    const String &message() const;
    void setMessage(const String &message);

protected:
    u64 m_channel;
    u64 m_user;
    String m_message;
};

class PrivateMessage : public Packet {
public:
    PrivateMessage();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 user() const;
    void setUser(u64 user);

    const String &message() const;
    void setMessage(const String &message);

    u64 recipient() const;
    void setRecipient(u64 recipient);

protected:
    u64 m_user;
    u64 m_recipient;
    String m_message;
};

END_NAMESPACE END_NAMESPACE
