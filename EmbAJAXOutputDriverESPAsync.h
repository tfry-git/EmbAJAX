/**
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

#ifndef EMBAJAXOUTPUTDRIVERESPASYNC_H
#define EMBAJAXOUTPUTDRIVERESPASYNC_H

#if defined (EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION)
#error Duplicate definition of output driver. Fix your include-directives.
#endif
#define EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION

// For EmbAJAXPage. Important to include after defining EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION
#include "EmbAJAX.h"

#include <ESPAsyncWebServer.h>

#if defined (ESP32)
#define EmbAJAXOutputDriverWebServerClass AsyncWebServer
#else
#define EmbAJAXOutputDriverWebServerClass ESPAsyncWebServer
#endif

/**  @brief Output driver implementation. This implementation works with ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer).
 *   
 *   To use this class, you will have to include EmbAJAXOutputDriverESPAsync.h *before* EmbAJAX.h
 */
class EmbAJAXOutputDriverESPAsync : public EmbAJAXOutputDriverBase {
public:
    /** To register an (ESP)AsyncWebServer with EmbAJAX, simply create a (global) instance of this class. 
        @param server pointer to the server. The class of this is usually an auto-detected sensible default for the platform, AsyncWebServer on ESP32, ESPAsyncWebServer on ESP8266. */
    EmbAJAXOutputDriverESPAsync(EmbAJAXOutputDriverWebServerClass *server) {
        EmbAJAXBase::setDriver(this);
        _server = server;
        _request = 0;
    }
    void printHeader(bool html) override {
        _response = _request->beginResponseStream(html ? "text/html" : "text/json");
    }
    void printContent(const char *content) override {
        _response->print(content);
    }
    const char* getArg(const char* name, char* buf, int buflen) override {
        _request->arg(name).toCharArray (buf, buflen);
        return buf;
    }
    void installPage(EmbAJAXPageBase *page, const char *path, void (*change_callback)()=0) override {
        _server->on(path, [=](AsyncWebServerRequest* request) {
             _request = request;
             _response = 0;
             if (_request->method() == HTTP_POST) {  // AJAX request
                 page->handleRequest(change_callback);
             } else {  // Page load
                 page->printPage();
             }
             _request->send(_response);
             _request = 0;
        });
    }
    void loopHook() override {};
private:
    EmbAJAXOutputDriverWebServerClass *_server;
    AsyncWebServerRequest *_request;
    AsyncResponseStream *_response;
};

typedef EmbAJAXOutputDriverESPAsync EmbAJAXOutputDriver;

#endif
