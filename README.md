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
- Supports arbitrary number of pages, and elements can be shared across pages.

This framework _could_ be used independently of the Arduino environment, but Arduino is the main target, thus the C++ implementation,
and a focus on keeping things lean in memory.

## Status

The library is still pretty new, but quite functional, and stabilizing. This means you can start using the library right now.
However, please note, that there is _no_ guarantee that upcoming versions of this library remain 100% compatible with the current
version. I'll try not to break things _except_ when there is a good reason to.

### Supported elements / features

These controls / elements are supported as of now:

- Checkboxes
- Radio buttons (mutually exclusive buttons)
- Push buttons
- Sliders
- Text display
- Text input
- Static HTML blocks
- Connection status indicator

The following additional features may be of interest (supported as of now):

- Allows to insert your own custom CSS for styling (no styling in the lib).
- Elements can be hidden, inputs can be disabled from the server (ArduJAXBase::setVisible(), setEnabled()).

## Example sketch (compilable on ESP8266)

Not really useful, but you know what to really do with a slider, and a display, right?
Some further examples can be found in the examples folder.

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

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_ArduJAXPage is just a convenience macro around the ArduJAXPage<>-class.
MAKE_ArduJAXPage(page, "ArduJAXTest", "",
  new ArduJAXStatic("<h1>This is a test</h1><p>The value you set in the following slider: "),
  &slider,
  new ArduJAXStatic(" is sent to the server...</p><p>... which displays it here: <b>"),
  &display,
  new ArduJAXStatic("</b></p>")
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

  updateUI();  // initialize display
}

char buf[16];
void updateUI() {
  // And the following line is all that is needed to read the slider value, convert to a string, and send it
  // back to the client for display:
  display.setValue(itoa(slider.intValue(), buf, 10));

  // NOTE: Instead of in this separate function, you could also place the above line in loop(). However, having
  // it here allows the library to send back the updated display, immediately, resulting in a snappier UI on
  // the client.
}

void loop() {
  // handle network
  server.handleClient();
}

```

## Installation

For now the installation routine is:
- Download a ZIP of the current development version: https://github.com/tfry-git/ArduJAX/archive/master.zip
- In your Arduino-IDE, select Sketch->Include Library->Add .ZIP Library, then select the downloaded .zip for installation
- You may need to restart your IDE for the library an its examples to show up

## Further readings

API documentation is at https://tfry-git.github.io/ArduJAX/api/annotated.html .

## Some implementation notes

Currently, the web servers for embeddables I have dealt with so far, are limited to one client at a time. Therefore, if using
a permanent AJAX connection, all further access would be blocked. Even separate page loads from the same browser. So, instead,
we resort to regular polling for updates. An update poll is always included, automatically, when the client sends control
changes to the server, so in most cases, the client would still appear to be refreshed, immediately.

To avoid sending all states of all controls on each request from each client, the framework keeps track of the lastest "revision number"
sent to any client. The client pings back its current revision number on each request, so only real changes have to be forwarded.

You may have noted that the framework avoids the use of the String class, even though that would make some things easier. The reason
for this design choice is that the overhead of using char*, here, in a sketch that may be using String, already, is lot. However, if this
framework were to rely on String, while nothing else in the sketch uses String, that would incur a significant overhead. Further, it should
be noted, that the risk of memory-fragmentation is relatively real in the present use-case, as arbitrary strings are regularly coming in
"from the outside". Nonetheless, using Strings would relieve the use from having to worry about the lifetime of strings passed in to the
framework, and thus, in the future, it may make sense to support String *optionally*.

Another thing you will notice is that the framework avoids any sort of dynamic list. Instead, template classes with a size parameter are
used to keep lists of elements (such as the ArduJAXPage\<SIZE>). The reason is again, memory efficiency, and fear of fragmentation. Also,
the vast majority of use cases should be perfectly fine with a statically defined setup of elements. However, should the need arise, it
would be very easily possible to create a dynamically allocated analogon to ArduJAXContainer<SIZE>. An instance of that could simply be
inserted into a page, and serve as a straight-forward wrapper around elements that are created dynamically. (A different question is how to
keep this in sync with the client, of course, if that is also a requirement...)

## Some TODOs

- Cannot ever have enough controls: drop-down (\<select>), div (esp. to show/hide static elements in a group)
- More drivers
- API docs
- More examples
- In the case of fragile, but not totally broken connections, we may want to re-send change notifications on failure
  - This would mean keeping a map of "id->value in need of syncing" in the client.
  - Then whenever sending a new request (e.g. when polling), that list would be checked, first, for anything in need of attention.
  - Perhaps also limit things to one pending request at a time?

## The beggar's line

So far everbody else has been rolling their own piece of AJAX for their own little project, and that's what I could have done, too, in a tiny
fraction of the time spent on this framework, including its docs. But I chose to write all this, instead. Because I believe in re-usable solutions
and shared efforts. If you do too, and this framework is useful to you, consider dropping a micro-donation via PayPal to thomas.friedrichsmeier@gmx.de .
Thanks!
