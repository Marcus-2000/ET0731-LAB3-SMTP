#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Arduino.h> 
#include "ESP32_MailClient.h"

#define LED_PIN 2

// Defining the topics for the device to subscribe and publish to
#define aws_iot_sub_topic "fire-alarm/topic"
#define aws_iot_pub_topic "fire-alarm/fire_detected"

/* Change the following configuration options */
const char* ssid = "ssid"; // SSID of WLAN
const char* password = "pass";// password for WLAN access
const char* aws_iot_hostname = ""; // Change Host name
// const char* aws_iot_sub_topic = "topic/hello"; //Change Subscription Topic name here
// const char* aws_iot_pub_topic = "another/topic/echo"; // Change publish topic name here
const char* aws_iot_pub_message = "Test.";
const char* aws_iot_pub_message2 = "There is a fire detected.";
const char* client_id = "MyIoT";

// change the ca certificate
const char* ca_certificate = "-----BEGIN CERTIFICATE-----\n \n-----END CERTIFICATE-----"; // cert
// change the IOT device certificate
const char* iot_certificate = "-----BEGIN CERTIFICATE-----\n \n-----END CERTIFICATE-----\n"; //Own certificate
// change to the private key of aws
const char* iot_privatekey = "-----BEGIN RSA PRIVATE KEY-----\n \n-----END RSA PRIVATE KEY-----\n"; // RSA Private Key

//WiFi or HTTP client for internet connection
HTTPClientESP32Ex http;

//The Email Sending data object contains config and data to send
SMTPData smtpData;

//Callback function to get the Email sending status
void sendCallback(SendStatus info);

#define SSID_HAS_PASSWORD //comment this line if your SSID does not have a password
/*Variables for sensor*/
int SmokeSensor = 34;
int SmokeSensorVal = 0;
int sensorThres = 400; //Smoke sensor threshold
int DataToSent = 0;

// Temperature
int TempSensor = 32;
float TempSensorVal = 0;
float Temp = 0;
int TempThres = 75;

// Buzzer
int buzzerPin = 14;

int j = 0;

/* Global Variables */
WiFiClientSecure client;
PubSubClient mqtt(client);

/* Functions */
void sub_callback(const char* topic, byte* payload, unsigned int length) {
  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  for (int i = 0; i < length; i++)
    Serial.print((char) payload[i]);
  Serial.println();

  if ((char) payload[0] == '1')
    digitalWrite(LED_PIN, HIGH);
  else if ((char) payload[0] == '0')
    digitalWrite(LED_PIN, LOW);

  mqtt.publish(aws_iot_pub_topic, aws_iot_pub_message);
}

void setup() {
  //Initializations
  Serial.begin(115200);
  Serial.print("Attempting WiFi connection on SSID: ");
  Serial.print(ssid);
  pinMode(LED_PIN, OUTPUT);
  ledcSetup(0,1E5,12);
  ledcAttachPin(14,0);
  digitalWrite(LED_PIN, LOW);
  
  // WiFi
  #ifdef SSID_HAS_PASSWORD
  WiFi.begin(ssid, password);
  #else
  WiFi.begin(ssid);
  #endif

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.print("\nWiFi connection succeeded.\n");

  client.setCACert(ca_certificate);
  client.setCertificate(iot_certificate);
  client.setPrivateKey(iot_privatekey);

  // AWS IoT MQTT uses port 8883
  mqtt.setServer(aws_iot_hostname, 8883);
  mqtt.setCallback(sub_callback);

  
}

void loop() {
  // reconnect on disconnect
  while (!mqtt.connected()) {
    Serial.print("Now connecting to AWS IoT: ");
    if (mqtt.connect(client_id)) {
      Serial.println("connected!");

      const char message3[25] = "{\"fire\": 1}";
      const char message4[25] = "{\"fire\": 0}";
      
      while(1) {         // while loop to repeatedly send data to AWS
        //Smoke detector portion
        SmokeSensorVal = analogRead(SmokeSensor);
        
        //LM35 portion
        TempSensorVal = analogRead(TempSensor);
        
        Temp = (330 * TempSensorVal) / 1023;

        Serial.print("Temperature is ");
        Serial.println(Temp);
        Serial.print("Smoke is ");
        Serial.println(SmokeSensorVal);

        
        if ((SmokeSensorVal < sensorThres) || (Temp > TempThres)) {
          // Add buzzer code below
          ledcWriteTone(0,800);
          delay(1000);
          uint8_t octave = 1;
          ledcWriteNote(0,NOTE_C,octave);  
          delay(1000);
          
          mqtt.publish(aws_iot_pub_topic, message3); // Sending message to specific topic
          Serial.println("Sending email...");



          //Set the Email host, port, account and password
          smtpData.setLogin("smtp.gmail.com", 465, "email", "password");

          //Set the sender name and Email
          smtpData.setSender("ESP32", "email");

          //Add recipients, can add more than one recipient
          smtpData.addRecipient("email");  

          //Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
          smtpData.setPriority("High");

          //Set the subject
          smtpData.setSubject("Smart Fire Alarm System");

          //Set the message - normal text or html format
          smtpData.setMessage("<div style=\"color:#ff0000;font-size:20px;\">There is a potential fire detected, please send help!</div>", true);

          smtpData.setSendCallback(sendCallback);

          if (!MailClient.sendMail(http, smtpData))
            Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
          //Clear all data from Email object to free memory
          smtpData.empty();
        } else {
            mqtt.publish(aws_iot_pub_topic, message4); // Sending message to specific topic
        }
          delay(20000); // 30 Seconds Delay
        } 
        } else {
      Serial.print("failed with status code ");
      Serial.print(mqtt.state());
      Serial.println(" trying again in 5 seconds...");
      delay(5000);
    }
  }

  mqtt.loop();
}

//Callback function to get the Email sending status

void sendCallback(SendStatus msg)

{

  //Print the current status

  Serial.println(msg.info());



  //Do something when complete

  if (msg.success())

  {

    Serial.println("----------------");

  }

}
