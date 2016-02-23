#include <common/packets/slave_user.h>

#include <common/packet.h>
#include <common/types.h>

YAIC_NAMESPACE SUPACKETS_NAMESPACE

const int ChanUser::FlagOperator = 1 << 0;

UserHeartbeat::UserHeartbeat() :
    Packet(Packet::Type::UserHeartbeat)
{

}

bool UserHeartbeat::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);

    return true;
}

void UserHeartbeat::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

SlaveHeartbeat::SlaveHeartbeat() :
    Packet(Packet::Type::SlaveUserHeartbeat)
{

}

bool SlaveHeartbeat::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);

    return true;
}

void SlaveHeartbeat::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

Handshake::Handshake() :
    Packet(Packet::Type::Handshake)
{

}

bool Handshake::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_nick);
}

void Handshake::encodePayload(Vector<char> &payload) const
{
    write(payload, m_nick);
}

const String &Handshake::nick() const
{
    return m_nick;
}

void Handshake::setNick(const String &nick)
{
    m_nick.assign(nick);
}

HandshakeAck::HandshakeAck() :
    Packet(Packet::Type::HandshakeAck)
{

}

bool HandshakeAck::decodePayload(const Vector<char> &payload)
{
    u32 status = 0;
    bool result = read(payload, status);
    m_status = static_cast<HandshakeAck::Status>(status);

    result &= read(payload, m_userid);

    return result;
}

void HandshakeAck::encodePayload(Vector<char> &payload) const
{
    write(payload, static_cast<u32>(m_status));
    write(payload, m_userid);
}

u64 HandshakeAck::userId() const
{
    return m_userid;
}

HandshakeAck::Status HandshakeAck::status() const
{
    return m_status;
}

void HandshakeAck::setUserId(u64 userid)
{
    m_userid = userid;
}

void HandshakeAck::setStatus(HandshakeAck::Status status)
{
    m_status = status;
}

ListChannels::ListChannels() :
    Packet(Packet::Type::ListChannels)
{

}

bool ListChannels::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);

    return true;
}

void ListChannels::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

JoinChannel::JoinChannel() :
    Packet(Packet::Type::JoinChannel)
{

}

JoinChannel::JoinChannel(const String &channel) :
    Packet(Packet::Type::JoinChannel), m_name(channel)
{

}

bool JoinChannel::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_name);
}

void JoinChannel::encodePayload(Vector<char> &payload) const
{
    write(payload, m_name);
}

const String &JoinChannel::name() const
{
    return m_name;
}

PartChannel::PartChannel() :
    Packet(Packet::Type::PartChannel)
{

}

PartChannel::PartChannel(u64 id) :
    Packet(Packet::Type::PartChannel), m_id(id)
{

}

bool PartChannel::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_id);
}

void PartChannel::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
}

u64 PartChannel::id() const
{
    return m_id;
}

SendChannelMessage::SendChannelMessage() :
    Packet(Packet::Type::SendChannelMessage)
{

}

SendChannelMessage::SendChannelMessage(u64 id) :
    Packet(Packet::Type::SendChannelMessage), m_id(id)
{

}

bool SendChannelMessage::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_message);

    return result;
}

void SendChannelMessage::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_message);
}

u64 SendChannelMessage::id() const
{
    return m_id;
}

const String &SendChannelMessage::message() const
{
    return m_message;
}

void SendChannelMessage::setMessage(const String &name)
{
    m_message.assign(name);
}

Channels::Channels() :
    Packet(Packet::Type::Channels)
{

}

bool Channels::decodePayload(const Vector<char> &payload)
{
    u32 size;
    if (!readVectorSize(payload, size))
        return false;

    for (u32 i = 0; i < size; ++i) {
        String name;

        if (!read(payload, name))
            return false;

        m_channels.push_back(name);
    }

    return true;
}

void Channels::encodePayload(Vector<char> &payload) const
{
    writeVectorSize(payload, m_channels.size());

    for (auto &x : m_channels) {
        write(payload, x);
    }
}

void Channels::addChannel(const String &name)
{
    m_channels.push_back(name);
}

const Vector<String> &Channels::channels()
{
    return m_channels;
}

ChanUser::ChanUser() :
    id(0), flags(0)
{

}

ChanUser::ChanUser(u64 id, s32 flags, const String &name) :
    id(id), flags(flags), nick(name)
{

}

ChannelJoined::ChannelJoined() :
    Packet(Packet::Type::ChannelJoined)
{

}

bool ChannelJoined::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);

    u32 status = 0;
    result &= read(payload, status);
    m_status = static_cast<ChannelJoined::Status>(status);

    result &= read(payload, m_name);
    result &= read(payload, m_userFlags);

    if (!result)
        return false;

    u32 size;
    if (!readVectorSize(payload, size))
        return false;

    for (u32 i = 0; i < size; ++i) {
        u64 id;
        s32 flags;
        String nick;

        result &= read(payload, id);
        result &= read(payload, flags);
        result &= read(payload, nick);

        if (!result)
            return false;

        m_users.emplace_back(id, flags, nick);
    }

    return true;
}

void ChannelJoined::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, static_cast<u32>(m_status));
    write(payload, m_name);
    write(payload, m_userFlags);

    writeVectorSize(payload, m_users.size());

    for (auto &x : m_users) {
        write(payload, x.id);
        write(payload, x.flags);
        write(payload, x.nick);
    }
}

u64 ChannelJoined::id() const
{
    return m_id;
}

void ChannelJoined::setId(u64 id)
{
    m_id = id;
}

ChannelJoined::Status ChannelJoined::status() const
{
    return m_status;
}

void ChannelJoined::setStatus(ChannelJoined::Status status)
{
    m_status = status;
}

const String &ChannelJoined::name() const
{
    return m_name;
}

void ChannelJoined::setName(const String &name)
{
    m_name = name;
}

s32 ChannelJoined::userFlags() const
{
    return m_userFlags;
}

void ChannelJoined::setUserFlags(s32 userFlags)
{
    m_userFlags = userFlags;
}

Vector<ChanUser> &ChannelJoined::users()
{
    return m_users;
}

void ChannelJoined::addUser(u64 id, s32 flags, const String &name)
{
    m_users.emplace_back(id, flags, name);
}

ChannelParted::ChannelParted() :
    Packet(Packet::Type::ChannelParted)
{

}

bool ChannelParted::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);

    u32 status = 0;
    u32 reason = 0;

    result &= read(payload, status);
    result &= read(payload, reason);

    m_status = static_cast<Status>(status);
    m_reason = static_cast<Reason>(reason);

    return result;
}

void ChannelParted::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, static_cast<u32>(m_status));
    write(payload, static_cast<u32>(m_reason));
}

u64 ChannelParted::id() const
{
    return m_id;
}

void ChannelParted::setId(u64 id)
{
    m_id = id;
}

ChannelParted::Status ChannelParted::status() const
{
    return m_status;
}

void ChannelParted::setStatus(ChannelParted::Status status)
{
    m_status = status;
}

ChannelParted::Reason ChannelParted::reason() const
{
    return m_reason;
}

void ChannelParted::setReason(ChannelParted::Reason reason)
{
    m_reason = reason;
}

ChannelMessage::ChannelMessage() :
    Packet(Packet::Type::ChannelMessage)
{

}

bool ChannelMessage::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_channel);
    result &= read(payload, m_user);
    result &= read(payload, m_message);

    return result;
}

void ChannelMessage::encodePayload(Vector<char> &payload) const
{
    write(payload, m_channel);
    write(payload, m_user);
    write(payload, m_message);
}

u64 ChannelMessage::channel() const
{
    return m_channel;
}

void ChannelMessage::setChannel(u64 channel)
{
    m_channel = channel;
}

u64 ChannelMessage::user() const
{
    return m_user;
}

void ChannelMessage::setUser(u64 user)
{
    m_user = user;
}

const String &ChannelMessage::message() const
{
    return m_message;
}

void ChannelMessage::setMessage(const String &message)
{
    m_message = message;
}

ChannelUserJoined::ChannelUserJoined() :
    Packet(Packet::Type::ChannelUserJoined)
{

}

bool ChannelUserJoined::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_channel);
    result &= read(payload, m_user.id);
    result &= read(payload, m_user.flags);
    result &= read(payload, m_user.nick);

    return result;
}

void ChannelUserJoined::encodePayload(Vector<char> &payload) const
{
    write(payload, m_channel);
    write(payload, m_user.id);
    write(payload, m_user.flags);
    write(payload, m_user.nick);
}

u64 ChannelUserJoined::channel() const
{
    return m_channel;
}

void ChannelUserJoined::setChannel(u64 channel)
{
    m_channel = channel;
}

ChanUser &ChannelUserJoined::user()
{
    return m_user;
}

ChannelUserParted::ChannelUserParted() :
    Packet(Packet::Type::ChannelUserParted)
{

}

bool ChannelUserParted::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_channel);
    result &= read(payload, m_user);

    return result;
}

void ChannelUserParted::encodePayload(Vector<char> &payload) const
{
    write(payload, m_channel);
    write(payload, m_user);
}

u64 ChannelUserParted::channel() const
{
    return m_channel;
}

void ChannelUserParted::setChannel(u64 channel)
{
    m_channel = channel;
}

u64 ChannelUserParted::user() const
{
    return m_user;
}

void ChannelUserParted::setUser(u64 user)
{
    m_user = user;
}

ChannelUserUpdated::ChannelUserUpdated() :
    Packet(Packet::Type::ChannelUserUpdated)
{

}

bool ChannelUserUpdated::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_channel);
    result &= read(payload, m_user);
    result &= read(payload, m_flags);

    return result;
}

void ChannelUserUpdated::encodePayload(Vector<char> &payload) const
{
    write(payload, m_channel);
    write(payload, m_user);
    write(payload, m_flags);
}

u64 ChannelUserUpdated::channel() const
{
    return m_channel;
}

void ChannelUserUpdated::setChannel(u64 channel)
{
    m_channel = channel;
}

u64 ChannelUserUpdated::user() const
{
    return m_user;
}

void ChannelUserUpdated::setUser(u64 user)
{
    m_user = user;
}

s32 ChannelUserUpdated::flags() const
{
    return m_flags;
}

void ChannelUserUpdated::setFlags(s32 flags)
{
    m_flags = flags;
}

SendPrivateMessage::SendPrivateMessage() :
    Packet(Packet::Type::SendPrivateMessage)
{

}

bool SendPrivateMessage::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_user);
    result &= read(payload, m_message);

    return result;
}

void SendPrivateMessage::encodePayload(Vector<char> &payload) const
{
    write(payload, m_user);
    write(payload, m_message);
}

u64 SendPrivateMessage::user() const
{
    return m_user;
}

void SendPrivateMessage::setUser(u64 user)
{
    m_user = user;
}

const String &SendPrivateMessage::message() const
{
    return m_message;
}

void SendPrivateMessage::setMessage(const String &message)
{
    m_message = message;
}

UserDisconnected::UserDisconnected() :
    Packet(Packet::Type::UserDisconnected)
{

}

UserDisconnected::UserDisconnected(u64 user) :
    Packet(Packet::Type::UserDisconnected), m_id(user)
{

}

bool UserDisconnected::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);

    return result;
}

void UserDisconnected::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
}

u64 UserDisconnected::id() const
{
    return m_id;
}

void UserDisconnected::setId(u64 id)
{
    m_id = id;
}

UserUpdated::UserUpdated() :
    Packet(Packet::Type::UserUpdated)
{

}

bool UserUpdated::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_nick);

    return result;
}

void UserUpdated::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_nick);
}

u64 UserUpdated::id() const
{
    return m_id;
}

void UserUpdated::setId(u64 id)
{
    m_id = id;
}

const String &UserUpdated::nick() const
{
    return m_nick;
}

void UserUpdated::setNick(const String &nick)
{
    m_nick = nick;
}

PrivateMessageReceived::PrivateMessageReceived() :
    Packet(Packet::Type::PrivateMessageReceived)
{

}

bool PrivateMessageReceived::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_user);
    result &= read(payload, m_nick);
    result &= read(payload, m_message);

    return result;
}

void PrivateMessageReceived::encodePayload(Vector<char> &payload) const
{
    write(payload, m_user);
    write(payload, m_nick);
    write(payload, m_message);
}

u64 PrivateMessageReceived::user() const
{
    return m_user;
}

void PrivateMessageReceived::setUser(u64 user)
{
    m_user = user;
}

const String &PrivateMessageReceived::nick() const
{
    return m_nick;
}

void PrivateMessageReceived::setNick(const String &nick)
{
    m_nick.assign(nick);
}

const String &PrivateMessageReceived::message() const
{
    return m_message;
}

void PrivateMessageReceived::setMessage(const String &message)
{
    m_message = message;
}


END_NAMESPACE END_NAMESPACE
