#pragma once

#include <common/packet.h>
#include <common/types.h>

#define SUPACKETS_NAMESPACE namespace SlaveUserPackets {

YAIC_NAMESPACE SUPACKETS_NAMESPACE

class UserHeartbeat : public Packet {
public:
    UserHeartbeat();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class SlaveHeartbeat : public Packet {
public:
    SlaveHeartbeat();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class Handshake : public Packet {
public:
    Handshake();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    const String &nick() const;

    void setNick(const String &nick);

protected:
    String m_nick;
};

class HandshakeAck : public Packet {
public:
    enum class Status : u32 {
        Ok = 0,
        UnknownError = 1,
        InvalidNick = 2,
        Full = 3
    };

    HandshakeAck();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 userId() const;
    Status status() const;

    void setUserId(u64 userid);
    void setStatus(Status status);

protected:
    Status m_status;
    u64 m_userid;
};

class ListChannels : public Packet {
public:
    ListChannels();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class JoinChannel : public Packet {
public:
    JoinChannel();
    JoinChannel(const String &channel);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    const String &name() const;

protected:
    String m_name;
};

class PartChannel : public Packet {
public:
    PartChannel();
    PartChannel(u64 id);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;

protected:
    u64 m_id;
};

class SendChannelMessage : public Packet {
public:
    SendChannelMessage();
    SendChannelMessage(u64 id);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;
    const String &message() const;

    void setMessage(const String &name);

protected:
    u64 m_id;
    String m_message;
};

class SendPrivateMessage : public Packet {
public:
    SendPrivateMessage();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 user() const;
    void setUser(u64 user);

    const String &message() const;
    void setMessage(const String &message);

protected:
    u64 m_user;
    String m_message;
};

class Channels : public Packet {
public:
    Channels();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    void addChannel(const String &name);
    const Vector<String> &channels();

protected:
    Vector<String> m_channels;
};

class ChanUser {
public:
    const static int FlagOperator;

    ChanUser();
    ChanUser(u64 id, s32 flags, const String &name);

    u64 id;
    s32 flags;
    String nick;
};

class ChannelJoined : public Packet {
public:
    enum class Status : u32 {
        Ok = 1,
        UnknownError = 2
    };

    ChannelJoined();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;
    void setId(u64 id);

    Status status() const;
    void setStatus(Status status);

    const String &name() const;
    void setName(const String &name);

    s32 userFlags() const;
    void setUserFlags(s32 userFlags);

    Vector<ChanUser> &users();
    void addUser(u64 id, s32 flags, const String &name);

protected:
    u64 m_id;
    Status m_status;
    String m_name;
    s32 m_userFlags;
    Vector<ChanUser> m_users;
};

class ChannelParted : public Packet {
public:
    enum class Status : u32 {
        Ok = 0,
        UnknownError = 1
    };

    enum class Reason : u32 {
        Requested = 0,
        Unknown = 1,
        Kicked = 2
    };

    ChannelParted();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;
    void setId(u64 id);

    Status status() const;
    void setStatus(Status status);

    Reason reason() const;
    void setReason(Reason reason);

protected:
    u64 m_id;
    Status m_status;
    Reason m_reason;
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

class ChannelUserJoined : public Packet {
public:
    ChannelUserJoined();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 channel() const;
    void setChannel(u64 channel);

    ChanUser &user();

protected:
    u64 m_channel;
    ChanUser m_user;
};

class ChannelUserParted : public Packet {
public:
    ChannelUserParted();

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

class ChannelUserUpdated : public Packet {
public:
    ChannelUserUpdated();

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

class UserDisconnected : public Packet {
public:
    UserDisconnected();
    UserDisconnected(u64 user);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;
    void setId(u64 id);

protected:
    u64 m_id;
};

class UserUpdated : public Packet {
public:
    UserUpdated();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 id() const;
    void setId(u64 id);

    const String &nick() const;
    void setNick(const String &nick);

protected:
    u64 m_id;
    String m_nick;
};

class PrivateMessageReceived : public Packet {
public:
    PrivateMessageReceived();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 user() const;
    void setUser(u64 user);

    const String &message() const;
    void setMessage(const String &message);

protected:
    u64 m_user;
    String m_message;
};

END_NAMESPACE END_NAMESPACE
