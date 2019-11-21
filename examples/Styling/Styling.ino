/* CSS styling example for EmbAJAX library:
 * Show various methods of applying CSS styles to elements.
 *
 * This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <EmbAJAX.h>

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDirverWebServerClass is a
// converience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

void handleButton(EmbAJAXPushButton *button) {
  // this is just a dummy, we don't want to do anything on this page
}

//EmbAJAXBase* div_contents[3];

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "EmbAJAX example - Blink",
  // BEGIN: This is the actual CSS styling. For simplicitly reasons, we're inlining the CSS into the main page, here.
  //        Note that you could also link to an external stylesheet (served from SD card, or from the internet) like this:
  //        <link rel=\"stylesheet\" href=\"styles.css\">
  "<style>"
    "button {color: red;}\n"                                          // <-- Style all buttons as red
    "#my_button2 {width: 10em; height: 10em; border-radius: 50%;}\n"  // <-- Style the button with id "my_button2 as circular
    "#my_div {background-color: black; color: white;}\n"         // <-- Style the div with id "my_div" as white on black
    "#my_div button {color: blue;}\n"                                 // <-- Style all buttons within that div as blue
  "</style>",
  // END:   CSS styling
  new EmbAJAXStatic("<h1>Various ways to style elements</h1><p>Note that the point of this page is not to show pretty or cool styling (you'll have to do that yourself), but the available techniques to add CSS-styling to elements.</p><p>See the example source for details</p>"),
  new EmbAJAXPushButton("my_button1", "Nothing", handleButton),
  new EmbAJAXPushButton("my_button2", "Round", handleButton),
  new EmbAJAXPushButton("my_button3", "Nothing", handleButton),
  new EmbAJAXHideableContainer<3> ("my_div", new EmbAJAXBase*[3] {
    new EmbAJAXStatic("<p>White on black</p>"),
    new EmbAJAXPushButton("my_button4", "Nothing", handleButton),
    new EmbAJAXPushButton("my_button5", "Nothing", handleButton),
  })
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
  // This page does nothing
}

void loop() {
  // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
  driver.loopHook();
}
