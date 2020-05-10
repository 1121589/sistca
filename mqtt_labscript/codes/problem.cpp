#include <iostream>
#include "MQTTClient.h"
#include <nlohmann/json.hpp>
#include <csignal>  // Quit Handler

#define ADDRESS             "localhost:1883"
#define CLIENTID            "SistcaStudent"
#define TOPIC_STATERX(...)  "sistca/machines/" #__VA_ARGS__ "/state"
#define TOPIC_RAWMATS       "sistca/factory/current/rawmats/alldata"
#define TOPIC_FACSENS       "sistca/factory/current/sensors/alldata"
#define TOPIC_MACDATA       "sistca/machines/alldata"
#define TIMEOUT             10000L
#define SEND_INTERVAL       10      // Interval between message publications
#define QOS                 1

volatile MQTTClient_deliveryToken deliveredtoken;

using namespace std;
using json = nlohmann::json;
bool quit = false;

void conHandler(void *context, char *cause){};
void delHandler(void *context, MQTTClient_deliveryToken dt) {
    cout << "Message with token value " << dt << " delivery confirmed" << endl;
    deliveredtoken = dt;
}
int msgHandler(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    time_t now = time(0);

    cout << "\n" << (char*) ctime(&now) << "Message arrived" << endl;
    cout << "topic name: " <<  topicName << endl;
    cout << "message length: " << message->payloadlen << endl;
    cout << "message payload: " << (char*) message->payload << endl;

    if(topicLen == 0) {
        int machineId = 0;
        int* mcStArr = (int*)context;
        if(!strcmp(TOPIC_STATERX(1) , topicName)) machineId = 1;
        else if(!strcmp(TOPIC_STATERX(2) , topicName)) machineId = 2;

        if(machineId) {
            if(!strcmp((char*)message->payload, "Off")) mcStArr[machineId-1] = 0;
            if(!strcmp((char*)message->payload, "On")) mcStArr[machineId-1] = 1;
        }
        cout << "States: Machine[1]-> " << mcStArr[0] << " Machine[2]-> " << mcStArr[1] << endl;
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void getRawMatsData(char* txt, int *txtLen){

    srand(time(0));

    // create an object
    json o;
    o["rawmatsdata"]["water"]  = rand() % (int)1E3;
    o["rawmatsdata"]["cotton"] = rand() % (int)1E3;
    o["rawmatsdata"]["latex"]  = rand() % (int)1E3;
    o["rawmatsdata"]["wood"]   = rand() % (int)1E3;

    *txtLen = o.dump().length();
    strcpy(txt, o.dump().c_str()); // Serialize message
}

void getSensorsData(char* txt, int *txtLen){

    srand(time(0));

    // create an object
    json o;
    o["factorydata"]["temperature"] = rand() % 50;
    o["factorydata"]["humidity"]    = rand() % (int)1E2;
    o["factorydata"]["pressure"]    = rand() % (int)1E2 + 950;

    *txtLen = o.dump().length();
    strcpy(txt, o.dump().c_str()); // Serialize message
}

void getMachinesData(int mcId, char* txtPayload, int *txtLen){

    time_t rtime = time(&rtime) + mcId;
    srand(rtime);

    // create an object
    json o;

    o["machinesdata"]["id"]    = mcId;
    o["machinesdata"]["state"] = "On";
    o["machinesdata"]["onlinehours"]      = (rand() % (int)1E4);
    o["machinesdata"]["maintenancehours"] = rand() % (int)1E4;
    o["machinesdata"]["hourproduction"]   = rand() % (int)1E3;
    o["machinesdata"]["percdefects"]      = rand() % (int)1E2;

    *txtLen = o.dump().length();
    strcpy(txtPayload, o.dump().c_str()); // Serialize message
}

void quitHandler(int s){ cout << "\nQuitting" << endl; quit = true; }

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_createOptions createOpts = MQTTClient_createOptions_initializer;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer5;
    MQTTProperties props = MQTTProperties_initializer;
    MQTTProperties willProps = MQTTProperties_initializer;
    MQTTSubscribe_options sopts = { {'M', 'Q', 'S', 'O'}, 0, 1, 0, 0 };
    MQTTResponse response = MQTTResponse_initializer;
    MQTTClient_message msgToSend = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    // Array to store machines state
    int  machineSt[2] = {0, 0};
    char txTopic[64];
    char txMsg[256];
    int  txLen;
    bool txSendFlag = false;

    // Variables to non-sleep time control
    time_t time1;
    time(&time1);
    time_t time0 = time1 - 8;

    signal(SIGINT, quitHandler); // Catch SIGINT signal (CTRL+C)

    int rc=0;

    createOpts.MQTTVersion = MQTTVERSION_5;
    rc = MQTTClient_createWithOptions(&client, ADDRESS, CLIENTID,
         MQTTCLIENT_PERSISTENCE_NONE, NULL, &createOpts);
    if (rc != MQTTCLIENT_SUCCESS) {
         cerr << "Failed to create client, reason code " << rc << endl;
         exit(EXIT_FAILURE);
    }

    rc = MQTTClient_setCallbacks(client, machineSt, conHandler, msgHandler, delHandler);
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

    cout << "\nSubscribing to topic " << TOPIC_STATERX(+)<< endl;
    cout << "for client " << CLIENTID << " using QoS " << QOS << endl;
    cout << "Press CTRL+C to quit" << endl;
    response = MQTTClient_subscribe5(client, TOPIC_STATERX(+), QOS, &sopts, &props);
    if( response.reasonCode != MQTTCLIENT_SUCCESS && response.reasonCode != QOS) {
        cerr << "Failed to subscribe, return code: " << response.reasonCode
        << " - " <<  MQTTReasonCode_toString(response.reasonCode) << endl;
    	exit(EXIT_FAILURE);
    }
    else {
        MQTTResponse_free(response);
    	while(!quit)
        {
            time(&time1);
            if(difftime(time1, time0) >= SEND_INTERVAL) {

                time0 = time1;

                cout << "\n" << (char*) ctime(&time0) << "Sending New Report\n" << endl;

                for(int i=0; i<4; i++){
                    // Clean txt buffers
                    memset(txTopic, '\0', sizeof(txTopic));
                    memset(txMsg, '\0', sizeof(txMsg));

                    switch (i)
                    {
                    case 0:
                        getSensorsData(txMsg, &txLen);
                        strcpy(txTopic, TOPIC_FACSENS);
                        txSendFlag = true;
                        break;
                     case 1:
                        getRawMatsData(txMsg, &txLen);
                        strcpy(txTopic, TOPIC_RAWMATS);
                        txSendFlag = true;
                        break;
                     case 2:
                        if(!machineSt[0]) continue;
                        getMachinesData(1, txMsg, &txLen);
                        strcpy(txTopic, TOPIC_MACDATA);
                        txSendFlag = true;
                        break;
                     case 3:
                        if(!machineSt[1]) continue;
                        getMachinesData(2, txMsg, &txLen);
                        strcpy(txTopic, TOPIC_MACDATA);
                        txSendFlag = true;
                        break;
                    default:
                        break;
                    }

                    if(txSendFlag) {
                        txSendFlag = false;

                        // Insert message details
                        msgToSend.payload = txMsg;
                        msgToSend.payloadlen = txLen;
                        msgToSend.qos = QOS;
                        msgToSend.retained = 0;
                        deliveredtoken = 0;
                        response = MQTTClient_publishMessage5(client, txTopic, &msgToSend, &token);
                        if(response.reasonCode != MQTTCLIENT_SUCCESS) {
                            cerr << "Failed to publish message, return code: " << response.reasonCode
                            << " - " <<  MQTTReasonCode_toString(response.reasonCode) << endl;
                            exit(EXIT_FAILURE);
                        }

                        cout << "Waiting for the publication of\n" << txMsg << "\non topic:"
                            << txTopic << "\nfor client with ClientID: " << CLIENTID << endl;
                        while(deliveredtoken != token);
                        cout << "Message delivered\n" << endl;
                    }
                }

            }
        }

        response = MQTTClient_unsubscribe5(client, TOPIC_STATERX(+), &props);
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