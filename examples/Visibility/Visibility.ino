/* Basic usage example for Ajane library:
*
* This example shows the semantics of calling setVisible() and setEnabled() on the various
* elements.
* 
* This example is based on an ESP8266 with Arduino core (https://github.com/esp8266/Arduino).
* 
* This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ajane.h>

// Set up web server, and register it with Ajane
ESP8266WebServer server(80);
AjaneOutputDriverESP8266 driver(&server);

// The radio groups will be used to control the state
const char* visibility_opts[] = {"Normal", "Hidden", "Disabled"};
AjaneRadioGroup<3> radioa("radioa", visibility_opts);
AjaneRadioGroup<3> radiob("radiob", visibility_opts);
AjaneRadioGroup<3> radioc("radioc", visibility_opts);

// Some random fillers
AjaneStatic statics[] = {
    AjaneStatic("<p>Lorem ipsum dolor</p>"),
    AjaneStatic("<p>sit amet, consectetur adipiscing elit,</p>"),
    AjaneStatic("<p>sed do eiusmod tempor incididunt</p>"),
    AjaneStatic("<p>ut labore et dolore magna aliqua.</p>")
};

// Some input elements. We won't be doing anything with these, just show/hide/disable them.
AjaneCheckButton check("check", "Some option");
const char* radio_opts[] = {"Option1", "Option2", "Option3"};
AjaneRadioGroup<3> radio("radio", radio_opts);
AjaneOptionSelect<3> select("select", radio_opts);
AjaneSlider slider("slider", 0, 1000, 500);
AjaneTextInput<30> text("text");
void buttonPressed(AjanePushButton*) { }
AjanePushButton button("button", "I can count", buttonPressed);

AjaneBase* container1_contents[] = {&statics[0], &check, &statics[1], &select, &statics[2], &slider, &statics[3]};
AjaneContainer<7> container1(container1_contents);

AjaneBase* container2_contents[] = {&statics[0], &button, &statics[1], &text, &statics[2], &radio, &statics[3]};
AjaneHideableContainer<7> container2("hideable", container2_contents);

// Define the page
MAKE_AjanePage(page, "Ajane example - Visibility", "",
    new AjaneStatic("<h1>AjaneContainer</h1><p>This radio group causes setEnabled()/setVisible()"
                      "to be called on an AjaneContainer-instance containing the elements below \"Elements\".</p><h2>Status</h2>\n"),
    &radioa,
    new AjaneStatic("<h2>Elements</h2>"),
    &container1,
    new AjaneStatic("<h1>AjaneHideableContainer</h1><p>This radio group causes setEnabled()/setVisible()"
                      "to be called on an AjaneHideableContainer-instance containing the elements below \"Elements\".</p><h2>Status</h2>\n"),
    &radiob,
    new AjaneStatic("<h2>Elements</h2>"),
    &container2,
    new AjaneStatic("<h1>Radio option control</h1><p>This radio group causes setEnabled()/setVisible()"
                      "to be called \"Option2\" of the radio control, above.</p>\n"),
    &radioc
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
    WiFi.softAP("AjaneTest", "12345678");

    // Tell the server to serve our Ajane test page on root
    server.on("/", handlePage);
    server.begin();

    updateUI(); // init displays
}

void updateUI() {
    container1.setVisible(radioa.selectedOption() != 1);
    container1.setEnabled(radioa.selectedOption() != 2);
    container2.setVisible(radiob.selectedOption() != 1);
    container2.setEnabled(radiob.selectedOption() != 2);
    container2.setVisible(radiob.selectedOption() != 1);
    container2.setEnabled(radiob.selectedOption() != 2);
    radio.button(1)->setVisible(radioc.selectedOption() != 1);
    radio.button(1)->setEnabled(radioc.selectedOption() != 2);
}

void loop() {
    // handle network
    server.handleClient();
}
