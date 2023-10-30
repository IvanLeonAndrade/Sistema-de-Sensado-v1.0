#include <SPI.h>
#include <Ethernet.h>
#include <Streaming.h>
#include <Flash.h>
#include <MemoryFree.h>
#include <Agentuino.h>
#include <DHT.h>

  
// CS = Current sensor; DHT22 = Temp & Hum sensor
const uint8_t  dht_pin = 5;
const uint8_t  cs_pin = 0;

// Variables transmitidas via SNMP
float current = 0.0;
float humedad = 0.0;
float temperatura = 0.0;

// Calibrar CS
const float cs_voltage_delta = 2475.0; // Centra lectura del sensor en 0V
const float cs_relation_VoltAmper = 12.0; // Sensibilidad: mV/A

DHT dht(dht_pin, DHT22);

static byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 95);
IPAddress ip_dns(192, 168, 1, 1);
IPAddress ip_gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);


char result[8];

const char sysDescr[] PROGMEM      = "1.3.6.1.2.1.1.1.0";
const char sysContact[] PROGMEM    = "1.3.6.1.2.1.1.4.0";
const char sysName[] PROGMEM       = "1.3.6.1.2.1.1.5.0";
const char sysLocation[] PROGMEM   = "1.3.6.1.2.1.1.6.0";
const char sysServices[] PROGMEM   = "1.3.6.1.2.1.1.7.0";

const char snmp_temperature[]     PROGMEM     = "1.3.6.1.3.2016.5.1.0";
const char snmp_humidity[]        PROGMEM     = "1.3.6.1.3.2016.5.1.1";
const char snmp_current[]        PROGMEM     = "1.3.6.1.3.2016.5.1.2";

static char locDescr[35]            = "Sistema de Sensado v1.0";
static char locContact[25]          = "Silica Networks SA";
static char locName[20]             = "Made NOC";
static char locLocation[20]         = "SMA 638 - CABA";
static int32_t locServices          = 2;

uint32_t prevMillis = millis();
char oid[SNMP_MAX_OID_LEN];
SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;

//  Promedia y calcula los valores de corriente.
float deltaCurrent() {
  float current_sum = 0.0;
  float cs_voltage = 0.0;

  for (int i = 0; i < 4; i++) {
    cs_voltage = (analogRead(cs_pin) * 5000.0) / 1023.0;
    current_sum += (cs_voltage - cs_voltage_delta) / cs_relation_VoltAmper;
    delay(250);
  }

  return current_sum / 4.0;
}


void pduReceived() {
  SNMP_PDU pdu;
  api_status = Agentuino.requestPdu(&pdu);

  if ((pdu.type == SNMP_PDU_GET || pdu.type == SNMP_PDU_GET_NEXT || pdu.type == SNMP_PDU_SET)
      && pdu.error == SNMP_ERR_NO_ERROR && api_status == SNMP_API_STAT_SUCCESS ) {

    pdu.OID.toString(oid);

    if ( strcmp_P(oid, sysDescr ) == 0 ) {
      if ( pdu.type == SNMP_PDU_SET ) {

        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {

        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locDescr);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else if ( strcmp_P(oid, sysName ) == 0 ) {

      if ( pdu.type == SNMP_PDU_SET ) {

        status = pdu.VALUE.decode(locName, strlen(locName));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {

        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locName);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else if ( strcmp_P(oid, sysContact ) == 0 ) {

      if ( pdu.type == SNMP_PDU_SET ) {

        status = pdu.VALUE.decode(locContact, strlen(locContact));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {

        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locContact);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else if ( strcmp_P(oid, sysLocation ) == 0 ) {

      if ( pdu.type == SNMP_PDU_SET ) {

        status = pdu.VALUE.decode(locLocation, strlen(locLocation));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {

        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locLocation);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysServices) == 0 ) {

      if ( pdu.type == SNMP_PDU_SET ) {

        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {

        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, locServices);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else if ( strcmp_P(oid, snmp_temperature ) == 0 ) {

      if ( pdu.type == SNMP_PDU_SET ) {

        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      }
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(temperatura, 6, 2, result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else if ( strcmp_P(oid, snmp_humidity ) == 0 ) {

      if ( pdu.type == SNMP_PDU_SET ) {

        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      }
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(humedad, 6, 2, result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else if ( strcmp_P(oid, snmp_current ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {

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
  dht.begin();
  Ethernet.begin(mac, ip, ip_dns, ip_gateway, subnet);

  // Inicia agente SNMP
  api_status = Agentuino.begin();

  // Verificación estado agente SNMP
  if ( api_status == SNMP_API_STAT_SUCCESS ) {
    Agentuino.onPduReceive(pduReceived);
    delay(10);
    return;
  }

  // Retraso estado no éxito
  delay(10);
}


void loop() {
  Agentuino.listen();

  if (millis() - prevMillis > 2000) {
    temperatura = dht.readTemperature();
    current = deltaCurrent();
    humedad = dht.readHumidity();
    prevMillis = millis();
  }
}
