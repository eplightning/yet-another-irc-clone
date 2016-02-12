#pragma once

#include <common/packet.h>
#include <common/types.h>

#define MSPACKETS_NAMESPACE namespace MasterSlavePackets {

YAIC_NAMESPACE MSPACKETS_NAMESPACE

// -------------------------
// Master -> Slave
// -------------------------

class MasterHeartbeat : public Packet {
public:
    MasterHeartbeat();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class AuthResponseServer {
public:
    AuthResponseServer();
    AuthResponseServer(u32 id, const String &addr, u16 port);

    u32 id;
    String address;
    u16 port;
};

class AuthResponse : public Packet {
public:
    enum class Status : u32 {
        Ok = 0,
        UnknownError = 1,
        InvalidMode = 2,
        InvalidPlaintextPassword = 3,
    };

    AuthResponse();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    Status status() const;
    u32 id() const;
    Vector<AuthResponseServer> &servers();
    const Vector<AuthResponseServer> &servers() const;

    void setStatus(Status status);
    void setId(u32 id);

    u64 authPassword() const;
    void setAuthPassword(const u64 &authPassword);

protected:
    Status m_status;
    u32 m_id;
    u64 m_authPassword;
    Vector<AuthResponseServer> m_slaves;
};

class SyncEnd : public Packet {
public:
    SyncEnd();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class NewSlave : public Packet {
public:
    NewSlave();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u32 id() const;
    const String &address() const;
    u16 port() const;

    void setId(u32 id);
    void setAddress(const String &address);
    void setPort(u16 port);

protected:
    u32 m_id;
    String m_address;
    u16 m_port;
};

class RemoveSlave : public Packet {
public:
    RemoveSlave();
    RemoveSlave(u32 id);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u32 id() const;

protected:
    u32 m_id;
};


// -------------------------
// Slave -> Master
// -------------------------

class SlaveHeartbeat : public Packet {
public:
    SlaveHeartbeat();
    SlaveHeartbeat(u32 connections);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u32 connections() const;

protected:
    u32 m_connections;
};

class Auth : public Packet {
public:
    enum class Mode : u32 {
        None = 0,
        Plaintext = 1
    };

    Auth();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    const String &name() const;
    void setName(const String &name);

    Mode mode() const;
    void setMode(const Mode &mode);

    const String &userAddress() const;
    void setUserAddress(const String &userAddress);

    u16 userPort() const;
    void setUserPort(const u16 &userPort);

    const String &slaveAddress() const;
    void setSlaveAddress(const String &slaveAddress);

    u16 slavePort() const;
    void setSlavePort(const u16 &slavePort);

    const String &plaintextPassword() const;
    void setPlaintextPassword(const String &plaintextPassword);

protected:
    String m_name;
    Mode m_mode;
    String m_userAddress;
    u16 m_userPort;
    String m_slaveAddress;
    u16 m_slavePort;
    String m_plaintextPassword;
};

class SyncStart : public Packet {
public:
    SyncStart();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class NewAck : public Packet {
public:
    NewAck();
    NewAck(u32 id);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u32 id() const;

protected:
    u32 m_id;
};

END_NAMESPACE END_NAMESPACE
