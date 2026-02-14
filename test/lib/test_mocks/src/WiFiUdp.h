#ifndef MOCK_WIFIUDP_H
#define MOCK_WIFIUDP_H

class WiFiUDP {
public:
    int begin(uint16_t port) { (void)port; return 1; }
    void stop() {}
};

#endif // MOCK_WIFIUDP_H
