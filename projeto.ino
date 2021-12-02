// Bibliotecas necessárias para o Projeto
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_CCS811.h"

#define SEALEVELPRESSURE_HPA (1013.25)

//Variáveis do sensor BMP280
Adafruit_BMP280 bmp; // I2C
float temperatura, umidade, pressao, altitude;

//Variáveis do sensor CCS811
Adafruit_CCS811 ccs;
float co2, tvoc;
String sensorClassCO2 = "\"sensor\"";
String sensorClassTVOC = "\"sensor\"";

//Variáveis do Sensor HC-SR04
const int trigPin = 5;
const int echoPin = 18;
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701
long duracao;
float distanciaCm;
float percentualUso = 0.00;

//Parâmetros do Silo
float alturaSilo = 25.00;
float raioSilo = 13.00;
float volumeSilo = 3.1415 * (raioSilo * raioSilo) * alturaSilo;

//Credenciais do WIFI
const char* ssid     = "Overtech";
const char* password = "qazwsxedc";

//Variáveis do servidor HTTP
WiFiServer server(80);
String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;


void setup() {
  Serial.begin(9600);
  bool status;

  //Inicia o sensor BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
  }

  //Inicia o sensor CCS811
  if (!ccs.begin()) {
    Serial.println("Failed to start sensor CSS811! Please check your wiring.");
    while (1);
  }
  //Aguarda o sensor iniciar
  while (!ccs.available());

  //Ajusta os pinos do HC-SR04
  pinMode(trigPin, OUTPUT); // Ajusta trigPin para Saída
  pinMode(echoPin, INPUT); // Ajusta echoPin para Entrada

  //Conecta o WiFi
  Serial.print("Conectando-se a rede ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("Nova conexão");
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            //Mostra a página
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial;}");
            client.println("table { border-collapse: collapse; width:35%; margin-left:auto; margin-right:auto; }");
            client.println("th { padding: 12px; background-color: #7788a5; color: white; }");
            client.println("tr { border: 1px solid #ddd; padding: 12px; }");
            client.println("tr:hover { background-color: #bcbcbc; }");
            client.println("td { border: none; padding: 12px; }");
            client.println(".sensor { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }");
            client.println(".sensorOK { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }");
            client.println(".sensorATENCAO { color:white; font-weight: bold; background-color: #d1c000; padding: 1px; }");
            client.println(".sensorALTO { color:white; font-weight: bold; background-color: #ff0000; padding: 1px; }");

            client.println("</style></head><body><h1>Projeto Monitoramento de Silo</h1>");
            client.println("<table><tr><th>SENSOR</th><th>VALOR</th></tr>");
            client.println("<tr><td>Temperatura</td><td><span class=\"sensor\">");
            client.println(bmp.readTemperature());
            client.println(" *C</span></td></tr>");
            client.println("<tr><td>Press&atilde;o Atmosf&eacute;rica</td><td><span class=\"sensor\">");
            client.println(bmp.readPressure() / 100.0F);
            client.println(" hPa</span></td></tr>");
            client.println("<tr><td>Altitude Aproximada</td><td><span class=\"sensor\">");
            client.println(altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA));
            client.println(" m</span></td></tr>");
            client.println("<tr><td>Umidade</td><td><span class=\"sensor\">");
            client.println("59");
            client.println(" %</span></td></tr>");
            buscaQualidadeAR();                        
            client.println("<tr><td>Di&oacute;xido de Carbono (CO2) </td><td><span class=" + sensorClassCO2 + ">");
            client.println(co2);
            client.println(" ppm</span></td></tr>");
            client.println("<tr><td>Total de Compostos Org&acirc;nicos Vol&aacute;teis(TVOC)</td><td><span class=" + sensorClassTVOC + ">");
            client.println(tvoc);
            client.println(" ppb</span></td></tr>");
            buscaDistancia();
            client.println("<tr><td> </tr></td>");
            client.println("<tr><td>Dist&acirc;ncia aproximada at&eacute; a linha de gr&atilde;os</td><td><span class=\"sensor\">");
            client.println(distanciaCm);
            client.println(" cm</span></td></tr>");
            client.println("<tr><td>Porcentagem aproximada de utiliza&ccedil;&atilde;o</td><td><span class=\"sensor\">");
            client.println(percentualUso);
            client.println(" %</span></td></tr>");
            
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void buscaQualidadeAR() {
  if (ccs.available()) {
    if (!ccs.readData()) {
      Serial.print("CO2: ");
      Serial.print(ccs.geteCO2());
      Serial.print("ppm, TVOC: ");
      Serial.println(ccs.getTVOC());
      co2 = ccs.geteCO2();
      tvoc = ccs.getTVOC();

      //Trata as faixas de exibição do co2
      if (co2 < 1000) {
        sensorClassCO2 = "\"sensorOK\"";
      } else if ((co2 >= 1000) && (co2 < 5000)) {
        sensorClassCO2 = "\"sensorATENCAO\"";
      } else {
        sensorClassCO2 = "\"sensorALTO\"";
      }

      //Trata as faixas de exibição do co2
      if (tvoc < 250) {
        sensorClassTVOC = "\"sensorOK\"";
      } else if ((tvoc >= 250) && (tvoc < 2000)) {
        sensorClassTVOC = "\"sensorATENCAO\"";
      } else {
        sensorClassTVOC = "\"sensorALTO\"";
      }

    }
    else {
      Serial.println("ERROR!");
      while (1);
    }
  }
}

void buscaDistancia() {
  // Limpa o pino de trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Seta o pino em estado HIGH por 10 microsegundos 
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Le o pino de echo
  duracao = pulseIn(echoPin, HIGH);
  
  // Calcula a distância
  distanciaCm = duracao * SOUND_SPEED/2;  
  
  Serial.print("Distancia (cm): ");
  Serial.println(distanciaCm);

  //Calcula porcentagem de uso
  percentualUso = 100.00 - ((distanciaCm*100)/alturaSilo);
  
  
  delay(1000);
}

void testBMP() {
  temperatura = bmp.readTemperature();
  umidade = 59.0;
  pressao = bmp.readPressure() / 100.0F;
  altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.print("Temperatura: ");
  Serial.println(temperatura);
  Serial.print("Umidade: ");
  Serial.println(umidade);
  Serial.print("Pressão: ");
  Serial.println(pressao);
  Serial.print("Altitude: ");
  Serial.println(altitude);

  delay(1000);
}
