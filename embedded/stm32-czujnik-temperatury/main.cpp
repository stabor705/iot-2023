#include "SocketAddress.h"
// #include "TCPSocket.h"
#include "TLSSocket.h"
#include "WiFiInterface.h"
#include "mbed.h"
#include "wifi-ism43362/ISM43362Interface.h"
#include "TMP102.h"
#include "nsapi_types.h"
#include <cstdio>
#include <cstring>
#include "azure_cloud/azure_cloud_credentials.h"
#include "https_request.h"

#define SSID "809A stachu"
#define PASS "stachuszklo"

int prepare_socket(NetworkInterface *net, TLSSocket *socket) {
    int result;
    result = socket->set_root_ca_cert(azure_cloud::credentials::certificate_pem_crt);
    if (result != NSAPI_ERROR_OK) {
        printf("TLSSocket.set_root_ca_cert(...) returned %d\n", result);
        return -1;
    }
    result = socket->open(net);
    if (result != NSAPI_ERROR_OK) {
        printf("TLSSocket.open(...) returned %d\n", result);
        return -1;
    }
    result = socket->connect("iot-project-agh-bcdgl.azurewebsites.net");
    if (result != NSAPI_ERROR_OK) {
        printf("TLSSocket.connect(...) returned %d\n", result);
        return -1;
    }
    return 0;
}

int TLS_send_message(TLSSocket *socket, const char *message) {
    int result;
    result = socket->send(message, strlen(message));
    if (result != NSAPI_ERROR_OK) {
        printf("TLSSocket.send(...) returned %d\n", result);
        return -1;
    }
    return 0;
}

void dump_response(HttpResponse* res) {
    printf("Status: %d - %s\n", res->get_status_code(), res->get_status_message().c_str());

    printf("Headers:\n");
    for (size_t ix = 0; ix < res->get_headers_length(); ix++) {
        printf("\t%s: %s\n", res->get_headers_fields()[ix]->c_str(), res->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%d bytes):\n\n%s\n", res->get_body_length(), res->get_body_as_string().c_str());
}

int sendGetRequest(TLSSocket *socket){
    HttpsRequest* get_req = new HttpsRequest(socket, HTTP_GET, "https://iot-project-agh-bcdgl.azurewebsites.net");
    get_req->set_header("Content-Type", "application/json");
    HttpResponse *get_response = get_req->send();
    if(!get_response){
        printf("HTTPS GET REQUEST FAILED err: %d", get_req->get_error());
        return -1;
    }
    dump_response(get_response);
    delete get_response;
    return 0;
}

int sendPostRequest(TLSSocket *socket, const char *body){
    HttpsRequest* post_req = new HttpsRequest(socket, HTTP_POST, "https://iot-project-agh-bcdgl.azurewebsites.net");
    post_req->set_header("Content-Type", "application/json");
    HttpResponse *post_response = post_req->send(body, strlen(body));
    if(!post_response){
        printf("HTTPS GET REQUEST FAILED err: %d", post_req->get_error());
        return -1;
    }
    dump_response(post_response);
    delete post_response;
    return 0;
}

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

// main() runs in its own thread in the OS
int main()
{
    ISM43362Interface wifi;
    int result = wifi.startOpenAP();
    printf("OpenAP result = %d", result);

    TMP102 tmp(D14, D15, 0x90);

}

