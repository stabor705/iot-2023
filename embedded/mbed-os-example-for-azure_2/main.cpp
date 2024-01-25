/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ISM43362Interface.h"
#include "InterfaceDigitalIn.h"
#include "PinNames.h"
#include "SocketAddress.h"
#include "TLSSocket.h"
#include "entropy.h"
#include "mbed.h"
#include "nsapi_types.h"
#include "rtos/ThisThread.h"
#include "NTPClient.h"
#include "TMP102/TMP102.h"
#include "certs.h"
#include "iothub.h"
#include "iothub_client_options.h"
#include "iothub_device_client.h"
#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/xlogging.h"
#include "source/lcd.h"
#include "iothubtransportmqtt.h"
#include "azure_cloud_credentials.h"

#include "mbedtls/aes.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include <cstddef>
#include <cstdint>
#include <array>
#include <cstdio>

/**
 * This example sends and receives messages to and from Azure IoT Hub.
 * The API usages are based on Azure SDK's official iothub_convenience_sample.
 */

// Global symbol referenced by the Azure SDK's port for Mbed OS, via "extern"
NetworkInterface *_defaultSystemNetwork;
TouchScreen touchScreen;
static bool message_received = false;

const char *root_ca_cert = R"EOF(-----BEGIN CERTIFICATE-----
MIIF8zCCBNugAwIBAgIQCq+mxcpjxFFB6jvh98dTFzANBgkqhkiG9w0BAQwFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0yMDA3MjkxMjMwMDBaFw0yNDA2MjcyMzU5NTlaMFkxCzAJBgNVBAYTAlVT
MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKjAoBgNVBAMTIU1pY3Jv
c29mdCBBenVyZSBUTFMgSXNzdWluZyBDQSAwMTCCAiIwDQYJKoZIhvcNAQEBBQAD
ggIPADCCAgoCggIBAMedcDrkXufP7pxVm1FHLDNA9IjwHaMoaY8arqqZ4Gff4xyr
RygnavXL7g12MPAx8Q6Dd9hfBzrfWxkF0Br2wIvlvkzW01naNVSkHp+OS3hL3W6n
l/jYvZnVeJXjtsKYcXIf/6WtspcF5awlQ9LZJcjwaH7KoZuK+THpXCMtzD8XNVdm
GW/JI0C/7U/E7evXn9XDio8SYkGSM63aLO5BtLCv092+1d4GGBSQYolRq+7Pd1kR
EkWBPm0ywZ2Vb8GIS5DLrjelEkBnKCyy3B0yQud9dpVsiUeE7F5sY8Me96WVxQcb
OyYdEY/j/9UpDlOG+vA+YgOvBhkKEjiqygVpP8EZoMMijephzg43b5Qi9r5UrvYo
o19oR/8pf4HJNDPF0/FJwFVMW8PmCBLGstin3NE1+NeWTkGt0TzpHjgKyfaDP2tO
4bCk1G7pP2kDFT7SYfc8xbgCkFQ2UCEXsaH/f5YmpLn4YPiNFCeeIida7xnfTvc4
7IxyVccHHq1FzGygOqemrxEETKh8hvDR6eBdrBwmCHVgZrnAqnn93JtGyPLi6+cj
WGVGtMZHwzVvX1HvSFG771sskcEjJxiQNQDQRWHEh3NxvNb7kFlAXnVdRkkvhjpR
GchFhTAzqmwltdWhWDEyCMKC2x/mSZvZtlZGY+g37Y72qHzidwtyW7rBetZJAgMB
AAGjggGtMIIBqTAdBgNVHQ4EFgQUDyBd16FXlduSzyvQx8J3BM5ygHYwHwYDVR0j
BBgwFoAUTiJUIBiV5uNu5g/6+rkS7QYXjzkwDgYDVR0PAQH/BAQDAgGGMB0GA1Ud
JQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjASBgNVHRMBAf8ECDAGAQH/AgEAMHYG
CCsGAQUFBwEBBGowaDAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGlnaWNlcnQu
Y29tMEAGCCsGAQUFBzAChjRodHRwOi8vY2FjZXJ0cy5kaWdpY2VydC5jb20vRGln
aUNlcnRHbG9iYWxSb290RzIuY3J0MHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6Ly9j
cmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RHMi5jcmwwN6A1oDOG
MWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RHMi5j
cmwwHQYDVR0gBBYwFDAIBgZngQwBAgEwCAYGZ4EMAQICMBAGCSsGAQQBgjcVAQQD
AgEAMA0GCSqGSIb3DQEBDAUAA4IBAQAlFvNh7QgXVLAZSsNR2XRmIn9iS8OHFCBA
WxKJoi8YYQafpMTkMqeuzoL3HWb1pYEipsDkhiMnrpfeYZEA7Lz7yqEEtfgHcEBs
K9KcStQGGZRfmWU07hPXHnFz+5gTXqzCE2PBMlRgVUYJiA25mJPXfB00gDvGhtYa
+mENwM9Bq1B9YYLyLjRtUz8cyGsdyTIG/bBM/Q9jcV8JGqMU/UjAdh1pFyTnnHEl
Y59Npi7F87ZqYYJEHJM2LGD+le8VsHjgeWX2CJQko7klXvcizuZvUEDTjHaQcs2J
+kPgfyMIOY1DMJ21NxOJ2xPRC/wAh/hzSBRVtoAnyuxtkZ4VjIOh
-----END CERTIFICATE-----
)EOF";


static void on_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        LogInfo("Connected to IoT Hub");
    } else {
        LogError("Connection failed, reason: %s", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT on_message_received(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    LogInfo("Message received from IoT Hub");

    const unsigned char *data_ptr;
    size_t len;
    if (IoTHubMessage_GetByteArray(message, &data_ptr, &len) != IOTHUB_MESSAGE_OK) {
        LogError("Failed to extract message data, please try again on IoT Hub");
        return IOTHUBMESSAGE_ABANDONED;
    }

    message_received = true;
    LogInfo("Message body: %.*s", len, data_ptr);
    return IOTHUBMESSAGE_ACCEPTED;
}

static void on_message_sent(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
        LogInfo("Message sent successfully");
    } else {
        LogInfo("Failed to send message, error: %s",
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    }
}

void send_temperatures() {
    TMP102 tmp(D14, D15, 0x90);
    bool screenPressed = 0;
    bool trace_on = MBED_CONF_APP_IOTHUB_CLIENT_TRACE;
    tickcounter_ms_t interval = 100;
    IOTHUB_CLIENT_RESULT res;

    LogInfo("Initializing IoT Hub client");
    IoTHub_Init();
    LogInfo("After init");
    IOTHUB_DEVICE_CLIENT_HANDLE client_handle = IoTHubDeviceClient_CreateFromConnectionString(
        azure_cloud::credentials::iothub_connection_string,
        MQTT_Protocol
    );
    if (client_handle == nullptr) {
        LogError("Failed to create IoT Hub client handle");
        goto cleanup;
    }

    // Enable SDK tracing
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_LOG_TRACE, &trace_on);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to enable IoT Hub client tracing, error: %d", res);
        goto cleanup;
    }

    // Enable static CA Certificates defined in the SDK
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_TRUSTED_CERT, certificates);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set trusted certificates, error: %d", res);
        goto cleanup;
    }

    // Process communication every 100ms
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_DO_WORK_FREQUENCY_IN_MS, &interval);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set communication process frequency, error: %d", res);
        goto cleanup;
    }

    // set incoming message callback
    res = IoTHubDeviceClient_SetMessageCallback(client_handle, on_message_received, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set message callback, error: %d", res);
        goto cleanup;
    }

    // Set connection/disconnection callback
    res = IoTHubDeviceClient_SetConnectionStatusCallback(client_handle, on_connection_status, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set connection status callback, error: %d", res);
        goto cleanup;
    }
    // Send ten message to the cloud (one per second)
    // or until we receive a message from the cloud
    touchScreen.updateScreen("Touch to Pair");
    IOTHUB_MESSAGE_HANDLE message_handle;
    char message[250];
    for (int i = 0; !screenPressed; ++i) {
        if (message_received) {
            // If we have received a message from the cloud, don't send more messeges
            break;
        }

        BSP_TS_GetState(&touchScreen.TS_State);
        if(touchScreen.TS_State.touchDetected) {
            screenPressed = 1;
            touchScreen.updateScreen("Pairing");
        }
        

        // deviceId
        // value
        // timestamp 
        // {\"DeviceId\":\"stm32actual\", \"value\":%.2f, \"timestamp\": %d}
        sprintf(message, "{\"DeviceId\":\"stm32actual\", \"value\":%.2f }", tmp.read_12b());
        // sprintf(message, "%d messages left to send, or until we receive a reply", 10 - i);
        LogInfo("Sending: \"%s\"", message);

        message_handle = IoTHubMessage_CreateFromString(message);
        if (message_handle == nullptr) {
            LogError("Failed to create message");
            goto cleanup;
        }

        res = IoTHubDeviceClient_SendEventAsync(client_handle, message_handle, on_message_sent, nullptr);
        IoTHubMessage_Destroy(message_handle); // message already copied into the SDK

        if (res != IOTHUB_CLIENT_OK) {
            LogError("Failed to send message event, error: %d", res);
            goto cleanup;
        }

        ThisThread::sleep_for(10s);

    }

cleanup:
    IoTHubDeviceClient_Destroy(client_handle);
    IoTHub_Deinit();
}

int main() {
    

    touchScreen.init();
    touchScreen.updateScreen("Connecting to net");
    LogInfo("Connecting to the network");

    _defaultSystemNetwork = NetworkInterface::get_default_instance();
    if (_defaultSystemNetwork == nullptr) {
        LogError("No network interface found");
        return -1;
    }

    int ret = _defaultSystemNetwork->connect();
    if (ret != 0) {
        LogError("Connection error: %d", ret);
        return -1;
    }
    LogInfo("Connection success, MAC: %s", _defaultSystemNetwork->get_mac_address());


    LogInfo("Getting time from the NTP server");
    NTPClient ntp(_defaultSystemNetwork);

    ntp.set_server("time.google.com", 123);
    time_t timestamp = ntp.get_timestamp();
    if (timestamp < 0) {
        LogError("Failed to get the current time, error: %ld", timestamp);
        return -1;
    }
    LogInfo("Time: %s", ctime(&timestamp));
    set_time(timestamp);

    send_temperatures();

    nsapi_size_or_error_t result;
    const char *hostname = "iot-project-agh-bcdgl.azurewebsites.net";
    TLSSocket *socket = new TLSSocket();
    printf("Setting CA Certs\n");
    result = socket->set_root_ca_cert(root_ca_cert);
    if (result != 0) {
        printf("Failed to set CA root cert error: %d", result);
    }

    printf("Opening...\n");
    result = socket->open(_defaultSystemNetwork);
    if (result != 0) {
        printf("Failed to open socket error: %d", result);
    }

    printf("Connecting...\n");
    SocketAddress a;
    _defaultSystemNetwork->gethostbyname(hostname, &a);
    printf("%s", a.get_ip_address());
    a.set_port(443);
    result = socket->connect(a);
    if (result != 0) {
        printf("Failed to connect to host %d\n", result);
        return 1;
    }
    printf("Success!\n");

    char sbuffer[] = "POST /api/pair HTTP/1.1\r\n"
                     "Host: iot-project-agh-bcdgl.azurewebsites.net\r\n"
                     "x-functions-key: SqUzcVOp21WG8hsiAl4XF0Qtc4p4OjiI6Sm87HJdcOCHAzFuM5L4yw==\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: 108\r\n"
                     "\r\n"
                     "iIVRL4CQScmXyj/fCHqI04LdCNjDoD6tXLtJsXvvb7T9b8vVfSKef2Iz97DSM54D2090j0KwSQueeR0ajYXn0WDMbxEgWVCDKZpYnItwqpc=";
    int scount = socket->send(sbuffer, sizeof sbuffer);
    if (scount < sizeof sbuffer) {
        printf("Failed to send buffer: %d %d", scount, sizeof sbuffer);
    }
    printf("sent %d [%.*s]\n", scount, strstr(sbuffer, "\r\n") - sbuffer, sbuffer);

    // Recieve a simple http response and print out the response line
    char rbuffer[256];
    int rcount = socket->recv(rbuffer, sizeof rbuffer);
    printf("recv %d [%.*s]\n", rcount, strstr(rbuffer, "\r\n") - rbuffer, rbuffer);
    touchScreen.updateScreen("Pairing success!");

    delete socket;
    return 0;
}
