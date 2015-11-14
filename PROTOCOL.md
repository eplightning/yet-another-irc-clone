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
        ClientToSlave = 0, // <1;
        SlaveToClient = 1, // <8192;
        ClientToMaster = 2, // <16384;
        MasterToClient = 3, // <24576;
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
        u8 count;                       // Ilość serwerów
        Vector<Server> servers[count];  // Serwery
    }
    
    struct Server {
        String address;             // IP lub hostname
        u16 port;                   // Port
    }

