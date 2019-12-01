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

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "EmbAJAX example - Styling",
  // BEGIN: This is the actual CSS styling.
  "<!-- For simplicitly reasons, we're inlining the CSS into the main page, here.\n"
  "Note that you could also link to an external stylesheet (served from SD card, or from the internet) like this:\n"
  "<link rel=\"stylesheet\" href=\"styles.css\">  -->\n"
  "<style>\n"
    "button {color: red;}                                         /* <-- Style all buttons as red */\n"
    "#my_button2 {width: 10em; height: 10em; border-radius: 50%;} /* <-- Style the button with id \"my_button2\" as circular */\n"
    "#my_div {background-color: black; color: white;}             /* <-- Style the div with id \"my_div\" as white on black */\n"
    "#my_div button {color: blue;}                                /* <-- Style all buttons within that div as blue */\n"
    "\n"
    "/* Checkbox styling. The surounding <span> has class \"checkbox\", so we can use that for selecting. Using flexbox layout to move the box to the right of the label */\n"
    ".checkbox { display: flex; align-items: center; }\n"
    ".checkbox label { order: 1; text-align: right; width: 20em; margin-right: 1em; }\n"
    "/* Technique of the box heavily inspired by Mik Ted's excellent post, here: https://www.inserthtml.com/2012/06/custom-form-radio-checkbox/ */\n"
    ".checkbox input[type=\"checkbox\"] {\n"
    "   order: 2;\n"
    "   -webkit-appearance: none;\n"
    "   appearance: none;\n"
    "   background-color: #fafafa;\n"
    "   border: 2px solid #aaaace;\n"
    "   border-radius: 0.3em;\n"
    "   width: 2em;\n"
    "   height: 2em;\n"
    "   position: relative;\n"
    "}\n"
    ".checkbox input[type=\"checkbox\"]:checked {\n"
    "   /* Applying a different background to the checked box, just in case the :after-spec, below does not work for some reason */\n"
    "   background-color: #99bb99\n"
    "}\n"
    ".checkbox input[type=\"checkbox\"]:checked:after {\n"
    "   content: '\\2714';\n"
    "   font-size: 2.3em;\n"
    "   position: absolute;\n"
    "   top: -0.4em;\n"
    "   left: 0.2em;\n"
    "   color: #33bb33;\n"
    "}\n"
    "\n"
    "/* Slider styling. Rotating appears to be the easiest way to make the slider vertical, cross-browser. */\n"
    "input[type=\"range\"] {\n"
    "   transform: rotate(90deg);\n"
    "   width: 5em;\n                     /* <-- Note: width becomes height, after rotating */\n"
    "   transform-origin: bottom left;\n"
    "   margin-bottom: 5em;               /* <-- Fix bounding box after rotating */\n"
    "}\n"
    "\n"
    "/* Radio styling. The surounding <span> of each option has class \"radio\", so we can use that for selecting. */\n"
    ".radio {\n"
    "  display: block\n"
    "}\n"
    ".radio:hover {\n"
    "   background-color: #bbbbbb;\n"
    "}\n"
    "input[type=\"radio\"]:checked ~ label {\n"
    "   font-weight: bold;\n"
    "}\n"
  "</style>",
  // END:   CSS styling
  new EmbAJAXStatic("<h1>Various ways to style elements</h1><p>Note that the point of this page is not to show pretty or cool styling (you'll have to do that yourself), but the available techniques to add CSS-styling to elements. The controls do not actually do anything in this example.</p><p>See the page source (and / or program source) for details.</p><p>BTW, if you create a smooth CSS scheme for EmbAJAX, consider sharing it!</p><h2>Various buttons</h2>"),
  new EmbAJAXPushButton("my_button1", "Nothing", handleButton),
  new EmbAJAXPushButton("my_button2", "Round", handleButton),
  new EmbAJAXPushButton("my_button3", "Nothing", handleButton),
  new EmbAJAXHideableContainer<3> ("my_div", new EmbAJAXBase*[3] {
    new EmbAJAXStatic("<p>White on black</p>"),
    new EmbAJAXPushButton("my_button4", "Nothing", handleButton),
    new EmbAJAXPushButton("my_button5", "Nothing", handleButton),
  }),
  new EmbAJAXStatic("<h2>Styling examples for other elements</h2>"),
  new EmbAJAXCheckButton("my_checkbox", "Checkbox control with custom box moved to the right-hand side"),
  new EmbAJAXStatic("<p>A short slider oriented vertically</p>"),
  new EmbAJAXSlider("my_slider", 0, 1000, 500),
  new EmbAJAXRadioGroup<5>("my_radio", new const char*[5] {"The currently selected option", "of this radio control", "will be shown in bold.", "Hovering an option", "highlights its background."})
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
