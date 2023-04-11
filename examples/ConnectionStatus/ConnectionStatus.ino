/* Example of tracking client connection status.
 *
 * Onboard LED blinks as long as there is an active connection, and page shows time of last connection.
 * 
 * This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <EmbAJAX.h>
#include <EmbAJAXScriptedSpan.h>

#define LEDPIN LED_BUILTIN

EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

bool connection_active = false;
uint64_t previous_connection_start = 0;

char value_buf[20] = "0";
EmbAJAXScriptedSpan m_script_s("m_script_s", "this.receiveValue = function(value) { this.innerHTML = (value == 0) ? 'Never - you were the first to connect' :  (Number(value) / 1000) + ' seconds server time'; };");

MAKE_EmbAJAXPage(page, "EmbAJAX example - Connection Status", "",
  new EmbAJAXStatic("<p>Previous connection to this page started at: <b>"),
  &m_script_s,
  new EmbAJAXStatic("</b></p><p>Note: This demo does not actually track individual connections, only the fact whether at least one client was connected. An actual use case might be "
                    "engaging a safety procedure, if the connection to controlling clients has been lost.</p>"),
)

void setup() {
  // Example WIFI setup as an access point. Change this to whatever suits you, best.
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig (IPAddress (192,168,4,1), IPAddress (0,0,0,0), IPAddress (255,255,255,0));
  WiFi.softAP("EmbAJAXTest", "12345678");

  m_script_s.setValue(value_buf);
  driver.installPage(&page, "/");
  server.begin();

  pinMode(LEDPIN, OUTPUT);
}

void newConnection() {
  m_script_s.setValue(itoa(previous_connection_start ? (millis() - previous_connection_start) : 0, value_buf, 10));
  previous_connection_start = millis();
}

void loop() {
  // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
  driver.loopHook();

  if (page.hasActiveClient() != connection_active) {
    if (!connection_active) newConnection();
    connection_active = !connection_active;
  }

  digitalWrite(LEDPIN, connection_active && ((millis() / 200) % 2));
}
