#include <iostream>
#include "MQTTClient.h"
#include <nlohmann/json.hpp>

#define ADDRESS     "localhost:1883"
#define CLIENTID    "SistcaStudent"
#define TOPIC       "sistca/machines/alldata"
#define TIMEOUT     10000L
#define QOS         1

using namespace std;
using json = nlohmann::json;

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_createOptions createOpts = MQTTClient_createOptions_initializer;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer5;
    MQTTProperties props = MQTTProperties_initializer;
    MQTTProperties willProps = MQTTProperties_initializer;
    MQTTResponse response = MQTTResponse_initializer;
    MQTTClient_message msgToSend = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    int rc=0;

    createOpts.MQTTVersion = MQTTVERSION_5;
    rc = MQTTClient_createWithOptions(&client, ADDRESS, CLIENTID,
         MQTTCLIENT_PERSISTENCE_NONE, NULL, &createOpts);
    if (rc != MQTTCLIENT_SUCCESS) {
         cerr << "Failed to create client, reason code " << rc << endl;
         exit(EXIT_FAILURE);
    }

    opts.keepAliveInterval = 20;
	opts.cleanstart = 1;
	opts.username = "sistcauser";
	opts.password = "sistcapass";
	opts.MQTTVersion = MQTTVERSION_5;
    
    response = MQTTClient_connect5(client, &opts, &props, &willProps);
    if (response.reasonCode != MQTTCLIENT_SUCCESS){
        cerr << "Failed to connect, reason code: " << response.reasonCode
             << " - " <<  MQTTReasonCode_toString(response.reasonCode) << endl;
        exit(EXIT_FAILURE);
    }
    else cout << "Connect successful, reason code:  " << response.reasonCode
              << " - " << MQTTReasonCode_toString(response.reasonCode) << endl;
    MQTTResponse_free(response);
    
    // create an object
    json o;
    o["machinesdata"]["id"] = 2;
    o["machinesdata"]["state"] = "On";
    o["machinesdata"]["onlinehours"] = 1450;
    o["machinesdata"]["maintenancehours"] = 12;
    o["machinesdata"]["hourproduction"] = 899;
    o["machinesdata"]["percdefects"] = 20;

    // Acquire information about the message
    int txLen = o.dump().length();
    char txMsg[txLen] = {  };
    strcpy(txMsg, o.dump().c_str());

    // Insert message details
    msgToSend.payload = txMsg;
    msgToSend.payloadlen = txLen;
    msgToSend.qos = QOS;
    msgToSend.retained = 0;

    response = MQTTClient_publishMessage5(client, TOPIC, &msgToSend, &token);
    if(response.reasonCode != MQTTCLIENT_SUCCESS) {
        cerr << "Failed to publish message, return code: " << response.reasonCode
        << " - " <<  MQTTReasonCode_toString(response.reasonCode) << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Waiting up to " << (int)(TIMEOUT/1000) << " seconds for publication of\n" << txMsg
         << "\non topic:" << TOPIC << "\nfor client with ClientID: " << CLIENTID << endl;
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    if (rc != MQTTCLIENT_SUCCESS) {
        cerr << "Failed to receive delivery confirmation, return code " << rc << endl;
    }

    rc = MQTTClient_disconnect5(client, TIMEOUT, MQTTREASONCODE_NORMAL_DISCONNECTION, &props);
    if(rc != MQTTCLIENT_SUCCESS) {
        cerr << "Failed to disconnect, reason code: " << rc
             <<  " - " << MQTTReasonCode_toString((MQTTReasonCodes)rc) << endl;
    }
    else cout << "Disconnect successful, reason code: " << rc
              << " - " << MQTTReasonCode_toString((MQTTReasonCodes)rc) << endl;
    
    MQTTClient_destroy(&client);

    return EXIT_SUCCESS;
}