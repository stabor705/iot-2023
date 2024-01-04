#include "SocketAddress.h"
#include "TCPSocket.h"
#include "WiFiInterface.h"
#include "mbed.h"
#include "wifi-ism43362/ISM43362Interface.h"
#include "TMP102.h"
#include "nsapi_types.h"
#include <cstdio>

#define SSID "809A stachu"
#define PASS "stachuszklo"

void wifi_procedure() {
    ISM43362Interface wifi;

    printf("Connecting to %s...\n", SSID);
    int ret = wifi.connect(SSID, PASS, NSAPI_SECURITY_WPA2);
    if (ret != 0) {
        printf("Connection error: %d\n", ret);
    }

    printf("Success! Connected to %s\n", SSID);
    SocketAddress a;
    wifi.get_ip_address(&a);
    printf("IP Address: %s\n", a.get_ip_address());

    nsapi_error_t response;
    TCPSocket socket;

    response = socket.open(&wifi);
    if (response != 0) {
        printf("Error connecting socket: %d", response);
        socket.close();
    }
}

void bluetooth_procedure() {
    ISM43362Interface wifi;

    auto res = wifi.set_credentials("dupa", "gowno");
    if (res != NSAPI_ERROR_OK) {
        printf("Failure: %d", res);
    }
    while (true) {
    } 
}

// main() runs in its own thread in the OS
int main()
{
    bluetooth_procedure();

    TMP102 tmp(D14, D15, 0x90);

}

