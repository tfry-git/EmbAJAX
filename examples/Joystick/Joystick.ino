/* Basic usage example for EmbAJAX library:
 * Demonstrate directional control using EmbAJAXJoystick.
 * 
 * Note that ESP boards seems to be no real standard on which pin the builtin LED is on, and
 * there is a very real chance that LED_BUILTIN is not defined, correctly, for your board.
 * If you see no blinking, try changing the LEDPIN define (with an externally connected LED
 * on a known pin being the safest option). Similarly, on and off states are sometimes reversed.
 * 
 * This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <EmbAJAX.h>
#include <EmbAJAXJoystick.h>

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDirverWebServerClass is a
// converience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

// Define the main elements of interest as variables, so we can access to them later in our sketch.
EmbAJAXJoystick joy1("joy1", 300, 300, 100, 2000, EmbAJAXJoystick_POSITION_9_DIRECTIONS);
EmbAJAXJoystick joy2("joy2", 300, 300);

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "EmbAJAX example - Joystick control", "<style>canvas { background-color: grey; }</style>",
  new EmbAJAXStatic("<h1>Joystick demonstration.</h1><p><b>Note that the EmbAJAXJoystick class is not entirely finished, yet, and the API and behavior might be subject to change!</b></p><p>The upper joystick is confined to the 9 \"keypad\" positions.</p>"),
  &joy1,
  new EmbAJAXStatic("<p>This lower joystick follows the position of the upper joystick (for no other purpose than to demonstrate a round-trip over the server). When moving it directly, it is not confined to the keypad positions.</p>"),
  &joy2
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
}

void updateUI() {
  // Make joystick two follow the position of joystick 1. There is not really a point to this, other than to demonstrate both receiving and sending position values.
  joy2.setPosition(joy1.getX(), joy1.getY());
}

void loop() {
  // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
  driver.loopHook();
}
