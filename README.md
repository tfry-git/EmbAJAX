# Ajane

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
- Simple but effective error handling for unreliable connections and server reboots

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
- Drop down option select
- RGB Color picker
- Static HTML blocks
- Connection status indicator

The following additional features may be of interest (supported as of now):

- Allows to insert your own custom CSS for styling (no styling in the lib).
- Elements can be hidden, inputs can be disabled from the server (AjaneBase::setVisible(), setEnabled()).

## Example sketch (compilable on ESP8266)

Not really useful, but you know what to really do with a slider, and a display, right?
Some further examples can be found in the examples folder.

```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ajane.h>

// Set up web server, and register it with Ajane
ESP8266WebServer server(80);
AjaneOutputDriverESP8266 driver(&server);

// Define the main elements of interest as variables, so we can access them later in our sketch.
AjaneSlider slider("slider", 0, 500, 400);   // slider, from 0 to 500, initial value 400
AjaneMutableSpan display("display");         // a plain text display

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_AjanePage is just a convenience macro around the AjanePage<>-class.
MAKE_AjanePage(page, "AjaneTest", "",
  new AjaneStatic("<h1>This is a test</h1><p>The value you set in the following slider: "),
  &slider,
  new AjaneStatic(" is sent to the server...</p><p>... which displays it here: <b>"),
  &display,
  new AjaneStatic("</b></p>")
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
- Download a ZIP of the current development version: https://github.com/tfry-git/Ajane/archive/master.zip
- In your Arduino-IDE, select Sketch->Include Library->Add .ZIP Library, then select the downloaded .zip for installation
- You may need to restart your IDE for the library and its examples to show up

## Further readings

API documentation is at https://tfry-git.github.io/Ajane/api/annotated.html .

## Some implementation notes

Currently, the web servers for embeddables I have dealt with so far, are limited to one client at a time. Therefore, if using
a permanent AJAX connection, all further access would be blocked. Even separate page loads from the same browser. So, instead,
we resort to regular polling for updates. An update poll is always included, automatically, when the client sends control
changes to the server, so in most cases, the client would still appear to be refreshed, immediately.

To avoid sending all states of all controls on each request from each client, the framework keeps track of the latest "revision number"
sent to any client. The client pings back its current revision number on each request, so only real changes have to be forwarded.

You may have noted that the framework avoids the use of the String class, even though that would make some things easier. The reason
for this design choice is that the overhead of using char*, here, in a sketch that may be using String, already, is low. However, if this
framework were to rely on String, while nothing else in the sketch uses String, that would incur a significant overhead. Further, it should
be noted, that the risk of memory-fragmentation is relatively real in the present use-case, as arbitrary strings are regularly coming in
"from the outside". Nonetheless, using Strings would relieve the use from having to worry about the lifetime of strings passed in to the
framework, and thus, in the future, it may make sense to support String *optionally*.

Another thing you will notice is that the framework avoids any sort of dynamic list. Instead, template classes with a size parameter are
used to keep lists of elements (such as the AjanePage\<SIZE>). The reason is again, memory efficiency, and fear of fragmentation. Also,
the vast majority of use cases should be perfectly fine with a statically defined setup of elements. However, should the need arise, it
would be very easily possible to create a dynamically allocated analogon to AjaneContainer<SIZE>. An instance of that could simply be
inserted into a page, and serve as a straight-forward wrapper around elements that are created dynamically. (A different question is how to
keep this in sync with the client, of course, if that is also a requirement...)

Connection error handling, despite asynchronous requests: Both server and client keep track of the "revision" number of their state. This is
used for keeping several clients in sync, but also for error handling: If the server detects that it has a lower revision than the client, it
will know that it has rebooted (while the client has not), and will re-send all current states. If the client tries to send a UI change, but
the network request fails, it will discard its revision, and thereby ask the server to also re-send all states. Thus, the latest user input
may get lost on a network error, but the state of the controls shown in the client will remain in sync with the state as known to the server.

## Some TODOs

- More controls:
  - One nice-to-have feature would be a button-variant used for steering applications (think: an RC car). Instead of click, this would
    have to report "pressed" and "released". Importantly, it would have to include a mechanism to cope with connection errors. Probably, while
    pressed, it would have to send frequent "pings", and if the server-side does not receive such a ping after - say - 500ms, it will asuume
    the button _may_ have been released (tri-state, so the client can select the best safe behavior in this state of uncertainty).
  - As an even more sophisticated idea: A sort of "touch-screen", reporting clicks, and releases with exact position, continuous reporting of
    mouse position (optional: always, never, while button pressed).
    - Bonus points: With position marker ("cursor"), with ability to load arbitrary background image.
- More drivers
- More examples
- Basic authentication features
  - For now, Ajane makes no attempt at implementing any security features. If you need security, you will have to implement it at the network
    level. However, it would be nice, if there was at least a rudimentary mechanism besides this, so there can be differential permissions
    on the same network (e.g. view vs. control), and controlled access beyond a single subnet. Making a go at this makes relatively little sense
    with plain text communication, of course. One exciting news in this regard is that an HTTPS server implementation is about to be added to the
    ESP8266 arduino core. Once that is a bit more accessible, this should be revisited.

## What does the name Ajane stand for?

Ajane stands for "Asynchronous JAvascript Now/Neatly Embedded", and is the preliminary result of trying to come up with a name that does not clash with
various trademarks and existing library names. This framework was originally called "ArduJAX", but the Arduino tradmark holders (very politely) asked me to
try to find a different name.

## The beggar's line

So far everbody else has been rolling their own piece of AJAX for their own little project, and that's what I could have done, too, in a tiny
fraction of the time spent on this framework, including its docs. But I chose to write all this, instead. Because I believe in re-usable solutions
and shared efforts. If you do too, and this framework is useful to you, consider dropping a micro-donation via PayPal to thomas.friedrichsmeier@gmx.de .
Thanks!
