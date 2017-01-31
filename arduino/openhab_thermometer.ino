
/**
 * Simple OpenHAB thermometer/humidity sensor based on ESP8266-01.
*/
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

// Pin where is DTH22 connected
#define DHT_PIN 2


// Check URL for WIFI
#define DEFAULT_CHECK_HOST "192.168.128.100"
#define DEFAULT_CHECK_PORT 9000
#define DEFAULT_CHECK_URL  "/rest"

// Frequency of updating temperature/humidity values
#define DEFAULT_UPDATE_FREQUENCY 5

// Frequency of check whether WiFi is alive
#define DEFAULT_CHECK_FREQUENCY 60

// Local network definition; must have static IP address
#define LOCAL_IP IPAddress(192, 168, 128, 205)
#define GATEWAY IPAddress(192, 168, 128, 1)
#define SUBNETMASK IPAddress(255, 255, 255, 0)

// Local Wifi AP for connect
#define DEFAULT_WIFI_AP "******"
#define DEFAULT_WIFI_PASSWORD "******"

// Error count for reconnect WIFI
#define ERR_COUNT_FOR_RECONNECT 30
#define HTTP_READ_TIMEOUT       10000
#define HTTP_CONNECT_TIMEOUT    5000

// Local WEB server port
#define WEB_SERVER_PORT 80

// Magic for detecting empty (unconfigured) EEPROM
#define MAGIC 0xCD

/**
 * EEPROM structure.
*/
struct OhConfiguration {
  int magic;
  char apName [24];
  char password [48];
  char checkHost [20];
  unsigned int checkPort;
  char checkUrl [50];
  unsigned int checkFrequency;
  unsigned int updateFrequency;
};

// Configuration
OhConfiguration config;

// Configuration server
ESP8266WebServer *server = (ESP8266WebServer *) NULL;

// Last interaction time
long lastInteractionTime;

// Last check time
long lastCheckTime;

// Number of communication errors
int errorCount = 0;

// Last temperature.
float lastTemperature = 0.0;

// Last humidity.
float lastHumidity = 0.0;

// DHT interface
DHT dht (DHT_PIN, DHT22);

/**
 * Setup method.
*/
void setup() {
  Serial.begin (115200);
  Serial.println ("setup()");

  // Read config
  EEPROM.begin (sizeof (OhConfiguration));
  EEPROM.get (0, config);
  if (config.magic != MAGIC) {
    memset (&config, 0, sizeof (OhConfiguration));
    config.magic = MAGIC;
    updateConfigKey (config.apName, 24, String(DEFAULT_WIFI_AP));
    updateConfigKey (config.password, 48, String(DEFAULT_WIFI_PASSWORD));
    config.updateFrequency = DEFAULT_UPDATE_FREQUENCY;
    updateConfigKey (config.checkHost, 20, String (DEFAULT_CHECK_HOST));
    config.checkPort = DEFAULT_CHECK_PORT;
    updateConfigKey (config.checkUrl, 50, String(DEFAULT_CHECK_URL));
    config.checkFrequency = DEFAULT_CHECK_FREQUENCY;
  }

  reconnectWifi();
  createServer();

  lastInteractionTime = millis();
  lastCheckTime = lastInteractionTime;

  // Init interface object
  Serial.println ("Initializing DHT...");
  dht.begin();

  delay (1000);
}

/**
 * Helper routine; updates config key.
 *
 * @param c key.
 * @param len max length.
 * @param val value.
*/
void updateConfigKey (char *c, int len, String val) {
  memset (c, 0, len);
  sprintf (c, "%s", val.c_str());
}

/**
 * Reconects WIFI.
*/
void reconnectWifi() {
  Serial.println ("WiFi disconnected...");
  WiFi.disconnect();
  String hostname = String("openhab_temp/hum sensor");
  WiFi.hostname (hostname);
  WiFi.config (LOCAL_IP, GATEWAY, SUBNETMASK);
  WiFi.mode (WIFI_STA);
  delay (1000);

  Serial.print ("Connecting to "); Serial.print (WIFI_AP); Serial.print (' ');
  WiFi.begin (config.apName, config.password);
  delay (1000);
  while (WiFi.status() != WL_CONNECTED) {
    yield();
    delay (500);
    Serial.print (".");
  }
  delay (500);
  Serial.println();
  Serial.println ("WiFi connected...");
}

/**
 * Creates HTTP server.
*/
void createServer() {
  if (server) {
    server->close();
    delete server;
    server = NULL;
  }
  
  // ... and run HTTP server for setup
  server = new ESP8266WebServer (LOCAL_IP, WEB_SERVER_PORT);
  server->on ("/temperature", wsGetTemp);
  server->on ("/humidity", wsGetHumidity);
  server->on ("/tempIndex", wsGetTempIndex);
  server->on ("/", wsConfig);
  server->on ("/update", wsUpdate);
  server->on ("/reconnect", wsReconnect);
  server->begin();

  Serial.println ("Created server...");
}

/**
 * GETs temperature.
*/
void wsGetTemp() {
  server->send (200, "text/html", String(lastTemperature));
}

/**
 * GETs humidity.
*/
void wsGetHumidity() {
  server->send (200, "text/html", String(lastHumidity));
}

/**
 * GETs temperature index.
*/
void wsGetTempIndex() {
  server->send (200, "text/html", String(dht.computeHeatIndex (lastTemperature, lastHumidity, false)));
}

/**
 * Configuration page.
*/
void wsConfig() {
  yield();

  String resp = "<html><head><title>OpenHAB switch configuration</title>";
  resp += "<meta name=\"viewport\" content=\"initial-scale=1.0, width = device-width, user-scalable = no\">";
  resp += "</head><body>";
  resp += "<h1>OpenHAB temperature/humidity sensor configuration</h1>";
  resp += "<form method=\"post\" action=\"/update\" id=\"form\">";
  resp += "<table border=\"0\" cellspacing=\"0\" cellpadding=\"5\">";
  resp += "<tr><td>AP SSID:</td><td><input type=\"text\" name=\"apName\" value=\"" + String(config.apName) + "\" maxlength=\"24\"></td><td></td></tr>";
  resp += "<tr><td>AP Password:</td><td><input type=\"password\" name=\"password\" value=\"" + String(config.password) + "\" maxlength=\"48\"></td><td></td></tr>";
  resp += "<tr><td>Update sensor time [sec]:</td><td><input type=\"text\" name=\"updateFrequency\" value=\"" + String(config.updateFrequency) + "\"></td><td></td></tr>";
  resp += "<tr><td>Check host IP:</td><td><input type=\"text\" name=\"checkHost\" value=\"" + String(config.checkHost) + "\" maxlength=\"20\"></td><td></td></tr>";
  resp += "<tr><td>Check port:</td><td><input type=\"text\" name=\"checkPort\" value=\"" + String(config.checkPort) + "\" maxlength=\"5\"></td><td></td></tr>";
  resp += "<tr><td>Check URL:</td><td><input type=\"text\" name=\"checkUrl\" value=\"" + String(config.checkUrl) + "\" maxlength=\"50\"></td><td></td></tr>";
  resp += "<tr><td>Check frequency [sec]:</td><td><input type=\"text\" name=\"checkFrequency\" value=\"" + String(config.checkFrequency) + "\"></td><td></td></tr>";

  resp += "<tr><td colspan=\"3\" align=\"center\"><input type=\"submit\" value=\"Save\"></td></tr>";
  resp += "</table></form>";
  resp += "<p><a href=\"/reconnect\">Reconnect WiFi...</a></p>";
  resp += "</body></html>";

  server->send (200, "text/html", resp);
}

/**
 * Reconnects WiFi with new parameters.
*/
void wsReconnect() {
  yield();
  String resp = "<script>window.alert ('Reconnecting WiFi...'); window.location.replace ('/');</script>";
  server->send (200, "text/html", resp);
  reconnectWifi();
  createServer();
}

/**
 * Saves configuration.
*/
void wsUpdate() {
  yield();

  String apName = server->arg ("apName");
  String password = server->arg ("password");
  unsigned int updateFrequency = atoi (server->arg ("updateFrequency").c_str());
  String checkHost = server->arg ("checkHost");
  unsigned int checkPort = atoi (server->arg ("checkPort").c_str());
  String checkUrl = server->arg ("checkUrl");
  unsigned int checkFrequency = atoi (server->arg("checkFrequency").c_str());
  
  if (apName.length() > 1) {
    updateConfigKey (config.apName, 24, apName);
    updateConfigKey (config.password, 48, password);
    config.updateFrequency = constrain (updateFrequency, 1, 600);
    updateConfigKey (config.checkHost, 20, checkHost);
    config.checkPort = constrain (checkPort, 1, 65535);
    updateConfigKey (config.checkUrl, 50, checkUrl);
    config.checkFrequency = constrain (checkFrequency, 0, 30000);
  
    // store configuration
    EEPROM.begin (sizeof (OhConfiguration));
    EEPROM.put (0, config);
    EEPROM.end();
  
    String resp = "<script>window.alert ('Configuration updated...'); window.location.replace ('/');</script>";
    server->send (200, "text/html", resp);
  } else {
    server->send (200, "text/html", "");
  }
}

/**
 * Checks WIFI connection by getting check URL.
*/
void checkWiFi() {
  if ((strlen (config.checkHost) < 1) || (config.checkFrequency < 1)) {
    return;
  }

  WiFiClient client;

  // connect to OpenHAB
  Serial.print ("Connecting to http://"); Serial.print (config.checkHost); Serial.print (':'); Serial.print (config.checkPort); Serial.print (config.checkUrl); Serial.println (" ...");
  if (client.connect (config.checkHost, config.checkPort)) {
    // send request
    String req = String("GET ") + config.checkUrl + String (" HTTP/1.1\r\n")
      + String("Host: ") + String (config.checkHost) + String ("\r\nConnection: close\r\n\r\n");
    client.print (req);
    Serial.print (req);

    bool isError = false;
    
    // wait HTTP_CONNECT_TIMEOUT for response
    unsigned long connectStartTime = millis();
    while (client.available() == 0) {
      if (millis() - connectStartTime > HTTP_CONNECT_TIMEOUT) {
        errorCount++;
        isError = true;
        break;
      }

      yield();
    }

    Serial.print ("Reading response -> ");
    if (!isError) {
      // read response lines
      unsigned long readStartTime = millis();

      int ch;
      while ((ch = client.read()) != -1) {
        if (millis() - readStartTime > HTTP_READ_TIMEOUT) {
          errorCount++;
          isError = true;
          break;
        }

        yield();
      }

      Serial.println ("OK");
      
      client.stop();
    } else {
      Serial.println ("ERROR");
      client.stop();
    }
  }
}

/**
 * Loop.
*/
void loop() {
  if (server) {
    server->handleClient();
  }

  if (millis() - lastInteractionTime > config.updateFrequency * 1000L) {
    yield();
    
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      lastTemperature = t;
      lastHumidity = h;
      
      Serial.print("Humidity: ");
      Serial.print(lastHumidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(lastTemperature);
      Serial.println(" *C ");
    }

    lastInteractionTime = millis();
  }

  if (millis() - lastCheckTime > config.checkFrequency * 1000L) {
    yield();
    
    if (WiFi.status() == WL_CONNECTED) {
      checkWiFi();
    } else {
      Serial.println ("Wifi not connected!");
      errorCount++;
    }

    // when error count reaches ERR_COUNT_FOR_RECONNECT, reconnect WiFi.
    if (errorCount > ERR_COUNT_FOR_RECONNECT) {
      errorCount = 0;
      reconnectWifi();
      createServer();
    }

    lastCheckTime = millis();
  }

  yield();
}

