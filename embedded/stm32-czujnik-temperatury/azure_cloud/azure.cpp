/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "azure.h"

/**
 * This example sends and receives messages to and from Azure IoT Hub.
 * The API usages are based on Azure SDK's official iothub_convenience_sample.
 */

// Global symbol referenced by the Azure SDK's port for Mbed OS, via "extern"
static bool message_received = false;


void AZURE::on_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        LogInfo("Connected to IoT Hub");
    } else {
        LogError("Connection failed, reason: %s", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
    }
}

IOTHUBMESSAGE_DISPOSITION_RESULT AZURE::on_message_received(IOTHUB_MESSAGE_HANDLE message, void* user_context)
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

void AZURE::on_message_sent(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
        LogInfo("Message sent successfully");
    } else {
        LogInfo("Failed to send message, error: %s",
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    }
}
AZURE::AZURE(NetworkInterface *iface) {
    _defaultSystemNetwork = iface;
    if(this->init()) {
        LogInfo("Azure Module initialized");
    } else {
        LogError("Azure Module failed to initialize");
    }
}

bool AZURE::init() {
    LogInfo("Azure Module checking connection to the network");
    if (_defaultSystemNetwork == nullptr) {
        LogError("No network interface found");
        return false;
    }
    LogInfo("Connection Info, MAC: %s", _defaultSystemNetwork->get_mac_address());

    LogInfo("Getting time from the NTP server");
    NTPClient ntp(_defaultSystemNetwork);
    ntp.set_server("time.google.com", 123);
    time_t timestamp = ntp.get_timestamp();
    if (timestamp < 0) {
        LogError("Failed to get the current time, error: %ld", timestamp);
        return false;
    }

    LogInfo("Time: %s", ctime(&timestamp));
    set_time(timestamp);
    return true;
}


bool AZURE::connect() {
    LogInfo("Initializing IoT Hub client");
    IoTHub_Init();
    int interval = 100;

    IOTHUB_DEVICE_CLIENT_HANDLE client_handle = IoTHubDeviceClient_CreateFromConnectionString(
        azure_cloud::credentials::iothub_connection_string,
        MQTT_Protocol
    );
    if (client_handle == nullptr) {
        LogError("Failed to create IoT Hub client handle");
        return false
    }

    // Enable SDK tracing
    // bool trace_on = false;
    // IOTHUB_CLIENT_RESULT res = IoTHubDeviceClient_SetOption(client_handle, OPTION_LOG_TRACE, &trace_on);
    // if (res != IOTHUB_CLIENT_OK) {
    //     LogError("Failed to enable IoT Hub client tracing, error: %d", res);
    //     return -1;
    // }

    // Enable static CA Certificates defined in the SDK
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_TRUSTED_CERT, certificates);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set trusted certificates, error: %d", res);
        return false;
    }

    // Process communication every 100ms
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_DO_WORK_FREQUENCY_IN_MS, &interval);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set communication process frequency, error: %d", res);
        return false;
    }

    // set incoming message callback
    res = IoTHubDeviceClient_SetMessageCallback(client_handle, on_message_received, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set message callback, error: %d", res);
        return false
    }

    // Set connection/disconnection callback
    res = IoTHubDeviceClient_SetConnectionStatusCallback(client_handle, on_connection_status, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set connection status callback, error: %d", res);
        return false;
    }
    return true;
}

bool AZURE::send_message(const char *message, IOTHUB_DEVICE_CLIENT_HANDLE client_handle) {
    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(message);
    if (message_handle == nullptr) {
        LogError("Failed to create message");
        return false;
    }

    IOTHUB_CLIENT_RESULT res = IoTHubDeviceClient_SendEventAsync(client_handle, message_handle, on_message_sent, nullptr);
    IoTHubMessage_Destroy(message_handle); // message already copied into the SDK

    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to send message event, error: %d", res);
        return false;
    }
    return true;
}


bool AZURE::receive_message() {
    while (!message_received) {
        // Continue to receive messages in the communication thread
        // which is internally created and maintained by the Azure SDK.
        sleep();
    }
    return true;
}


void AZURE::disconnect() {
    IoTHubDeviceClient_Destroy(client_handle);
    IoTHub_Deinit();
}



// cleanup:
//     IoTHubDeviceClient_Destroy(client_handle);
//     IoTHub_Deinit();
// }

