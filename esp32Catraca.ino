#include <WiFi.h>
#include <Arduino.h>
#include <HTTPClient.h>   
#include <Arduino_JSON.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Wiegand.h>

#define bobina 13
#define sensorSaida 27
#define sensorEntrada 26
#define setaEntrada 15
#define setaSaida 2

char ssid[] = "";         // your SSID
char pass[] = "";     // your SSID Password
String serverName = ""; // your url api ex."http://192.168.0.1:9090/api/acesso"
String tag;
boolean sensorAtuado = false;
boolean lastStateSensorEntrada = 1;
boolean lastStateSensorSaida = 1;


WIEGAND wg;

LiquidCrystal_I2C lcd(0x27,16,2);

class JsonResponse {

 public:
  String usuario;
  String tipoUsuario;
  String timestamp;
  String mensagem;
  boolean acessoLiberado;
  String entradaSaida;
};

JsonResponse jsonResponse;

void setup() {
  pinMode(bobina, OUTPUT);
  pinMode(setaSaida, OUTPUT);
  pinMode(setaEntrada, OUTPUT);
  pinMode(sensorSaida, INPUT);
  pinMode(sensorEntrada, INPUT);

  wg.D0PinA = 16;
  wg.D1PinA = 17;

  wg.D0PinB = 18;
  wg.D1PinB = 19;

  wg.D0PinC = 20;
  wg.D1PinC = 21;

  wg.begin(1, 1, 0);

  lcd.begin();
  lcd.setBacklight(HIGH);
  Serial.begin(115200);
  wifiConnection();
  wg.begin(1,1,0);
  lcdPrintHome();
}

void loop() {

  setSetaBloqueio();
  if (digitalRead(sensorEntrada) == 0 || digitalRead(sensorSaida) == 0) {
    if(sensorAtuado == false){
      lcdPrintSensorAtuado();
      sensorAtuado = true;
    }
    digitalWrite(bobina, LOW);
    delay(500);
  }

  else{
    if(sensorAtuado == true){
      lcdPrintHome();
      sensorAtuado = false;
    }
    digitalWrite(bobina, HIGH);
  }
   
   if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
    if(wg.available()){
       tagCheck(String(wg.getCode()));
    }
  }
}

void tagCheck(String tag){
    HTTPClient http;   
    String serverNameWithTag = serverName + tag;
    http.begin(serverNameWithTag); 
    http.addHeader("Content-Type", "text/plain"); 
    
    int httpResponseCode = http.GET();
    String response = http.getString(); 
    
    JSONVar json = JSON.parse(response);
    if (JSON.typeof(json) == "undefined") {
        Serial.println("Json response undefined!");
        return;
      }
 
    JSONVar jsonKeys = json.keys();
    jsonResponse.usuario = json[jsonKeys[0]];
    jsonResponse.tipoUsuario = json[jsonKeys[1]];
    jsonResponse.timestamp = json[jsonKeys[2]];
    jsonResponse.mensagem = json[jsonKeys[3]];
    jsonResponse.acessoLiberado = json[jsonKeys[4]];
    jsonResponse.entradaSaida = json[jsonKeys[5]];
    if(jsonResponse.acessoLiberado == true) {
      if(jsonResponse.tipoUsuario.compareTo("Aluno") == 0){
        lastStateSensorEntrada = 1;
        lastStateSensorSaida = 1;
        if(jsonResponse.entradaSaida == "ENTRADA" || jsonResponse.entradaSaida == "ALMOCO_ENTRADA"){
          lcdPrintEntradaLiberada();
          setSetaEntrada();
          for (int i = 0; i <= 400; i++) {
            if (digitalRead(sensorSaida) == 0) {
              digitalWrite(bobina, LOW);
            }
            else{
              digitalWrite(bobina, HIGH);
              if(digitalRead(sensorEntrada) == 0){
                lastStateSensorEntrada = 0;
              }
              if(digitalRead(sensorEntrada) != lastStateSensorEntrada){
                lastStateSensorEntrada = 1;
                lcdPrintHome();
                http.end();
                return;
              }
            }
            delay(15);
          }
        }else if(jsonResponse.entradaSaida == "SAIDA" || jsonResponse.entradaSaida == "ALMOCO_SAIDA"){
          lcdPrintSaidaLiberada();
          setSetaSaida();
         for (int i = 0; i <= 400; i++) {
            if (digitalRead(sensorEntrada) == 0) {
              digitalWrite(bobina, LOW);
            }
            else{
              digitalWrite(bobina, HIGH);
              if(digitalRead(sensorSaida) == 0){
                lastStateSensorSaida = 0;
              }
              if(digitalRead(sensorSaida) != lastStateSensorSaida){
                lastStateSensorSaida = 1;
                lcdPrintHome();
                http.end();
                return;
              }
            }
            delay(15);
          }
      }
    }else if(jsonResponse.tipoUsuario.compareTo("Funcionario") == 0){
        lastStateSensorEntrada = 1;
        lastStateSensorSaida = 1;
        lcdPrintAcessoLiberado("FUNCIONARIO");
        setSetaDesbloqueio(); 
        lastStateSensorEntrada = 1;
        lastStateSensorSaida = 1;
        for (int i = 0; i <= 400; i++) {
           digitalWrite(bobina, HIGH);
           if(digitalRead(sensorEntrada) == 0 && digitalRead(sensorSaida) == 1){
                lastStateSensorEntrada = 0;
              }
              if(digitalRead(sensorEntrada) != lastStateSensorEntrada){
                lastStateSensorEntrada = 1;
                lcdPrintHome();
                http.end();
                return;
              }
           if(digitalRead(sensorSaida) == 0 && digitalRead(sensorEntrada) == 1){
                lastStateSensorSaida = 0;
              }
              if(digitalRead(sensorSaida) != lastStateSensorSaida){
                lastStateSensorSaida = 1;
                lcdPrintHome();
                http.end();
                return;
              }
            delay(15);
          }
        
      }else if(jsonResponse.tipoUsuario.compareTo("Visitante") == 0){
        lastStateSensorEntrada = 1;
        lastStateSensorSaida = 1;
        lcdPrintAcessoLiberado("VISITANTE");
        setSetaDesbloqueio();
        for (int i = 0; i <= 400; i++) {
           digitalWrite(bobina, HIGH);
           if(digitalRead(sensorEntrada) == 0){
                lastStateSensorEntrada = 0;
              }
              if(digitalRead(sensorEntrada) != lastStateSensorEntrada){
                lastStateSensorEntrada = 1;
                lcdPrintHome();
                http.end();
                return;
              }
           if(digitalRead(sensorSaida) == 0){
                lastStateSensorSaida = 0;
              }
              if(digitalRead(sensorSaida) != lastStateSensorSaida){
                lastStateSensorSaida = 1;
                lcdPrintHome();
                http.end();
                return;
              }
            delay(15);
          }
      }else{
      lcdPrint("Erro", "Erro", 1000);
    }
 
 }else if (jsonResponse.acessoLiberado == false){
      lcdPrintAcessoNaoAutorizado();
      setSetaBloqueio();
      digitalWrite(bobina, LOW);
      delay(1000);
    }else{
      lcdPrint("Erro", "Erro", 1000);
    }
 lcdPrintHome();
 http.end();
}

 void wifiConnection() {
  Serial.printf("\nConnecting to %s", ssid);
  lcd.setCursor(0,0);
  lcd.print("CONECTANDO A: ");
  lcd.setCursor(0,1);
  lcd.print(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado!");
  Serial.print("My IP address is: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CONECTADO! ");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());
  delay(1000);
  
}

void setSetaEntrada(){
  digitalWrite(setaEntrada, LOW);
  digitalWrite(setaSaida, HIGH);
}

void setSetaSaida(){
  digitalWrite(setaEntrada, HIGH);
  digitalWrite(setaSaida, LOW);
}

void setSetaBloqueio(){
  digitalWrite(setaEntrada, HIGH);
  digitalWrite(setaSaida, HIGH);
}

void setSetaDesbloqueio(){
  digitalWrite(setaEntrada, LOW);
  digitalWrite(setaSaida, LOW);
}

void serialPrintTag(){
   Serial.print("Wiegand HEX = ");
   Serial.print(wg.getCode(),HEX);
   Serial.print(", DECIMAL = ");
   Serial.print(wg.getCode());
   Serial.print(", Type W");
   Serial.println(wg.getWiegandType()); 
}

void lcdPrintHome(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SEJA BEM VINDO");
  lcd.setCursor(0,1);
  lcd.print("A ESCOLA SENAI!");
}

void lcdPrintEntradaLiberada(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("ENTRADA");
  lcd.setCursor(4, 1);
  lcd.print("LIBERADA");
}

void lcdPrintSaidaLiberada(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("SAIDA");
  lcd.setCursor(4, 1);
  lcd.print("LIBERADA");
}

void lcdPrintAcessoLiberado(String perfil){
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print(perfil);
  lcd.setCursor(3, 1);
  lcd.print("AUTORIZADO");
}

void lcdPrintAcessoNaoAutorizado(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("ACESSO");
  lcd.setCursor(1, 1);
  lcd.print("NAO AUTORIZADO");
}

void lcdPrint(String text1, String text2,int timeDelay){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(text1);
  lcd.setCursor(0,1);
  lcd.print(text2);
  delay(timeDelay);
}

void serialPrintObject(){
  Serial.println(jsonResponse.usuario);
  Serial.println(jsonResponse.tipoUsuario);
  Serial.println(jsonResponse.timestamp);
  Serial.println(jsonResponse.mensagem);
  Serial.println(jsonResponse.acessoLiberado);
  Serial.println(jsonResponse.entradaSaida);
  
}

void lcdPrintSensorAtuado(){
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("POR FAVOR");
  lcd.setCursor(0,1);
  lcd.print("PASSE O CRACHA");
  
}
