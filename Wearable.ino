/*
  Context Project BLE Login Device
*/

#include <RFduinoBLE.h>

bool shouldSend = false;
String inputString = "";

void setup() 
{
  // Setup serial
  Serial.begin(9600);

  // Broadcast Bluetooth
  RFduinoBLE.advertisementData = "EA8F2A44";
  RFduinoBLE.deviceName = "Context";
  RFduinoBLE.begin();
}

void loop() 
{
  RFduino_ULPDelay(SECONDS(1));
  if (shouldSend == true) {
    sendMessage("9AD6368489A9A856D0E454641521DA3F56F5F9E9CAEF7AF60E84ABD1F1901F059AD6368489A9A856D0E454641521DA3F56F5F9E9CAEF7AF60E84ABD1F1901F059AD6368489A9A856D0E454641521DA3F56F5F9E9CAEF7AF60E84ABD1F1901F059AD6368489A9A856D0E454641521DA3F56F5F9E9CAEF7AF60E84ABD1F1901F05");
  }
}

void receivedMessage(String message)
{ 
  Serial.println(message);
  String testCase = "9AD6368489A9A856D0E454641521DA3F56F5F9E9CAEF7AF60E84ABD1F1901F059AD6368489A9A856D0E454641521DA3F56F5F9E9CAEF7AF60E84ABD1F1901F059AD6368489A9A856D0E454641521DA3F56F5F9E9CAEF7AF60E84ABD1F1901F059AD6368489A9A856D0E454641521DA3F56F5F9E9CAEF7AF60E84ABD1F1901F05";
  if(testCase == message) {
    Serial.println("Test case passed");
  } 
}

void RFduinoBLE_onReceive(char *data, int len) 
{    
  // Start of message
  if (data[0] == '1') {
    inputString = "";
    for (int i = 1; i < len; i++) {
      inputString += data[i];
    }
  } 
 
  // Data
  if (data[0] == '2') {
    for (int i = 1; i < len; i++) {
      inputString += data[i];
    }
  }  
  
  // EOM
  if (data[0] == '3') {
    if (inputString != "") {
      receivedMessage(inputString);
    }  
  }
}

void RFduinoBLE_onConnect() 
{    
  shouldSend = true;
}

void sendMessage(String messageToSend) {
  // The MTU for RFDuino is 20 bytes
  int stringLength = messageToSend.length(); 
  
  String currentSubMessage = "1";
  int msgSize = 19;
  if (stringLength < 19) msgSize = stringLength;

  currentSubMessage += messageToSend.substring(0, msgSize+1);
  
  char currentSubMessageArray[currentSubMessage.length()+1];
  currentSubMessage.toCharArray(currentSubMessageArray, currentSubMessage.length()+1);
  
  Serial.println(currentSubMessageArray);
  while(!RFduinoBLE.send(currentSubMessageArray, msgSize+1));
  
  int messagesToSend = ceil((float)stringLength / (float)19);
  
  for (int i = 1; i < messagesToSend; i++) {
    currentSubMessage = "2";
    
    if ((i*19+19) > stringLength) {
     // Serial.println("%d is longer than %d", (i*19+19), stringLength);
      msgSize = stringLength - (i*19);
     // Serial.println("Using a value of %d for msgSize", msgSize);
    } 
    
    currentSubMessage += messageToSend.substring(i*19, (i*19)+msgSize+1);
    
    char currentSubMessageArray[currentSubMessage.length()+1];
    currentSubMessage.toCharArray(currentSubMessageArray, currentSubMessage.length()+1);
    Serial.println(currentSubMessageArray);
    while(!RFduinoBLE.send(currentSubMessageArray, msgSize+1));
  }
  
  while(!RFduinoBLE.send('3'));
  
  shouldSend = false;
 
}

