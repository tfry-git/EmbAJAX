/* Basic usage example for ArduJAX library:
* This simply creates a page with one instance of each input element available at the time
* of this writing. The right hand side has displays to show some sort of value for each (after a
* round-trip to the server).
* 
* This example is based on an ESP8266 with Arduino core (https://github.com/esp8266/Arduino).
* 
* This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduJAX.h>

// Set up web server, and register it with ArduJAX
ESP8266WebServer server(80);
ArduJAXOutputDriverESP8266 driver(&server);

#define BUFLEN 30

// Define the main elements of interest as variables, so we can access to them later in our sketch.
ArduJAXCheckButton check("check", "Some option");
ArduJAXMutableSpan check_d("check_d");

const char* radio_opts[] = {"Option1", "Option2", "Option3"};
ArduJAXRadioGroup<3> radio("radio", radio_opts);
ArduJAXMutableSpan radio_d("radio_d");

ArduJAXSlider slider("slider", 0, 1000, 500);
ArduJAXMutableSpan slider_d("slider_d");
char slider_d_buf[BUFLEN];

ArduJAXTextInput<BUFLEN> text("text");  // Text input, width BUFLEN
ArduJAXMutableSpan text_d("text_d");
char text_d_buf[BUFLEN];

int button_count = 0;
void buttonPressed(ArduJAXPushButton*) { button_count++; }
ArduJAXPushButton button("button", "I can count", buttonPressed);
ArduJAXMutableSpan button_d("button_d");
char button_d_buf[BUFLEN];

ArduJAXStatic nextCell("</td><td>&nbsp;</td><td><b>");
ArduJAXStatic nextRow("</b></td></tr><tr><td>");

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_ArduJAXPage is just a convenience macro around the ArduJAXPage<>-class.
MAKE_ArduJAXPage(page, "ArduJAX example - Inputs", "",
    new ArduJAXStatic("<table cellpadding=\"10\"><tr><td>"),
    &check,
    &nextCell,  // Static elements can safely be inserted into the same page more than once!
    &check_d,
    &nextRow,
    &radio,
    &nextCell,
    &radio_d,
    &nextRow,
    &slider,
    &nextCell,
    &slider_d,
    &nextRow,
    &text,
    &nextCell,
    &text_d,
    &nextRow,
    &button,
    &nextCell,
    &button_d,
    new ArduJAXStatic("</b></td></tr></table>")
)

// This is all you need to write for the page handler
void handlePage() {
    if(server.method() == HTTP_POST) { // AJAX request
        page.handleRequest(updateUI);
    } else {  // Page load
        page.print();
    }
}

void setup() {
    // Example WIFI setup as an access point. Change this to whatever suits you, best.
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig (IPAddress (192,168,4,1), IPAddress (0,0,0,0), IPAddress (255,255,255,0));
    WiFi.softAP("ArduJAXTest", "12345678");

    // Tell the server to serve our ArduJAX test page on root
    server.on("/", handlePage);
    server.begin();

    updateUI(); // init displays
}

void updateUI() {
    // Update UI. Note that you could simply do this inside the loop. However,
    // placing it here makes the client UI more responsive (try it).
    check_d.setValue(check.isChecked() ? "checked" : "not checked");
    radio_d.setValue(radio_opts[radio.selectedOption()]);
    slider_d.setValue(itoa(slider.intValue(), slider_d_buf, 10));
    text_d.setValue(strncpy(text_d_buf, text.value(), BUFLEN));
    button_d.setValue(itoa(button_count, button_d_buf, 10));
}

void loop() {
    // handle network
    server.handleClient();
}
