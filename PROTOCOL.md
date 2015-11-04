# PROTOKÓŁ

[TOC]

## Podstawowe informacje

Cała komunikacja opiera się na pakietach przesyłanych na socketach TCP/IP. Wszystkie pakiety mają następującą strukture:

    struct Packet {
        u16 id;                     // Typ pakietu
        u16 payloadSize;            // Rozmiar ładunku
        char payload[payloadSize];  // Ładunek zależny od typu pakietu
    }

Wszystkie dane liczbowe są w formacie big endian. Wszystko wyrównane do 1 bajta (#pragma pack(1)). Typ String jest zdefiniowany następująco:

    struct String {
        u16 size;
        char string[size]; // UTF-8 / ASCII (bez kończącego NUL-a)
    }

## Pakiety

### Serwer główny < - > Klient

#### RequestServers (id: 1)

    struct RequestServers {
        RequestServersFlags flags;  // Flagi
        u8 max;                     // Ile maksymalnie serwerów wysłać
    }
    
    enum class RequestServersFlags {
        IPv4Only = 1 << 0,        // Tylko serwery posiadające adres IPv4
        IPv6Only = 1 << 1,        // IPv6
    }    

#### ServerList (id: 2)

    struct ServerList {
        u8 count;                   // Ilość serwerów
        Server servers[count];      // Serwery
    }
    
    struct Server {
        String address;             // IP lub hostname
        u16 port;                   // Port
    }

