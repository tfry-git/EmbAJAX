/* Basic usage example for EmbAJAX library:
 * Provide a web interface to set built-in LED on, off, or blinking.
 * 
 * Note that ESP boards seems to be no real standard on which pin the builtin LED is on, and
 * there is a very real chance that LED_BUILTIN is not defined, correctly, for your board.
 * If you see no blinking, try changing the LEDPIN define (with an externally connected LED
 * on a known pin being the safest option). Similarly, on and off states are sometimes reversed.
 * 
 * This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <EmbAJAX.h>

#define LEDPIN LED_BUILTIN

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDirverWebServerClass is a
// converience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

// Define the main elements of interest as variables, so we can access to them later in our sketch.
const char* modes[] = {"On", "Blink", "Off"};
EmbAJAXRadioGroup<3> mode("mode", modes);
EmbAJAXSlider blinkfreq("blfreq", 0, 1000, 100);   // slider, from 0 to 500, initial value 400

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
  // Example WIFI setup as an access point. Change this to whatever suits you, best.
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig (IPAddress (192,168,4,1), IPAddress (0,0,0,0), IPAddress (255,255,255,0));
  WiFi.softAP("EmbAJAXTest", "12345678");

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
