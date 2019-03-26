/* Basic usage example for EmbAJAX library:
*
* This example shows the semantics of calling setVisible() and setEnabled() on the various
* elements.
* 
* This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <EmbAJAX.h>

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDirverWebServerClass is a
// converience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

// The radio groups will be used to control the state
const char* visibility_opts[] = {"Normal", "Hidden", "Disabled"};
EmbAJAXRadioGroup<3> radioa("radioa", visibility_opts);
EmbAJAXRadioGroup<3> radiob("radiob", visibility_opts);
EmbAJAXRadioGroup<3> radioc("radioc", visibility_opts);

// Some random fillers
EmbAJAXStatic statics[] = {
    EmbAJAXStatic("<p>Lorem ipsum dolor</p>"),
    EmbAJAXStatic("<p>sit amet, consectetur adipiscing elit,</p>"),
    EmbAJAXStatic("<p>sed do eiusmod tempor incididunt</p>"),
    EmbAJAXStatic("<p>ut labore et dolore magna aliqua.</p>")
};

// Some input elements. We won't be doing anything with these, just show/hide/disable them.
EmbAJAXCheckButton check("check", "Some option");
const char* radio_opts[] = {"Option1", "Option2", "Option3"};
EmbAJAXRadioGroup<3> radio("radio", radio_opts);
EmbAJAXOptionSelect<3> optionselect("optionselect", radio_opts);
EmbAJAXSlider slider("slider", 0, 1000, 500);
EmbAJAXTextInput<30> text("text");
void buttonPressed(EmbAJAXPushButton*) { }
EmbAJAXPushButton button("button", "I can count", buttonPressed);

EmbAJAXBase* container1_contents[] = {&statics[0], &check, &statics[1], &optionselect, &statics[2], &slider, &statics[3]};
EmbAJAXContainer<7> container1(container1_contents);

EmbAJAXBase* container2_contents[] = {&statics[0], &button, &statics[1], &text, &statics[2], &radio, &statics[3]};
EmbAJAXHideableContainer<7> container2("hideable", container2_contents);

// Define the page
MAKE_EmbAJAXPage(page, "EmbAJAX example - Visibility", "",
    new EmbAJAXStatic("<h1>EmbAJAXContainer</h1><p>This radio group causes setEnabled()/setVisible()"
                      "to be called on an EmbAJAXContainer-instance containing the elements below \"Elements\".</p><h2>Status</h2>\n"),
    &radioa,
    new EmbAJAXStatic("<h2>Elements</h2>"),
    &container1,
    new EmbAJAXStatic("<h1>EmbAJAXHideableContainer</h1><p>This radio group causes setEnabled()/setVisible()"
                      "to be called on an EmbAJAXHideableContainer-instance containing the elements below \"Elements\".</p><h2>Status</h2>\n"),
    &radiob,
    new EmbAJAXStatic("<h2>Elements</h2>"),
    &container2,
    new EmbAJAXStatic("<h1>Radio option control</h1><p>This radio group causes setEnabled()/setVisible()"
                      "to be called \"Option2\" of the radio control, above.</p>\n"),
    &radioc
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
    // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
    driver.loopHook();
}
