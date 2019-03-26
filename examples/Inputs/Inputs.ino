/* Basic usage example for EmbAJAX library:
* This simply creates a page with one instance of each input element available at the time
* of this writing. The right hand side has displays to show some sort of value for each (after a
* round-trip to the server).
* 
* This example code is in the public domain (CONTRARY TO THE LIBRARY ITSELF). */

#include <EmbAJAX.h>
#include <EmbAJAXValidatingTextInput.h> // Fancier text input in a separate header file

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDirverWebServerClass is a
// converience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

#define BUFLEN 30

// Define the main elements of interest as variables, so we can access to them later in our sketch.
EmbAJAXCheckButton check("check", "Some option");
EmbAJAXMutableSpan check_d("check_d");

const char* radio_opts[] = {"Option1", "Option2", "Option3"};
EmbAJAXRadioGroup<3> radio("radio", radio_opts);
EmbAJAXMutableSpan radio_d("radio_d");

EmbAJAXOptionSelect<3> optionselect("optionselect", radio_opts);
EmbAJAXMutableSpan optionselect_d("optionselect_d");

EmbAJAXSlider slider("slider", 0, 1000, 500);
EmbAJAXMutableSpan slider_d("slider_d");
char slider_d_buf[BUFLEN];

EmbAJAXColorPicker color("color", 0, 255, 255);
EmbAJAXMutableSpan color_d("color_d");
char color_d_buf[BUFLEN];

EmbAJAXTextInput<BUFLEN> text("text");  // Text input, width BUFLEN
EmbAJAXMutableSpan text_d("text_d");
char text_d_buf[BUFLEN];

EmbAJAXValidatingTextInput<16> valtext("valtext");
EmbAJAXMutableSpan valtext_d("valtext_d");
char valtext_d_buf[BUFLEN];

int button_count = 0;
void buttonPressed(EmbAJAXPushButton*) { button_count++; }
EmbAJAXPushButton button("button", "I can count", buttonPressed);
EmbAJAXMutableSpan button_d("button_d");
char button_d_buf[BUFLEN];

EmbAJAXMomentaryButton m_button("m_button", "Press and hold");
EmbAJAXMutableSpan m_button_d("m_button_d");

EmbAJAXStatic nextCell("</td><td>&nbsp;</td><td><b>");
EmbAJAXStatic nextRow("</b></td></tr><tr><td>");

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "EmbAJAX example - Inputs", "",
    new EmbAJAXStatic("<table cellpadding=\"10\"><tr><td>"),
    &check,
    &nextCell,  // Static elements can safely be inserted into the same page more than once!
    &check_d,
    &nextRow,

    &radio,
    &nextCell,
    &radio_d,
    &nextRow,

    &optionselect,
    &nextCell,
    &optionselect_d,
    &nextRow,

    &slider,
    &nextCell,
    &slider_d,
    &nextRow,

    &color,
    &nextCell,
    &color_d,
    &nextRow,

    &text,
    &nextCell,
    &text_d,
    &nextRow,

    &valtext,
    &nextCell,
    &valtext_d,
    &nextRow,

    &button,
    &nextCell,
    &button_d,
    &nextRow,

    &m_button,
    &nextCell,
    &m_button_d,
    &nextRow,

    new EmbAJAXStatic("Server status:"),
    &nextCell,
    new EmbAJAXConnectionIndicator(),
    new EmbAJAXStatic("</b></td></tr></table>")
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

    valtext.setPlaceholder("192.168.1.1");
    valtext.setPattern("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");

    updateUI(); // init displays
}

void updateUI() {
    // Update UI. Note that you could simply do this inside the loop. However,
    // placing it here makes the client UI more responsive (try it).
    check_d.setValue(check.isChecked() ? "checked" : "not checked");
    radio_d.setValue(radio_opts[radio.selectedOption()]);
    optionselect_d.setValue(radio_opts[optionselect.selectedOption()]);
    slider_d.setValue(itoa(slider.intValue(), slider_d_buf, 10));
    color_d.setValue(strncpy(color_d_buf, color.value(), BUFLEN));  // r, g, b, are also available, numerically.
    text_d.setValue(strncpy(text_d_buf, text.value(), BUFLEN));
    valtext_d.setValue(strncpy(valtext_d_buf, valtext.value(), BUFLEN));
    button_d.setValue(itoa(button_count, button_d_buf, 10));
    m_button_d.setValue((m_button.status() == EmbAJAXMomentaryButton::Pressed) ? "pressed" : "not pressed");
}

void loop() {
    // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
    driver.loopHook();
}
