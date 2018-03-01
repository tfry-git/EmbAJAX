# ArduJAX

Simplistic framework for creating and handling displays and controls on a WebPage served by an Arduino (or other small device).

## Overview

In the Arduino world, it has become astonishingly easy to create a web server on a microprocessor. But at the same time there is a
surprising lack of libraries to facilitat common tasks such as displaying some meter readings in textual or numeric displays, and
keeping those up-to-date on the client, or providing basic HTML controls such as sliders, radio buttons, and pushbuttons to control
the operation of the microprocessor.

While many, many projects are doing exactly this, each seems to have to implement the wheel anew. The goal of the present project is
to provide a basic but extensible framework to facilitate these common tasks, so that you can focus on the actual functionality.

I.e. the core features are:
- Ability to "print" common controls to an HTML page 
- Automatic addition of AJAX code to relay control information from the client to the server and back
- Automatic handling of the AJAX requests on the server side
- Object-based representation of the controls on the web page. The programmer can simply interact with local objects, while the
  framework shall take care of keeping information in sync with the client.

This framework _could_ be used independtly of the Arduino environment, but Arduino is the main target, thus the C++ implementation,
and a focus on keeping things lean in memory.

## Status

Early drafting stage. Nothing real to be seen here, yet.

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
// [...]  // usual web server setup
  server.on("/", handleRoot);  // set handler
  ArduJAXBase::setDriver(new ArduJAXOoutputDriverESP8266(&server));
}

void loop() {
  digitalWrite(some_pin, slider1.numericValue());
  display1.setValue(digitalRead(another_pin) ? "Status OK" : "Status Fail");
}

```

## And a less exciting, but somewhat convoluted show off of a test-case that actually works right now (on ESP8266)

```
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduJAX.h>

ESP8266WebServer server(80); //Server on port 80

ArduJAXControllable tester("millis");
ArduJAXBase* elements[] = {
  new ArduJAXStatic("<h1>This is a test</h1>"),
  &tester
};
ArduJAXPage page(ArduJAX_makeList(elements), "ArduJAXTest");
ArduJAXOutputDriverESP8266 driver(&server);

void handleRoot() {
  page.print();
}

void handleReq() {
  page.handleRequest();
}

void setup() {

// Please fill in: Set up your WIFI connection
[...]

  page.setDriver(&driver);
  server.on("/", handleRoot);
  server.on("/", HTTP_POST, handleRoot);
  server.on("/ardujax", handleReq);
  server.begin();
}

String dummy;
void loop() {
  server.handleClient();
  dummy = String(millis ());
  tester.setValue (dummy.c_str ());
}
```
