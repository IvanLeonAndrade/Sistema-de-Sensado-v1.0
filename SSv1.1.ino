/*  Sistema de Sensado Versión 1.0 (SSv1.0)
 *  Copyright (C) 2023  Iván León Andrade Franco
 *  Empresa: Silica Networks SA

 *  Mail Personal: ivan.batapum@gmail.com
 *  Mail Coorporativo: Ivan.Andrade@datco.net
 *
 *  El equipo sensa temperatura, humedad y corriente 
 *  continua. Los valores se obtienen mediante SNMP V1.0
 *
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Streaming.h>
#include <Flash.h>
#include <MemoryFree.h>
#include <Agentuino.h> 
#include <DHT.h>

const uint8_t dht_pin = 5;
const uint8_t current_sensor_pin = 0;

float current = 0.0;
float humidity = 0.0;
float temperature = 0.0;

// Establecer afinidad segun el sensor.
const float cs_voltage_offset = 2475.0;
const float cs_sensitivity = 12.0;

DHT dht(dht_pin, DHT22);

static byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF};

IPAddress ip(192, 168, 1, 95);
IPAddress ip_dns(192, 168, 5, 1); 
IPAddress ip_gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

char result[8];

const char sysShelter[] PROGMEM = "1.3.6.1.2.1.1.5.0";
const char sysLocation[] PROGMEM = "1.3.6.1.2.1.1.6.0";
const char sysServices[] PROGMEM = "1.3.6.1.2.1.1.7.0";

const char snmp_temperature[] PROGMEM = "1.3.6.1.3.2016.5.1.0";
const char snmp_humidity[] PROGMEM = "1.3.6.1.3.2016.5.1.1";
const char snmp_current[] PROGMEM = "1.3.6.1.3.2016.5.1.2";

static char locShelter[7] = "Abasto";
static char locLocation[5] = "AMBA";
static int32_t locServices = 2;

uint32_t prevMillis = millis();
char oid[SNMP_MAX_OID_LEN];
SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;

/*  calculo_current
 *  Ajustar los parametros segun las caracteristicas del sensor.
 *  cs: Current sensor
 *  
 *  @cs_sensitivity: Voltage reading ratio per Ampere [12mV/A]
 *  @cs_voltage_offset: Centra la lectura de tension a 0
 */
float calculate_current(){
  float cs_sum = 0.0;
  float cs_voltage = 0.0;
  for (uint8_t i = 0; i < 4; i++) {
    cs_voltage = (analogRead(current_sensor_pin) * 5000.0) / 1023.0;
    cs_sum += (cs_voltage - cs_voltage_offset) / cs_sensitivity;
    delay(250);
  }
  return cs_sum / 4.0;
}

// La libreria Agentuino maneja las solicitudes SNMP. 
void pduReceived() {
  SNMP_PDU pdu;
  api_status = Agentuino.requestPdu(&pdu);

  bool isSnmpGetOperation = pdu.type == SNMP_PDU_GET || pdu.type == SNMP_PDU_GET_NEXT || pdu.type == SNMP_PDU_SET;
  bool isErrorFree = pdu.error == SNMP_ERR_NO_ERROR;
  bool isSuccess = api_status == SNMP_API_STAT_SUCCESS;

  if (isSnmpGetOperation && isErrorFree && isSuccess) {
    pdu.OID.toString(oid);
     if (strcmp_P(oid, sysShelter) == 0) {
      if (pdu.type == SNMP_PDU_SET) {
        status = pdu.VALUE.decode(locShelter, strlen(locShelter));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locShelter);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
    else if (strcmp_P(oid, sysLocation) == 0) {
      if (pdu.type == SNMP_PDU_SET) {
        status = pdu.VALUE.decode(locLocation, strlen(locLocation));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locLocation);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
    else if (strcmp_P(oid, sysServices) == 0) {
      if (pdu.type == SNMP_PDU_SET) {
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      }
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, locServices);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
    else if (strcmp_P(oid, snmp_temperature) == 0) {
      if (pdu.type == SNMP_PDU_SET) {
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      }
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(temperature, 6, 2, result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
    else if (strcmp_P(oid, snmp_humidity) == 0) {
      if (pdu.type == SNMP_PDU_SET) {
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      }
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(humidity, 6, 2, result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
    else if (strcmp_P(oid, snmp_current) == 0) {
      if (pdu.type == SNMP_PDU_SET) {
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      }
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(current, 6, 2, result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }
    else {
      pdu.type = SNMP_PDU_RESPONSE;
      pdu.error = SNMP_ERR_NO_SUCH_NAME;
    }
    Agentuino.responsePdu(&pdu);
  }
  Agentuino.freePdu(&pdu);
}

void setup() {
  Ethernet.begin(mac, ip, ip_dns, ip_gateway, subnet);
  api_status = Agentuino.begin();
  dht.begin();
  
  if (api_status == SNMP_API_STAT_SUCCESS) {
    Agentuino.onPduReceive(pduReceived);
    delay(10);
    return;
  }
  delay(1000);
}

void loop() {
  Agentuino.listen();
  
  if (millis() - prevMillis > 5000) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    current = calculate_current();
    
    prevMillis = millis();
  }
}
