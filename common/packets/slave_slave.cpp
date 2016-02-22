#include <common/packets/slave_slave.h>

#include <common/packet.h>
#include <common/types.h>

YAIC_NAMESPACE SSPACKETS_NAMESPACE

bool Hello::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_name);
    result &= read(payload, m_authPassword);

    return result;
}

void Hello::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_name);
    write(payload, m_authPassword);
}

u32 Hello::id() const
{
    return m_id;
}

void Hello::setId(u32 id)
{
    m_id = id;
}

const String &Hello::name() const
{
    return m_name;
}

void Hello::setName(const String &name)
{
    m_name = name;
}

u64 Hello::authPassword() const
{
    return m_authPassword;
}

void Hello::setAuthPassword(u64 authPassword)
{
    m_authPassword = authPassword;
}

Hello::Hello() :
    Packet(Packet::Type::SlaveHello)
{

}

HelloResponse::HelloResponse() :
    Packet(Packet::Type::SlaveHelloResponse)
{

}

HelloResponse::HelloResponse(u32 id) :
    Packet(Packet::Type::SlaveHelloResponse), m_id(id)
{

}

bool HelloResponse::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_authPassword);

    return result;
}

void HelloResponse::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_authPassword);
}

u32 HelloResponse::id() const
{
    return m_id;
}

u64 HelloResponse::authPassword() const
{
    return m_authPassword;
}

void HelloResponse::setAuthPassword(u64 authPassword)
{
    m_authPassword = authPassword;
}

Heartbeat::Heartbeat() :
    Packet(Packet::Type::SlaveSlaveHeartbeat)
{

}

bool Heartbeat::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);

    return true;
}

void Heartbeat::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

UserConnect::UserConnect() :
    Packet(Packet::Type::SlaveUserConnect)
{

}

bool UserConnect::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_nick);

    return result;
}

void UserConnect::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_nick);
}

u64 UserConnect::id() const
{
    return m_id;
}

void UserConnect::setId(u64 id)
{
    m_id = id;
}

const String &UserConnect::nick() const
{
    return m_nick;
}

void UserConnect::setNick(const String &value)
{
    m_nick = value;
}

ChannelNew::ChannelNew() :
    Packet(Packet::Type::SlaveChannelNew)
{

}

bool ChannelNew::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_name);

    return result;
}

void ChannelNew::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_name);
}

u64 ChannelNew::id() const
{
    return m_id;
}

void ChannelNew::setId(u64 id)
{
    m_id = id;
}

const String &ChannelNew::name() const
{
    return m_name;
}

void ChannelNew::setName(const String &name)
{
    m_name = name;
}

ChannelUser::ChannelUser() :
    Packet(Packet::Type::SlaveChannelUser)
{

}

bool ChannelUser::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_channel);
    result &= read(payload, m_user);
    result &= read(payload, m_flags);

    return result;
}

void ChannelUser::encodePayload(Vector<char> &payload) const
{
    write(payload, m_channel);
    write(payload, m_user);
    write(payload, m_flags);
}

u64 ChannelUser::channel() const
{
    return m_channel;
}

void ChannelUser::setChannel(u64 channel)
{
    m_channel = channel;
}

u64 ChannelUser::user() const
{
    return m_user;
}

void ChannelUser::setUser(u64 user)
{
    m_user = user;
}

s32 ChannelUser::flags() const
{
    return m_flags;
}

void ChannelUser::setFlags(s32 flags)
{
    m_flags = flags;
}

ChannelUserPart::ChannelUserPart() :
    Packet(Packet::Type::SlaveChannelUserPart)
{

}

bool ChannelUserPart::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_channel);
    result &= read(payload, m_user);

    return result;
}

void ChannelUserPart::encodePayload(Vector<char> &payload) const
{
    write(payload, m_channel);
    write(payload, m_user);
}

u64 ChannelUserPart::channel() const
{
    return m_channel;
}

void ChannelUserPart::setChannel(u64 channel)
{
    m_channel = channel;
}

u64 ChannelUserPart::user() const
{
    return m_user;
}

void ChannelUserPart::setUser(u64 user)
{
    m_user = user;
}

ChannelMessage::ChannelMessage() :
    Packet(Packet::Type::SlaveChannelMessage)
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

PrivateMessage::PrivateMessage() :
    Packet(Packet::Type::SlavePrivateMessage)
{

}

bool PrivateMessage::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_user);
    result &= read(payload, m_message);

    return result;
}

void PrivateMessage::encodePayload(Vector<char> &payload) const
{
    write(payload, m_user);
    write(payload, m_message);
}

u64 PrivateMessage::user() const
{
    return m_user;
}

void PrivateMessage::setUser(u64 user)
{
    m_user = user;
}

const String &PrivateMessage::message() const
{
    return m_message;
}

void PrivateMessage::setMessage(const String &message)
{
    m_message = message;
}

SyncUser::SyncUser(u64 uid, const String &name) :
    id(uid), nick(name)
{

}

SyncUsers::SyncUsers() :
    Packet(Packet::Type::SlaveSyncUsers)
{

}

bool SyncUsers::decodePayload(const Vector<char> &payload)
{
    u32 size;
    if (!readVectorSize(payload, size))
        return false;

    for (u32 i = 0; i < size; ++i) {
        u64 id;
        String nick;

        bool result = read(payload, id);
        result &= read(payload, nick);

        if (!result)
            return false;

        m_users.emplace_back(id, nick);
    }

    return true;
}

void SyncUsers::encodePayload(Vector<char> &payload) const
{
    writeVectorSize(payload, m_users.size());

    for (auto &x : m_users) {
        write(payload, x.id);
        write(payload, x.nick);
    }
}

const Vector<SyncUser> &SyncUsers::users() const
{
    return m_users;
}

void SyncUsers::addUser(u64 id, const String &nick)
{
    m_users.emplace_back(id, nick);
}

SyncChannelUser::SyncChannelUser(u64 id, s32 flags) :
    id(id), flags(flags)
{

}

SyncChannel::SyncChannel(u64 cid, const String &cname, const Vector<SyncChannelUser> &users) :
    id(cid), name(cname), users(users)
{

}

SyncChannels::SyncChannels() :
    Packet(Packet::Type::SlaveSyncChannels)
{

}

bool SyncChannels::decodePayload(const Vector<char> &payload)
{
    u32 size;
    if (!readVectorSize(payload, size))
        return false;

    for (u32 i = 0; i < size; ++i) {
        u64 id;
        String name;
        u32 count;
        Vector<SyncChannelUser> users;

        bool result = read(payload, id);
        result &= read(payload, name);
        result &= readVectorSize(payload, count);

        if (!result)
            return false;

        for (u32 j = 0; j < count; ++j) {
            u64 uid;
            s32 flags;

            bool result2 = read(payload, uid);
            result2 &= read(payload, flags);

            if (!result2)
                return false;

            users.emplace_back(uid, flags);
        }

        m_channels.emplace_back(id, name, users);
    }

    return true;
}

void SyncChannels::encodePayload(Vector<char> &payload) const
{
    writeVectorSize(payload, m_channels.size());

    for (auto &chan : m_channels) {
        write(payload, chan.id);
        write(payload, chan.name);
        writeVectorSize(payload, chan.users.size());

        for (auto &usr : chan.users) {
            write(payload, usr.id);
            write(payload, usr.flags);
        }
    }
}

const Vector<SyncChannel> &SyncChannels::channels() const
{
    return m_channels;
}

void SyncChannels::addChannel(u64 id, const String &name, const Vector<SyncChannelUser> &users)
{
    m_channels.emplace_back(id, name, users);
}

UserDisconnect::UserDisconnect(u64 id) :
    Packet(Packet::Type::SlaveUserDisconnect), m_id(id)
{

}

UserDisconnect::UserDisconnect() :
    Packet(Packet::Type::SlaveUserDisconnect)
{

}

bool UserDisconnect::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_id);
}

void UserDisconnect::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
}

u64 UserDisconnect::id() const
{
    return m_id;
}

ChannelRemove::ChannelRemove(u64 id) :
    Packet(Packet::Type::SlaveChannelRemove), m_id(id)
{

}

ChannelRemove::ChannelRemove() :
    Packet(Packet::Type::SlaveChannelRemove)
{

}

bool ChannelRemove::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_id);
}

void ChannelRemove::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
}

u64 ChannelRemove::id() const
{
    return m_id;
}



END_NAMESPACE END_NAMESPACE
