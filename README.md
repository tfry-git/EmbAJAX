# EmbAJAX

Simplistic framework for creating and handling displays and controls on a web page served by an embeddable device (Arduino or other
microcontroller with Arduino support).

## Documentation

- For an overview, getting started, troubleshooting basic problem: **Continue reading on this page**
- More usage examples: The regular [examples folder](/examples)
- All functions / classes: [full API reference](https://tfry-git.github.io/EmbAJAX/api/annotated.html)
- Tweaking RAM, flash, and network usage, understanding internal workings: [Technical details](/docs/Technical.md)

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
version. I'll try not to break things _except_ when there is a good reason to. Be aware that this "promise" applies to "released"
versions, only, however. Inside the git master branch, breaking changes may happen more frequently.

For details on what has been changed, refer to the [ChangeLog](/docs/ChangeLog.md).

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

Currently EmbAJAX will work, without additional configuration using WiFi on ESP8266, ESP32, and Raspberry Pi Pico.

For the general approach on using EmbAJAX with different hardware, see the Blink_Ethernet example. This relies on the EthernetWebServer library,
which supports a large number of different boards, including ATMEGA 2560, Teensy, etc. In a similar fashion, it should be possible to utilize the
WiFiWebServer library for boards that do not include native WiFi (this latter claim has not currently been tested).

Should your hardware need more custom tweaking, or you wish to use a different webserver library, drivers are really easy to add.
All that is needed is a very basic abstraction across some web server calls.

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

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDriverWebServerClass is a
// convenience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

// Define the main elements of interest as variables, so we can access them later in our sketch.
EmbAJAXSlider slider("slider", 0, 500, 400);      // slider, from 0 to 500, initial value 400
EmbAJAXMutableSpan display("display");            // a plain text display

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

EmbAJAX is available from the (Arduino Library Manager)[https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library].

If, instead, you want to use the bleeding edge version of EmbAJAX, the suggested routine is to _first_ install from the Library Manager, anyway, then
_replace_ the contents of the installation folder with either a ```git clone``` of this repository, or _the contents_ of an unpackaged ZIP:
https://github.com/tfry-git/EmbAJAX/archive/master.zip . The important thing is that the library installation folder itself _must_ be named "EmbAJAX",
while - quite unfortunately - in the ZIP file, the folder is called "EmbAJAX-master".

## A word on security

At present, EmbAJAX does not incorporate any security mechanisms. Anybody who can connect to the server will be able to view and
set any control. So any security will have to be implemented on the network level.

One easy way to do so, that will suit many simple use cases - and is used in the examples - is to simply set up your device as a WiFi access point,
with encryption and password protection. With this, any client in range can connect, _if_ they have the credentials for the access point.

If you need remote connections, currently your best bet will be to use an nginx proxy server (see e.g. https://jjssoftware.github.io/secure-your-esp8266/).

Future versions of EmbAJAX will provide a basic authentication and permission system, but to provide any meaningful level of security, this will
mean communication with your device will have to be encrypted. One exciting news in this regard is that an HTTPS server implementation is about to be
added to the ESP8266 arduino core. So check back soon (or submit your pull request)!

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
