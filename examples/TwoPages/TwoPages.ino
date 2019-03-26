/* Basic usage example for EmbAJAX library:
 * Demonstrate using two pages with shared controls
 * 
 * This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <EmbAJAX.h>

#define LEDPIN LED_BUILTIN

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDirverWebServerClass is a
// converience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

// Define the main elements of interest as variables, so we can access to them later in our sketch.
EmbAJAXSlider shared_slider("shared_slider", 0, 1000, 100);
EmbAJAXSlider other_slider("other_slider", 0, 1000, 100);
EmbAJAXMutableSpan other_slider_display("other_slider_display");
char other_slider_display_buf[8];

// Define two very simple pages "page1" and "page2"
MAKE_EmbAJAXPage(page1, "EmbAJAX example - Two Pages I", "",
  new EmbAJAXStatic("<h1>Page 1</h1><p>This slider is shared across pages: "),
  &shared_slider,
  new EmbAJAXStatic("</p><p>This slider is not shared, but its value is shown on page 2: "),
  &other_slider,
  new EmbAJAXStatic("</p><p>Link to <a href=\"/page2\">page 2</a> (you can open this directly, or in a tab of your browser).</p>")
)

MAKE_EmbAJAXPage(page2, "EmbAJAX example - Two Pages II", "",
  new EmbAJAXStatic("<h1>Page 2</h1><p>This slider is shared across pages: "),
  &shared_slider,
  new EmbAJAXStatic("</p><p>This display shows the value of the bottom slider on page 1: "),
  &other_slider_display,
  new EmbAJAXStatic("</p><p>Link to <a href=\"/\">page 1</a> (you can open this directly, or in a tab of your browser).</p>")
)

void setup() {
  // Example WIFI setup as an access point. Change this to whatever suits you, best.
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig (IPAddress (192,168,4,1), IPAddress (0,0,0,0), IPAddress (255,255,255,0));
  WiFi.softAP("EmbAJAXTest", "12345678");

  // Tell the server to serve the two pages at root, and at "/page2", respectively.
  // installPage() abstracts over the (trivial but not uniform) WebServer-specific instructions to do so
  driver.installPage(&page1, "/", updateUI);
  driver.installPage(&page2, "/page2", updateUI);
  server.begin();

  updateUI(); // to initialize display
}

void updateUI() {
  // Note that we do _not_ have to update the shared slider on the other page. It is a single object to EmbAJAX, and thus
  // is automatically in sync between the pages. So the only thing to do, here, is to send the other_slider value to the display.
  other_slider_display.setValue(itoa(other_slider.intValue(), other_slider_display_buf, 10));
}

void loop() {
  // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
  driver.loopHook();
}
