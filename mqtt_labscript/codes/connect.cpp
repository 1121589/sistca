#include <iostream>
#include "MQTTClient.h"

#define ADDRESS     "localhost:1883"
#define CLIENTID    "SistcaStudent"
#define TOPIC       "sistca/tutorial/activity"
#define TIMEOUT     10000L

using namespace std;

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_createOptions createOpts = MQTTClient_createOptions_initializer;
	MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer5;
	MQTTProperties props = MQTTProperties_initializer;
    MQTTClient_willOptions wopts = MQTTClient_willOptions_initializer;
	MQTTProperties willProps = MQTTProperties_initializer;
    MQTTResponse response = MQTTResponse_initializer;

    int rc = 0;

    createOpts.MQTTVersion = MQTTVERSION_5;
    rc = MQTTClient_createWithOptions(&client, ADDRESS, CLIENTID,
         MQTTCLIENT_PERSISTENCE_NONE, NULL, &createOpts);
    if (rc != MQTTCLIENT_SUCCESS) {
         cerr << "Failed to create client, reason code " << rc << endl;
         exit(EXIT_FAILURE);
    }

    opts.keepAliveInterval = 20;
	opts.cleansession = 1;
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
    
    // Disconnect without publishing the Will Message
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