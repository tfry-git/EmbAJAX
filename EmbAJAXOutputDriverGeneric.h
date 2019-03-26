/**
 * 
 * EmbAJAX - Simplistic framework for creating and handling displays and controls on a WebPage served by an Arduino (or other small device).
 * 
 * Copyright (C) 2018-2019 Thomas Friedrichsmeier
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
**/

#ifndef EMBAJAXOUTPUTDRIVERGENERIC_H
#define EMBAJAXOUTPUTDRIVERGENERIC_H

#if defined (EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION)
#error Duplicate definition of output driver. Fix your include-directives.
#endif
#define EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION

#if not defined EmbAJAXOutputDriverWebServerClass
#error Please define EmbAJAXOutputDriverWebServerClass or #include appropriate hardware specific driver
#endif

// For EmbAJAXPage. Important to include after defining EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION
#include "EmbAJAX.h"

/**  @brief Output driver implementation. This implementation should work for most arduino web servers with minimal adjustmnets. */
class EmbAJAXOutputDriverGeneric : public EmbAJAXOutputDriverBase {
public:
    /** To register an WebServer with EmbAJAX, simply create a (globaL) instance of this class. 
        @param server pointer to the server. The class of this is usually an auto-detected sensible default for the platform, e.g. ESP8266WebServer on ESP8266. */
    EmbAJAXOutputDriverGeneric(EmbAJAXOutputDriverWebServerClass *server) {
        EmbAJAXBase::setDriver(this);
        _server = server;
    }
    void printHeader(bool html) override {
        _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
        if (html) {
            _server->send(200, "text/html", "");
        } else {
            _server->send(200, "text/json", "");
        }
    }
    void printContent(const char *content) override {
        if (content[0] != '\0') _server->sendContent(content);  // NOTE: There seems to be a bug in the ESP8266 server when sending empty string.
    }
    const char* getArg(const char* name, char* buf, int buflen) override {
        _server->arg(name).toCharArray (buf, buflen);
        return buf;
    }
    void installPage(EmbAJAXPageBase *page, const char *path, void (*change_callback)()=0) override {
        _server->on(path, [=]() {
             if (_server->method() == HTTP_POST) {  // AJAX request
                 page->handleRequest(change_callback);
             } else {  // Page load
                 page->printPage();
             }
        });
    }
    void loopHook() override {
        _server->handleClient();
    };
private:
    EmbAJAXOutputDriverWebServerClass *_server;
};

typedef EmbAJAXOutputDriverGeneric EmbAJAXOutputDriver;

#endif
