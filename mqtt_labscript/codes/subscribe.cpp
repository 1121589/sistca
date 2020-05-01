#include <iostream>
#include "MQTTClient.h"

#define ADDRESS     "localhost:1883"
#define CLIENTID    "SistcaStudent"
#define TOPIC       "sistca/tutorial/machines/1/production"
#define TIMEOUT     10000L
#define QOS         0

using namespace std;

int msgHandler(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    time_t now = time(0);

    cout << "\n" << (char*) ctime(&now) << "Message arrived" << endl;
    cout << "topic name: " <<  topicName << endl;
    cout << "message length: " << message->payloadlen << endl;
    cout << "message payload: " << (char*) message->payload << endl;

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_createOptions createOpts = MQTTClient_createOptions_initializer;
	MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer5;
	MQTTProperties props = MQTTProperties_initializer;
    MQTTProperties willProps = MQTTProperties_initializer;
    MQTTSubscribe_options sopts = { {'M', 'Q', 'S', 'O'}, 0, 1, 0, 0 };
    MQTTResponse response = MQTTResponse_initializer;

    int rc = 0;

    createOpts.MQTTVersion = MQTTVERSION_5;
    rc = MQTTClient_createWithOptions(&client, ADDRESS, CLIENTID,
         MQTTCLIENT_PERSISTENCE_NONE, NULL, &createOpts);
    if (rc != MQTTCLIENT_SUCCESS) {
         cerr << "Failed to create client, reason code " << rc << endl;
         exit(EXIT_FAILURE);
    }

    rc = MQTTClient_setCallbacks(client, NULL, NULL, msgHandler, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        cerr << "Failed to set callbacks, return code " << rc << endl;
        MQTTClient_destroy(&client);
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

    char msgTopic[256] = TOPIC;
    if(argc>1) sprintf(msgTopic, "%s", argv[1]);
    cout << "\nSubscribing to topic " << msgTopic << endl;
    cout << "for client " << CLIENTID << " using QoS " << QOS << endl;
    cout << "Press Q<Enter> to quit" << endl;

    response = MQTTClient_subscribe5(client, msgTopic, QOS, &sopts, &props);
    if(response.reasonCode != MQTTCLIENT_SUCCESS) {
        cerr << "Failed to subscribe, return code: " << response.reasonCode
        << " - " <<  MQTTReasonCode_toString(response.reasonCode) << endl;
    	exit(EXIT_FAILURE);
    }
    else {
        MQTTResponse_free(response);
    	int ch;
    	do ch = getchar();
        while (ch!='Q' && ch != 'q');

        response = MQTTClient_unsubscribe5(client, msgTopic, &props);
        if(response.reasonCode != MQTTCLIENT_SUCCESS) {
            cerr << "Failed to unsubscribe, return code " << response.reasonCode
            << " - " <<  MQTTReasonCode_toString(response.reasonCode) << endl;
        	exit(EXIT_FAILURE);
        }
        MQTTResponse_free(response);
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