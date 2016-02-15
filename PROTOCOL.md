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

#### Handshake (id: 1)

    struct Handshake {
        String nick; // Wybrany nick
    }

#### ListChannels (id: 2)

    struct ListChannels {
        // Puste
    }

#### JoinChannel (id: 3)

    struct JoinChannel {
        u64 channel;   // Id kanału
    }

#### PartChannel (id: 4)

    struct PartChannel {
        u64 channel;    // ID kanału
    }

#### SendChannelMessage (id: 5)

    struct SendChannelMessage {
        u64 channel;
        String message;
    }

#### HandshakeAck (id: 8192)

    struct HandshakeAck {
        HandshakeAckStatus status; // Status
        u64 userid;                // Otrzymany ID
    }
    
    enum class HandshakeAckStatus {
        0 = Ok,                     // Wszystko OK
        1 = InvalidNick,            // Nieprawidłowy nick (np. zajęty)
        2 = UnknownError            // Każdy inny błąd
    }

#### Channels (id: 8193)

    struct Channels {
        Vector<Channel> channels;
    }
    
    struct Channel {
        u64 id;
        String name;
    }
  
#### ChannelJoined (id: 8194)

    struct ChannelJoined {
        ChannelJoinedStatus status;       // Status
        u64 id;                           // ID kanału
        String name;                      // Nazwa kanału
        UserFlags flags;                  // Twoje flagi
        Vector<User> users;               // Użytkownicy w kanale
    }
    
    enum class ChannelJoinedStatus {
        Ok = 0,                       // Ok
        UnknownError = 1              // Każdy inny błąd
    }
    
    struct User {
        u64 id;                       // ID
        UserFlags flags;              // Flagi
        String nick;                  // Nick użytkownika
    }
    
    enum class UserFlags {
        Operator = 1 << 0             // OP
    }

#### ChannelPart (id: 8195)

    struct ChannelPart {
        ChannelPartStatus status;   // Status
        u64 id;                     // Id kanału
    }
    
    enum class ChannelPartStatus {
        Ok = 0,                     // Ok
        UnknownError = 1            // Każdy inny błąd
    }

#### ChannelUserEntered (id: 8196)

    struct ChannelNewUser {
        u64 channel;                // ID kanału
        User user;                  // Patrz wyżej
    }

#### ChannelUserPart (id: 8197)

    struct ChannelUserPart {
        u64 channel;                // ID kanału
        u64 user;                   // ID użytkownika
    }

#### ChannelMessage (id: 8198)

    struct ChannelMessage {
        u64 channel;                // ID kanału
        u64 user;                   // ID użytkownika
        String message;             // Wiadomość
    }
    

#### ChannelUserUpdated (id: 8199)

    struct ChannelUserUpdate {
        u64 channel;                // ID kanału
        u64 user;                   // ID użytkownika
        UserFlags flags;            // Flagi
    }