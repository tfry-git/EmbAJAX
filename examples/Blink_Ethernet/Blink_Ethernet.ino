/* Demonstrate usage of with of EmbAJAX over an ethernet connection.
 *
 * This example is mostly identical to the basic Blink example, but shows how to specify
 * a different output driver. In this case the class EthernetWebServer from the library
 * of the same name is used (compatible with many different boards, including ATMEGA 2560).
 * In a similar fashion additionals boards can easily be supported, as long as a compatible
 * webserver implementation is available.
 *
 * For details on how to properly connect, and configure your ethernet shield, refer to the
 * documentation of the Ethernet library. The example used here should work in many cases,
 * however, and will create an EmbAJAX server listening on http://192.168.1.177 .
 *
 * This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <EthernetWebServer.h>
#define EmbAJAXOutputDriverWebServerClass EthernetWebServer
#include <EmbAJAXOutputDriverGeneric.h>
#include <EmbAJAX.h>

// default for most Arduino ethernet shields
#define ETHERNET_CS_PIN 10
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);

#define LEDPIN LED_BUILTIN

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDriverWebServerClass is a
// convenience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

// Define the main elements of interest as variables, so we can access to them later in our sketch.
const char* modes[] = {"On", "Blink", "Off"};
EmbAJAXRadioGroup<3> mode("mode", modes);
EmbAJAXSlider blinkfreq("blfreq", 0, 1000, 100);   // slider, from 0 to 1000, initial value 100

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "EmbAJAX example - Blink", "",
  new EmbAJAXStatic("<h1>Control the builtin LED</h1><p>Set the LED to: "),
  &mode,
  new EmbAJAXStatic("</p><p>Blink frequency: <i>SLOW</i>"),
  &blinkfreq,
  new EmbAJAXStatic("<i>FAST</i></p>")
)

void setup() {
  // Init Ethernet. For many 
  Ethernet.init(ETHERNET_CS_PIN);
  Ethernet.begin(mac, ip);

  // Tell the server to serve our EmbAJAX test page on root
  // installPage() abstracts over the (trivial but not uniform) WebServer-specific instructions to do so
  driver.installPage(&page, "/", updateUI);
  server.begin();

  pinMode(LEDPIN, OUTPUT);
}

void updateUI() {
  // Enabled / disable the slider. Note that you could simply do this inside the loop. However,
  // placing it here makes the client UI more responsive (try it).
  blinkfreq.setEnabled(mode.selectedOption() == 1);
}

void loop() {
  // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
  driver.loopHook();

  // And these lines are all you have to write for the logic: Access the elements as if they were plain
  // local controls
  if (mode.selectedOption() == 1) { // blink
      digitalWrite(LEDPIN, (millis() / (1100 - blinkfreq.intValue())) % 2);
  } else {  // on or off
      digitalWrite(LEDPIN, mode.selectedOption() != 0);
  }
}
