# ArduJAX

Simplistic framework for creating and handling displays and controls on a web page served by an Arduino (or other small device).

## Overview

In the Arduino world, it has become astonishingly easy to create a web server on a microprocessor. But at the same time there is a
surprising lack of libraries to facilitate common tasks such as displaying some meter readings in textual or numeric displays, and
keeping those up-to-date on the client, or providing basic HTML controls such as sliders, radio buttons, and pushbuttons to control
the operation of the microprocessor.

While many, many projects are doing exactly this, each seems to have to implement the wheel anew. The goal of the present project is
to provide a basic but extensible framework to take care of these common tasks, so that you can focus on the actual functionality.

I.e. the core features are:
- Ability to "print" common controls to an HTML page 
- Automatic addition of AJAX code to relay control information from the client to the server and back
- Automatic handling of the AJAX requests on the server side
- Object-based representation of the controls on the web page. The programmer can simply interact with local objects, while the
  framework takes care of keeping information in sync with the client.
- Allows multiple clients to interact with the same page, concurrently.
  - You can even pass information between two clients this way: Try loading the example, below, in two separate browsers!

This framework _could_ be used independently of the Arduino environment, but Arduino is the main target, thus the C++ implementation,
and a focus on keeping things lean in memory.

## Status

A first crude example works (see below). This means you are invited to start playing with this
library, _but_ many things will change, including in backwards-incompatible ways.

### Supported elements / features

These controls / elements are supported as of now:

- Checkboxes
- Radio buttons (mutually exclusive buttons)
- Sliders
- Text display
- Static HTML blocks

The following additional features may be of interest (supported as of now):

- Allows to insert your own custom CSS for styling (no styling in the lib).
- Elements can be hidden, inputs can be disabled from the server.

## Example sketch (compilable on ESP8266)

Not terribly useful, but you know what to really do with a slider, and a display, right?

```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduJAX.h>

// Set up web server, and register it with ArduJAX
ESP8266WebServer server(80);
ArduJAXOutputDriverESP8266 driver(&server);

// Define the main elements of interest as variables, so we can access to them later in our sketch.
ArduJAXSlider slider("slider", 0, 500, 400);   // slider, from 0 to 500, initial value 400
ArduJAXMutableSpan display("display");         // a plain text display
ArduJAXMutableSpan blinky("blinky");           // another plain text display that we will show/hide from the server

// Define a page "page" with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_ArduJAXPage is just a convenience macro around the ArduJAXPage<>-class.
MAKE_ArduJAXPage(page, "ArduJAXTest", "",
  new ArduJAXStatic("<h1>This is a test</h1><p>The value you set in the following slider: "),
  &slider,
  new ArduJAXStatic(" is sent to the server...</p><p>... which displays it here: <b>"),
  &display,
  new ArduJAXStatic("</b></p><p>And here's a totally useless element showing and hiding based on server time: "),
  &blinky
)

// This is all you need to write for the page handler
void handlePage() {
  if(server.method() == HTTP_POST) { // AJAX request
    page.handleRequest();
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

  // Just a dummy text. You could change this at any time during loop, too!
  blinky.setValue("Server makes me blink");
}

void loop() {
  // handle network
  server.handleClient();

  // And these two lines are all you have to write for the logic: Read slider value, write it to display,
  // and toggle the blinky every three seconds.
  display.setValue (slider.value());
  blinky.setVisible((millis() / 3000) % 2);
}

```

## Some implementation notes

Currently, the web servers for embeddables I have deal with so far, are limited to one client at a time. Therefore, if using
a permanent AJAX connection, all further access would be blocked. Even separate page loads from the same browser. So, instead,
we resort to regular polling for updates. An update poll is always included, automatically, when the client sends control
changes to the server, so in most cases, the client would still appear to be refreshed, immediately.

To avoid sending all states of all controls on each request from each client, the framework keeps track of the lastest "revision number"
sent to any client. The client pings back its current revision number on each request, so only real changes have to be forwarded.

- Explain why avoidance of String, and future directions
- Explain reason for updateFromDriverArg

## Some TODOs

- More controls (obviously), importantly pushbuttons, and text input
- Ability to register callback(s) on value changes, which will allow for cleaner code, and immediate reaction

## The beggar's line

So far everbody else has been rolling their own piece of AJAX for their own little project, and that's what I could have done, too, in a
tenth of the time spent on this framework, including its docs. But I chose to write all this, instead. Because I believe in re-usable solutions
and shared efforts. If you do too, and this framework is useful to you, consider dropping a micro-donation via PayPal to thomas.friedrichsmeier@gmx.de .
Thanks!
