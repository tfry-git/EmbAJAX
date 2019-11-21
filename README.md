# EmbAJAX

Simplistic framework for creating and handling displays and controls on a web page served by an embeddable device (Arduino or other
microcontroller with Arduino support).

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
- Directional input ("joystick")
- Static HTML blocks
- Connection status indicator

The following additional features may be of interest (supported as of now):

- Allows to insert your own custom CSS for styling (no styling in the lib).
  - See the "Styling"-example for techniques that can be used to define styling
- Elements can be hidden, inputs can be disabled from the server (EmbAJAXBase::setVisible(), setEnabled()).

### Hardware support

Currently there are output drivers for ESP8266 and ESP32. However, drivers are really easy to add. All that is needed is a very
basic abstraction across some web server calls.

#### ESP32 quirks and workaround

Unfortunately, ESP32-support is currently plagued by a bug in the ESP32's networking code, that causes incoming connections
to fail under some circumstances (https://github.com/espressif/arduino-esp32/issues/1921). If you are using ESP32, and EmbAJAX
feels sluggish, displays do not update, or the status indicator sometimes turns red, despite good network strength, you are probably
being hit by this bug.

You can work around this by using the ESPAsyncWebServer library (https://github.com/me-no-dev/ESPAsyncWebServer). To do so,
(after installing the lib), you will need to add:

```cpp
#include <AsyncTCP.h>
#include <EmbAJAXOutputDriverESPAsync.h>
```

**above** ```#include <EmbAJAX.h>``` in your EmbAJAX sketches. No further adjustments are needed in the examples, provided, here.

## Example sketch

Not really useful, but you know what to really do with a slider, and a display, right?
Some further examples can be found in the examples folder.

```cpp
#include <EmbAJAX.h>

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDirverWebServerClass is a
// converience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

// Define the main elements of interest as variables, so we can access them later in our sketch.
EmbAJAXSlider slider("slider", 0, 500, 400);   // slider, from 0 to 500, initial value 400
EmbAJAXMutableSpan display("display");         // a plain text display

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "EmbAJAXTest", "",
  new EmbAJAXStatic("<h1>This is a test</h1><p>The value you set in the following slider: "),
  &slider,
  new EmbAJAXStatic(" is sent to the server...</p><p>... which displays it here: <b>"),
  &display,
  new EmbAJAXStatic("</b></p>")
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
    // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.
    driver.loopHook();
}

```

## Installation

For now the installation routine is:
- Download a ZIP of the current development version: https://github.com/tfry-git/EmbAJAX/archive/master.zip
- In your Arduino-IDE, select Sketch->Include Library->Add .ZIP Library, then select the downloaded .zip for installation
- You may need to restart your IDE for the library and its examples to show up

## Further readings

API documentation is at https://tfry-git.github.io/EmbAJAX/api/annotated.html .

## A word on security

At present, EmbAJAX does not incorporate any security mechanisms. Anybody who can connect to the server will be able to view and
set any control. So any security will have to be implemented on the network level.

One easy way to do so, that will suit many simple use cases - and is used in the examples - is to simply set up your device as a WiFi access point,
with encryption and password protection. With this, any client in range can connect, _if_ they have the credentials for the access point.

If you need remote connections, currently your best bet will be to use an nginx proxy server (see e.g. https://jjssoftware.github.io/secure-your-esp8266/).

Future versions of EmbAJAX will provide a basic authentication and permission system, but to provide any meaningful level of security, this will
mean communication with your device will have to be encrypted. One exciting news in this regard is that an HTTPS server implementation is about to be
added to the ESP8266 arduino core. So check back soon (or submit your pull request)!

## Some implementation notes

Currently, the web servers for embeddables I have dealt with so far, are limited to one client at a time. Therefore, if using
a permanent connection, all further access would be blocked. Even separate page loads from the same browser. So, instead,
we resort to regular polling for updates. An update poll is always included, automatically, when the client sends control
changes to the server, so in most cases, the client would still appear to be refreshed, immediately.

To avoid sending all states of all controls on each request from each client, the framework keeps track of the latest "revision number"
sent to any client. The client pings back its current revision number on each request, so only real changes have to be forwarded.

Concurrent access by an arbitrary number of separate clients is the main reason behind going with AJAX, instead of WebSockets, even if the
latter are often described as more "modern". Note that the purpoted drawback to AJAX - latency - can easily be circumventented for most use
cases, as desribed, above. Still, it would be relatively easy to generalize the framework to also allow a WebSocket-connection. I'm not
doing this, ATM, for fear of adding unneccessary complexity for little or no practical gain.

You may have noted that the framework avoids the use of the String class, even though that would make some things easier. The reason
for this design choice is that the overhead of using char*, here, in a sketch that may be using String, already, is low. However, if this
framework were to rely on String, while nothing else in the sketch uses String, that would incur a significant overhead. Further, it should
be noted, that the risk of memory-fragmentation is relatively real in the present use-case, as arbitrary strings are regularly coming in
"from the outside". Nonetheless, using Strings would relieve the use from having to worry about the lifetime of strings passed in to the
framework, and thus, in the future, it may make sense to support String *optionally*.

Another thing you will notice is that the framework avoids any sort of dynamic list. Instead, template classes with a size parameter are
used to keep lists of elements (such as the EmbAJAXPage\<SIZE>). The reason is again, memory efficiency, and fear of fragmentation. Also,
the vast majority of use cases should be perfectly fine with a statically defined setup of elements. However, should the need arise, it
would be very easily possible to create a dynamically allocated analogon to EmbAJAXContainer<SIZE>. An instance of that could simply be
inserted into a page, and serve as a straight-forward wrapper around elements that are created dynamically. (A different question is how to
keep this in sync with the client, of course, if that is also a requirement...)

Connection error handling, despite asynchronous requests: Both server and client keep track of the "revision" number of their state. This is
used for keeping several clients in sync, but also for error handling: If the server detects that it has a lower revision than the client, it
will know that it has rebooted (while the client has not), and will re-send all current states. If the client tries to send a UI change, but
the network request fails, it will discard its revision, and thereby ask the server to also re-send all states. Thus, the latest user input
may get lost on a network error, but the state of the controls shown in the client will remain in sync with the state as known to the server.

## Some TODOs

- More controls:
  - One nice to have idea: A sort of "touch-screen", reporting clicks, and releases with exact position, continuous reporting of
    mouse position (optional: always, never, while button pressed).
    - Bonus points: With position marker ("cursor"), with ability to load arbitrary background image.
  - Toggle buttons (trivial to implement on top of pushbuttons)
- More drivers
- More examples
- Basic authentication / permission features
  - Will need to rely on encrypted communication. I.e. HTTPS server implementation.

## The beggar's line

So far everbody else has been rolling their own piece of AJAX for their own little project, and that's what I could have done, too, in a tiny
fraction of the time spent on this framework, including its docs. But I chose to write all this, instead. Because I believe in re-usable solutions
and shared efforts. If you do too, and this framework is useful to you, consider dropping a micro-donation via PayPal to thomas.friedrichsmeier@gmx.de .
Thanks!
