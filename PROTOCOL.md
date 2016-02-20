# PROTOKÓŁ

## Podstawowe informacje

Cała komunikacja opiera się na pakietach przesyłanych na socketach TCP/IP. Wszystkie pakiety mają następującą strukture:

    struct Packet {
        u16 id;                     // Typ pakietu
        u16 payloadSize;            // Rozmiar ładunku
        char payload[payloadSize];  // Ładunek zależny od typu pakietu
    }

Wszystkie typy liczbowe (u8, s8, s16, u16, u32, s32) są w formacie big endian. Wszystko wyrównane do 1 bajta (#pragma pack(1)). Dodatkowe typy jest zdefiniowane następująco:

    struct Vector<T> {
        u32 size;           // Rozmiar tablicy
        T elements[size];   // Elementy tablicy
    }
    
    String = Vector<char>; // UTF-8 / ASCII bez kończącego NUL-a

Trzy najbardziej znaczące bity typu pakietu oznaczają jego kierunek (kto wysyła, kto odbiera).

    enum class Direction : u8 {
        UserToSlave = 0, // <1;
        SlaveToUser = 1, // <8192;
        UserToMaster = 2, // <16384;
        MasterToUser = 3, // <24576;
        SlaveToMaster = 4, // <32768;
        MasterToSlave = 5, // <40960;
        SlaveToSlave = 6, // <49152;
        Unknown = 7
    };

## Pakiety

### Serwer główny < - > Klient

#### RequestServers (id: 16384)

    struct RequestServers {
        RequestServersFlags flags;  // Flagi
        u8 max;                     // Ile maksymalnie serwerów wysłać
    }
    
    enum class RequestServersFlags {
        IPv4Only = 1 << 0,        // Tylko serwery posiadające adres IPv4
        IPv6Only = 1 << 1,        // IPv6
    }    

#### ServerList (id: 24576)

    struct ServerList {
        Vector<Server> servers[count];  // Serwery
    }
    
    struct Server {
        String address;             // IP lub hostname
        u16 port;                   // Port
    }

### Serwer slave < - > Klient

#### UserHeartbeat (id: 1)

    struct UserHeartbeat {
    }

#### Handshake (id: 2)

    struct Handshake {
        String nick; // Wybrany nick
    }

#### ListChannels (id: 3)

    struct ListChannels {
        // Puste
    }

#### JoinChannel (id: 4)

    struct JoinChannel {
        String channel;   // Nazwa kanału
    }

#### PartChannel (id: 5)

    struct PartChannel {
        u64 channel;    // ID kanału
    }

#### SendChannelMessage (id: 6)

    struct SendChannelMessage {
        u64 channel;
        String message;
    }

#### SendPrivateMessage (id: 7)

    struct SendPrivateMessage {
        u64 user;
        String message;
    }

#### SlaveHeartbeat (id: 8192)

    struct SlaveHeartbeat {
    }

#### HandshakeAck (id: 8193)

    struct HandshakeAck {
        HandshakeAckStatus status; // Status
        u64 userid;                // Otrzymany ID
    }
    
    enum class HandshakeAckStatus {
        Ok = 0,
        UnknownError = 1,
        InvalidNick = 2,
        Full = 3
    };

#### Channels (id: 8194)

    struct Channels {
        Vector<String> channels;
    }
  
#### ChannelJoined (id: 8195)

    struct ChannelJoined {
        u64 id;                           // ID kanału
        ChannelJoinedStatus status;       // Status
        String name;                      // Nazwa kanału
        UserFlags flags;                  // Twoje flagi
        Vector<ChanUser> users;           // Użytkownicy w kanale
    }
    
    enum class ChannelJoinedStatus {
        Ok = 0,                       // Ok
        UnknownError = 1              // Każdy inny błąd
    }
    
    struct ChanUser {
        u64 id;                       // ID
        UserFlags flags;              // Flagi
        String nick;                  // Nick użytkownika
    }
    
    enum class UserFlags {
        Operator = 1 << 0             // OP
    }

#### ChannelParted (id: 8196)

    struct ChannelParted {
        u64 id;                       // Id kanału
        ChannelPartedStatus status;   // Status
        ChannelPartedReason reason;   // Powód
    }
    
    enum class ChannelPartedStatus {
        Ok = 0,                     // Ok
        UnknownError = 1            // Każdy inny błąd
    }
    
    enum class ChannelPartedReason {
        Requested = 0,              // Ty chciałeś
        Unknown = 1,                // Każdy inny
        Kicked = 2                  // Wyrzucony
    }

#### ChannelMessage (id: 8197)

    struct ChannelMessage {
        u64 channel;                // ID kanału
        u64 user;                   // ID użytkownika
        String message;             // Wiadomość
    }
    

#### ChannelUserJoined (id: 8198)

    struct ChannelNewUser {
        u64 channel;                // ID kanału
        ChanUser user;              // Patrz wyżej
    }

#### ChannelUserParted (id: 8199)

    struct ChannelUserPart {
        u64 channel;                // ID kanału
        u64 user;                   // ID użytkownika
    }

#### ChannelUserUpdated (id: 8200)

    struct ChannelUserUpdate {
        u64 channel;                // ID kanału
        u64 user;                   // ID użytkownika
        UserFlags flags;            // Flagi
    }
    

#### UserDisconnected (id: 8201)

    struct UserDisconnected {
        u64 userid;                 // ID użytkownika
    }

#### UserUpdated (id: 8202)

    struct UserUpdated {
        u64 userid;
        String nick;
    }

#### PrivateMessageReceived (id: 8203)

    struct PrivateMessageReceived {
        u64 userid;
        String message;
    }
