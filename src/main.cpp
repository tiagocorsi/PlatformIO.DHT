/* 
 *  Programa baseado no programa original desenvolvido por Timothy Woo 
 *  Tutorial do projeto original; https://www.hackster.io/botletics/esp32-ble-android-arduino-ide-awesome-81c67d
 *  Modificado para ler dados do sensor DHT11 - Bluetooth Low Energy com ESP32
 
 ESP32 and OLED display example
 http://www.esp32learning.com/code/esp32-and-oled-display-example.php
 */ 

//PlatformIO
#include <Arduino.h>

//BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//DHT11
#include <DHT.h>
#include <DHT_U.h>

//OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
bool deviceConnected = false;
const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
float humidity;
float temperature;

//Definição do OLED
//SDA - 21 | SCL - 22
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Definição do DHT11
#define DHTPIN 27 // pino de dados do DHT11
#define DHTTYPE DHT11 // define o tipo de sensor, no caso DHT11
DHT dht(DHTPIN, DHTTYPE);

// Veja o link seguinte se quiser gerar seus próprios UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define DHTDATA_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" 
 
BLECharacteristic *pCharacteristic;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
 
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};
 
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      Serial.println(rxValue[0]);
 
      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
 
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
        Serial.println("*********");
      }
 
      // Processa o caracter recebido do aplicativo. Se for A1 acende o LED. A0 apaga o LED
      if (rxValue.find("A1") != -1) { 
        Serial.println("Turning ON!");
        digitalWrite(LED, HIGH);
      }
      else if (rxValue.find("A0") != -1) {
        Serial.println("Turning OFF!");
        digitalWrite(LED, LOW);
      }
    }
};
 
void setup() {
  Serial.begin(115200);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  
  pinMode(LED, OUTPUT);
 
  // Create the BLE Device
  BLEDevice::init("ESP32 DHT11"); // Give it a name
 
  // Configura o dispositivo como Servidor BLE
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
 
  // Cria o servico UART
  BLEService *pService = pServer->createService(SERVICE_UUID);
 
  // Cria uma Característica BLE para envio dos dados
  pCharacteristic = pService->createCharacteristic(
                      DHTDATA_CHAR_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                       
  pCharacteristic->addDescriptor(new BLE2902());
 
  // cria uma característica BLE para recebimento dos dados
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
 
  pCharacteristic->setCallbacks(new MyCallbacks());
 
  // Inicia o serviço
  pService->start();
 
  // Inicia a descoberta do ESP32
  pServer->getAdvertising()->start();

  dht.begin();
  Serial.println("Esperando um cliente se conectar...");
}
 
void loop() {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    // testa se retorno é valido, caso contrário algo está errado.
    if (isnan(temperature) || isnan(humidity)) 
    {
      Serial.println("Failed to read from DHT");
      return;
    }
    
    display.clearDisplay();
    
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("SENSOR DHT11!");
    
    //display.setTextColor(BLACK, WHITE); // 'inverted' text
    //display.println(3.141592);
    
    display.setTextSize(2);
    display.setTextColor(WHITE);
    //display.print("0x"); 
    //display.println(0xDEADBEEF, HEX);
    display.println(humidity);

    display.setCursor(65,16);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    //display.print("0x"); 
    //display.println(0xDEADBEEF, HEX);
    display.println(temperature);
    
    display.display();
    display.clearDisplay();
    
  if (deviceConnected) {
    char humidityString[2];
    char temperatureString[2];
    dtostrf(humidity, 1, 2, humidityString);
    dtostrf(temperature, 1, 2, temperatureString);
    
    char dhtDataString[16];
    sprintf(dhtDataString, "%.2f,%.2f", humidity, temperature);
    pCharacteristic->setValue(dhtDataString);

    // Envia o valor para o aplicativo! 
    pCharacteristic->notify(); 
    Serial.print("*** Dado enviado: ");
    Serial.print(dhtDataString);
    Serial.println(" ***"); 
  }
  delay(1000);
}
