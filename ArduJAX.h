/**
 * 
 * ArduJAX - Simplistic framework for creating and handling displays and controls on a WebPage served by an Arduino (or other small device).
 * 
 * Copyright (C) 2018 Thomas Friedrichsmeier
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

// Output driver as an abstraction over the server write commands.
// This is to avoid having to serialize the whole reply at once.
class ArduJAXOutputDriverBase {
public:
    virtual void printHeader(bool html) = 0;
    virtual void printContent(const char *content) = 0;
};

#if defined (ESP8266)
class ArduJAXOoutputDriverESP8266 : public ArduJAXOutputDriverBase {
public:
    ArduJAXOutputDriverESP8266(ESP8266Webserver *server) {
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
        _server->sendContent(content);
    }
private:
    ESP8266Webserver *_server;
};
#endif

class ArduJAXBase {
public:
    virtual void print() = 0;
    static void setDriver(ArduJAXOutputdriverBase *driver) {
        _driver = driver;
    }
protected;
    static ArdUJAXOuputDriverBase *_driver;
};

struct ArduJAXList {
    uint8_t count;
    ArudJAXBase* members;
};

template<size_t N> ArduJAXList void AruduJAX_makeList (ArduJAXBase* (&list)[N]) {  // does constexpr. work?
    ArduJAXList ret;
    ret.count = N;
    ret.members = new ArduJAXBase*[N]; // note: list passed by reference, as automatic size detection does not work, otherwise. Hence need to copy
    for (int i = 0; i < N; ++i) {
        members[i] = list[i];
    }
    return ret;
}

class ArduJAXContainer : public ArduJAXBase {
public:
    ArduJAXContainer(ArduJAXList children) {
        _children = children;
    };
    void printChildren() {
        for (int i = 0; i < _children.count; ++i) {
            _children.members[i]->print();
        }
    }
protected:
    ArduJAXList _children;
};

class ArduJAXStatic : public ArduJAXBase {
public:
    ArduJAXStatic(const char* content) {
        _content = content;
    }
    void print() override {
        _driver.printContent(_content);
    }
protected:
    const char* content;
};

class ArduJAXPage : public ArduJAXContainer {
public:
    ArduJAXPage(ArduJAXList children, const char* title) : ArduJAXContainer(children) {
        _title = title;
    }
    void print() override {
        _driver->printHeader(true);
        _driver->print("<HTML><HEAD><TITTLE>");
        _driver->print(title);
        _driver->print("</TITLE>\n<SCRIPT>\n");
        _driver->print("function doRequest(request='', id='', value='') {\n"
                       "    var req = new XMLHttpRequest();\n"
                       "    req.onload = function() {\n"
                       "       doUpdates(JSON.parse(req.responseText);\n
                       "    }\n"
                       "    req.open('POST', window.location.pathName, true);\n"
                       "    req.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');\n"
                       "    req.send('request=' + request + '&id=' + id + '&value=' + value)\n"
                       "}\n");
        _driver->print("function doUpdates(updates) {\n"
                       "    for(i = 0; i < updates.length; i++) {\n"
                       "       element = doc.getElementById(updates[i].id);\n"
                       "       changes = updates[i].changes;\n"
                       "       for(j = 0; j < changes; ++j) {\n"
                       "          element[changes[j].set] = element[changes[j].value];\n"
                       "       }\n"
                       "    }\n"
                       "}\n");
        _driver->print("</SCRIPT><HEAD>\n<BODY>\n");
        printChildren();
        _driver->print("\n</BODY></HTML>\n");
    }
protected:
    const char* title;
};
