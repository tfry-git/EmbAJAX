# ArduJAX

Simplistic framework for creating and handling displays and controls on a WebPage served by an Arduino (or other small device).

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

This framework _could_ be used indepently of the Arduino environment, but Arduino is the main target, thus the C++ implementation,
and a focus on keeping things lean in memory.

## Status

A first crude example works (not the one directly below, but the one at the bottom). This means you are invited to start playing with this
library, _but_ many things will change, including in backwards-incompatible ways.

## Example mockup - Not (yet) compilable/working in this form - but see further below

```

ArduJAXSlider *slider1;
ArduJAXDisplay *display1;

page1 = ArduJAXPage(ArudJAX_makeList({
   new ArduJAXStatic("<h1>Hello world</h1><p>This is a static section of plain old HTML. Next up: A slider / range control</p>"),
   slider1 = new ArduJAXSlider("slider1_id", min, max, initial),
   new ArduJAXStatic("<h1>Hello world</h1><p>This is a static section of plain old HTML. Next up: A slider / range control</p>"),
   display1 = new ArduJAXDisplay("display1_id", "Some intial value")
}));
// Note ArduJAX-Objects could be allocated on the stack, too, but that would require more tedious writing, or some really fancy
// macros. As ArduJAX objects will probably be alive for the entire runtime, heap fragmentation should not be an issue.

void handlePage() {
  if(server.method() == HTTP_POST) { // AJAX request
    page1.handleRequest();
  } else {  // Page load
    page1.print();
  }
}

void setup() {
// Please fill in: Set up your WIFI connection
[...]

  new ArduJAXOoutputDriverESP8266(&server);
  server.on("/", handlePage);  // set handler
}

void loop() {
  digitalWrite(some_pin, slider1.numericValue());
  display1.setValue(digitalRead(another_pin) ? "Status OK" : "Status Fail");
}

```

## And a less exciting off of a test-case that actually works right now (on ESP8266)

```
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduJAX.h>

ESP8266WebServer server(80); //Server on port 80
ArduJAXOutputDriverESP8266 driver(&server);

ArduJAXControllable tester("millis");
ArduJAXControllable tester2("blinky");
ArduJAXBase* elements[] = {
  new ArduJAXStatic("<h1>This is a test</h1><p>"),
  &tester,
  new ArduJAXStatic("</p><p>"),
  &tester2
};
ArduJAXPage page(ArduJAX_makeList(elements), "ArduJAXTest");

void handlePage() {
  if(server.method() == HTTP_POST) { // AJAX request
    page.handleRequest();
  } else {  // Page load
    page.print();
  }
}

void setup() {

// Please fill in: Set up your WIFI connection
[...]

  server.on("/", handlePage);
  server.begin();

  tester2.setValue("The server makes me blink");
}

String dummy;
void loop() {
  server.handleClient();
  dummy = String(millis ());
  tester.setValue (dummy.c_str ());
  tester2.setVisible((millis() / 2000) % 2);
}
```

## Some implementation notes

Currently, the web servers for embeddables I have deal with so far, are limited to one client at a time. Therefore, if using
a permanent AJAX connection, all further access would be blocked. Even separate page loads from the same browser. So, instead,
we resort to regular polling for updates. An update poll is always included, automatically, when the client sends control
changes to the server, so in most cases, the client would still appear to be refreshed, immediately.

To avoid sending all states of all controls on each request from each client, the framework keeps track of the lastest "revision number"
sent to any client. The client pings back its current revision number on each request, so only real changes have to be forwarded.
