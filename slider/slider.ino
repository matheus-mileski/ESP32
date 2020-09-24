/*
 *Projeto utilizando ESP32 para enviar e receber comandos via BLE (Bluetooth Low-Energy).
 *Foi desenvolvido um aplicativo no App Inventor para utilizar as funções desenvolvidas.
 *Download do apk: https://bit.ly/33ZIdAd
 *Os valores são enviados por um Potenciômetro conectado a uma porta ADC1 do ESP32.
 *Os valores recebidos são convertidos de string para int para controlarem um LED RGB via PWM.
 *
 *Desenvolvido por Matheus Mileski.
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//Cria um ponteiro para as Características do BLE
//E uma variável para booleana para determinar se há algum dispositivo conectado
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

//Indica quais pinos estão sendo utilizados para o LED e para o Potenciômetro
//Obs: O Potenciômetro precisa estar conectado a um pino ADC_1 para funcionar junto com o BLE, os leds podem estar conectados ao ADC_1 ou ADC_2.
int ledR = 32;
int ledG = 33;
int ledB = 27;
int analogPin = 35;

//Valor que receberá os valores do Potenciômetro
int txValue = 0;

//Vetor com os canais utilizados pelo LED
int ledArray[3] = {1, 2, 3};

//Variáveis que receberão os valores de PWM
int R, G, B;

//Para o BLE funcionar é necessário definir uma UUID para cada serviço.
//Os valores foram gerados utilizando o https://www.uuidgenerator.net/
#define SERVICE_UUID            "067d1cb6-f6b2-11ea-adc1-0242ac120002"
#define CHARACTERISTIC_UUID_TX  "067d1cb9-f6b2-11ea-adc1-0242ac120002"
#define CHARACTERISTIC_UUID_RX  "067d1cb3-f6b2-11ea-adc1-0242ac120002"

//Classe que retornará se o dispositivo foi conectado ou desconectado
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect (BLEServer* pServer) {

      //Quando houver uma conexão bem-sucedida o LED acenderá por 1000ms e apagará
      Serial.println("Dispositivo Conectado");
      ledcWrite(1, 255);
      ledcWrite(2, 255);
      ledcWrite(3, 255);
      delay(1000);
      ledcWrite(1, 0);
      ledcWrite(2, 0);
      ledcWrite(3, 0);
      deviceConnected = true;
    }

    void onDisconnect (BLEServer *pServer) {

      //Desliga o LED quando o dispositivo for desconectado
      Serial.println("Dispositivo Desconectado");
      ledcWrite(1, 0);
      ledcWrite(2, 0);
      ledcWrite(3, 0);
      deviceConnected = false;
    }
};

//Classe que receberá os valores do aplicativo e enviará os valores para o LED
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *rCharacteristic) {
      //Atribui o valor recebido a string rxValue
      std::string rxValue = rCharacteristic->getValue();

      //O app inventor envia as informações em strings
      //"0,0,0"
      //Precisamos transformar os valores em int e atribuir cada um para a respectiva cor (R,G,B)

      //Variáveis temporárias que armazenaram o valor em string
      char tempR[5];
      char tempG[5];
      char tempB[5];

      //Variável que determina em qual posição está a vírgula no rxValue e assim quebrando a string em 3 partes
      int comma = 0;
      int j = 0;
      int k = 0;
      int l = 0;

      //Verifica se o Valor recebido é maior que 0
      if (rxValue.length() > 0) {
        Serial.println("*****************");
        Serial.print("Valor Recebido: ");

        for (int i = 0; i < rxValue.length(); i++) {
          //Imprime cada posição de rxValue
          Serial.print(rxValue[i]);

          //Verifica se a posição i da string é uma vírgula, se for, incrementa a variável comma e pula para a próxima posição
          if (rxValue[i] == ',') {
            comma++;
            continue;
          }

          //"R,G,B", quando comma = 0 valor R, comma = 1 G, comma = 2 B
          if (comma == 0) {
            tempR[j] = rxValue[i];
            j++;
          } else if (comma == 1) {
            tempG[k] = rxValue[i];
            k++;
          } else if (comma == 2) {
            tempB[l] = rxValue[i];
            l++;
          }

        }

        //Encerra a string com um \0
        tempR[j] = '\0';
        tempG[k] = '\0';
        tempB[l] = '\0';

        //Zera as variáveis
        comma = 0;
        j = 0;
        k = 0;
        l = 0;

        //Converte as strings de cada cor para int
        R = atoi(tempR);
        G = atoi(tempG);
        B = atoi(tempB);

        //Zera as strings temporárias
        tempR[0] = '\0';
        tempG[0] = '\0';
        tempB[0] = '\0';

        //Imprime no Serial o Valor recebido em int
        Serial.println();
        Serial.print("R: ");
        Serial.println(R);
        Serial.print("G: ");
        Serial.println(G);
        Serial.print("B: ");
        Serial.println(B);
        Serial.println();

        //Atribui os valores ao led
        ledcWrite(1, R);
        ledcWrite(2, G);
        ledcWrite(3, B);

        Serial.println();
        Serial.println("*****************");
      }
    }
};

void setup() {
  Serial.begin(115200);

  //Define os pinos do LED como Saída
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  //Atribui os canais aos pinos do LED
  ledcAttachPin(ledR, 1);
  ledcAttachPin(ledG, 2);
  ledcAttachPin(ledB, 3);
  
  //Configura os canais para 12kHz PWM e resolução de 8-bits
  ledcSetup(1, 12000, 8); 
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);

  /*
   *Para configurar o BLE é preciso seguir sempre a ordem de configuração:
   *- Cria o Dispositivo e atribui um nome
   *- Cria o Server utilizando um ponteiro e seta ele para a classe que dará o Callback da conexão
   *- Cria o Service utilizando um ponteiro e define o valor com a UUID de serviço
   *- Inicializa a Caraterística de Envio com os parâmetros da UUID de envio e o tipo de característica, no caso Notify para enviar os valores
   *- Inicializa o Descriptador que enviará os valores em string
   *- Inicializa a Caraterística de Recebimento com os parâmetros da UUID de recebimento e o tipo de característica, no caso Write para receber os valores
   *- Seta o Callback que analisará os valores recebidos
   *- Inicializa o Service
   *- Por fim Inicializa a Propagação do Server
   */
  
  //Cria o BLE Device com nome "ESP32"
  BLEDevice::init("ESP32"); 
  
  //Cria o BLE server
  BLEServer *pServer = BLEDevice::createServer(); 
  pServer->setCallbacks(new MyServerCallbacks());
  
  //Cria o BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID); 
  
  //Inicializa a BLE Characteristic de envio
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY); 

  //BLE2902 para notificar
  pCharacteristic->addDescriptor(new BLE2902());

  //Inicializa a BLE Characteristic de recebimento
  BLECharacteristic *rCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  //Seta o Callback de Recebimento 
  rCharacteristic->setCallbacks(new CharacteristicCallbacks());

  //Inicializa o Service
  pService->start(); 

  //Permite que o Server seja encontrado
  pServer->getAdvertising()->start();

  //Pisca o LED em Verde quando inicializado
  Serial.println("ESP32 Inicializado. Esperando conexão...");
  ledcWrite(1, 0);
  ledcWrite(2, 255);
  ledcWrite(3, 0);
  delay(1000);
  ledcWrite(1, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 0);
  delay(1000);

}

void loop() {

  //Se houver um dispositivo conectado começa a enviar os valores da porta analógica a cada 1000ms
  if (deviceConnected) {
    //Atribui os valores da porta analógica a variável int
    txValue = analogRead(analogPin);
    char txString[5];

    //Converte a variável int para ser enviada
    dtostrf(txValue, 4, 0, txString);

    //Coloca a variável para ser enviada pela Característica de Envio
    pCharacteristic->setValue(txString);

    //Envia o valore atribuido a característica através do Notify
    pCharacteristic->notify();

    //Imprime no Serial o valor enviado
    Serial.print("Valor lido: ");
    Serial.print(txValue);
    Serial.print("\n");
    Serial.println("Enviando valor: " + String(txString));
    
    delay(1000);
  }
}
