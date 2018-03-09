/* WiFi Example
 * Copyright (c) 2018 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "TCPSocket.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

#define WIFI_IDW0XX1    2

#include "SpwfSAInterface.h"

SpwfSAInterface wifi(MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX);

#define logMessage printf

const char *sec2str(nsapi_security_t sec)
{
    switch (sec) {
        case NSAPI_SECURITY_NONE:
            return "None";
        case NSAPI_SECURITY_WEP:
            return "WEP";
        case NSAPI_SECURITY_WPA:
            return "WPA";
        case NSAPI_SECURITY_WPA2:
            return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2:
            return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default:
            return "Unknown";
    }
}

int scan_demo(WiFiInterface *wifi)
{
    WiFiAccessPoint *ap;

    logMessage("Scan:\n");
    
    int count = wifi->scan(NULL,0);
    logMessage("%d networks available.\n", count);

    /* Limit number of network arbitrary to 15 */
    count = count < 15 ? count : 15;

    ap = new WiFiAccessPoint[count];
    count = wifi->scan(ap, count);
    for (int i = 0; i < count; i++)
    {
        logMessage("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\n", ap[i].get_ssid(),
               sec2str(ap[i].get_security()), ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
               ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5], ap[i].get_rssi(), ap[i].get_channel());
    }

    delete[] ap;
    return count;
}

void http_demo(NetworkInterface *net)
{
    TCPSocket socket;
    nsapi_error_t response;

    logMessage("Sending HTTP request to www.google.com...\n");

    // Open a socket on the network interface, and create a TCP connection to www.arm.com
    socket.open(net);
    response = socket.connect("www.google.com", 80);
    if(0 != response) {
        logMessage("Error connecting: %d\n", response);
        socket.close();
        return;
    }

    // Send a simple http request
    char sbuffer[] = "GET / HTTP/1.1\r\nHost: www.arm.com\r\n\r\n";
    nsapi_size_t size = strlen(sbuffer);
    response = 0;
    while(size)
    {
        response = socket.send(sbuffer+response, size);
        if (response < 0) {
            logMessage("Error sending data: %d\n", response);
            socket.close();
            return;
        } else {
            size -= response;
            // Check if entire message was sent or not
            logMessage("sent %d [%.*s]\n", response, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);
        }
    }

    // Recieve a simple http response and print out the response line
    char rbuffer[64];
    response = socket.recv(rbuffer, sizeof rbuffer);
    if (response < 0) {
        logMessage("Error receiving data: %d\n", response);
    } else {
        logMessage("recv %d [%.*s]\n", response, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);
    }

    // Close the socket to return its memory and bring down the network interface
    socket.close();
}

int arrivedcount = 0;


void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    logMessage("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    logMessage("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

int main()
{
    int count = 0;
    
    float version = 0.6;
    char* topic = "test";

//    logMessage("IDW04A1 WIFI MQTT Test Program...\n\n");

    //count = scan_demo(&wifi);
//    if (count == 0) {
//        logMessage("No WIFI APNs found - can't continue further.\n");
//        return -1;
//    }

    logMessage("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA2);
    if (ret != 0) {
        logMessage("\nConnection error\n");
        return -1;
    }

    logMessage("Success\n\n");
    logMessage("MAC: %s\n", wifi.get_mac_address());
    logMessage("IP: %s\n", wifi.get_ip_address());
    logMessage("Netmask: %s\n", wifi.get_netmask());
    logMessage("Gateway: %s\n", wifi.get_gateway());
    logMessage("RSSI: %d\n\n", wifi.get_rssi());

    //http_demo(&wifi);
    
    MQTTNetwork mqttNetwork(&wifi);

    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    const char* hostname = "10.0.30.60";
    int port = 1883;
    logMessage("Connecting to %s:%d\r\n", hostname, port);
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        logMessage("rc from TCP connect is %d\r\n", rc);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "mbed-sample";
    data.username.cstring = "";
    data.password.cstring = "";
    
    if ((rc = client.connect(data)) != 0)
        logMessage("rc from MQTT connect is %d\r\n", rc);

   // if ((rc = client.subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
//        logMessage("rc from MQTT subscribe is %d\r\n", rc);

    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    
 while(count < 10){
    
    // QoS 0
    char buf[100];
    sprintf(buf, "Hello World!  QoS 0 message %d\r\n", count);
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);
     count++;
    //Wait for 5 seconds.
     wait(5);
   }
    
    if ((rc = client.disconnect()) != 0)
        logMessage("rc from disconnect was %d\r\n", rc);

    mqttNetwork.disconnect();
     
    logMessage("Version %.2f: finish %d msgs\r\n", version, count);
     
    wifi.disconnect();

    logMessage("\nDone\n");
}