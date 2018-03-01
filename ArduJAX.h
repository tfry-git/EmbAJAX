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
class ArduJAXOutputDriverESP8266 : public ArduJAXOutputDriverBase {
public:
    ArduJAXOutputDriverESP8266(ESP8266WebServer *server) {
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
    const char* getArg (const char* name);
private:
    ESP8266WebServer *_server;
};
#endif

class ArduJAXBase {
public:
    virtual void print() = 0;
    static void setDriver(ArduJAXOutputDriverBase *driver) {
        _driver = driver;
    }
    virtual bool sendUpdates() {
        return false;
    }
protected:
    static ArduJAXOutputDriverBase *_driver;
};
ArduJAXOutputDriverBase *ArduJAXBase::_driver;  // TODO: terrible HACK

struct ArduJAXList {
    uint8_t count;
    ArduJAXBase** members;
};

template<size_t N> ArduJAXList ArduJAX_makeList (ArduJAXBase* (&list)[N]) {  // does constexpr. work?
    ArduJAXList ret;
    ret.count = N;
    ret.members = new ArduJAXBase*[N]; // note: list passed by reference, as automatic size detection does not work, otherwise. Hence need to copy
    for (int i = 0; i < N; ++i) {
        ret.members[i] = list[i];
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
    bool sendUpdates() override {
        bool sent = false;
        for (int i = 0; i < _children.count; ++i) {
            if (sent) _driver->printContent(",\n");
            sent = _children.members[i]->sendUpdates();
        }
        return sent;
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
        _driver->printContent(_content);
    }
protected:
    const char* _content;
};

class ArduJAXControllable : public ArduJAXBase {
public:
    ArduJAXControllable(const char* id) {
        _id = id;
        changed = false;
        _value = "";
    }
    void print() override {
        _driver->printContent("<div id=\"");
        _driver->printContent(_id);
        _driver->printContent("\">");
        _driver->printContent(_value);
        _driver->printContent("</div>\n");
    }
    virtual bool sendUpdates()  {
        if (!changed) return false;
        _driver->printContent("{\n\"id\": \"");
        _driver->printContent(_id);
        _driver->printContent("\",\n\"changes\": [{\n\"value\": \"");
        _driver->printContent(_value);
        _driver->printContent("\",\n\"set\": \"innerHTML\"\n}]\n}");
        changed = false;
        return true;
    }
    void setValue(const char* new_value) {
        _value = new_value;
        changed = true;
    }
protected:
friend class ArduJAXPage;
    const char* _id;
    const char* _value;
    bool changed;  // Needs syncing to client? TODO: actually this would have to be per client, but for now we focus on the single client use-case
};

class ArduJAXPage : public ArduJAXContainer {
public:
    ArduJAXPage(ArduJAXList children, const char* title) : ArduJAXContainer(children) {
        _title = title;
    }
    void print() override {
        _driver->printHeader(true);
        _driver->printContent("<HTML><HEAD><TITLE>");
        _driver->printContent(_title);
        _driver->printContent("</TITLE>\n<SCRIPT>\n");
        _driver->printContent("function doRequest(request='', id='', value='') {\n"
                              "    var req = new XMLHttpRequest();\n"
                              "    req.onload = function() {\n"
                              "       doUpdates(JSON.parse(req.responseText));\n"
                              "    }\n"
                              "    req.open('POST', 'ardujax', true);\n"
                              "    req.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');\n"
                              "    req.send('request=' + request + '&id=' + id + '&value=' + value);\n"
                              "}\n");
        _driver->printContent("function doUpdates(updates) {\n"
                              "    for(i = 0; i < updates.length; i++) {\n"
                              "       element = document.getElementById(updates[i].id);\n"
                              "       changes = updates[i].changes;\n"
                              "       for(j = 0; j < changes.length; ++j) {\n"
                              "          element[changes[j].set] = changes[j].value;\n"
                              "       }\n"
                              "    }\n"
                              "}\n");
        _driver->printContent("function doPoll() {\n"
                              "    doRequest('poll');\n"
                              "}\n"
                              "setInterval(doPoll,1000);\n");
        _driver->printContent("</SCRIPT></HEAD>\n<BODY>\n");
        printChildren();
        _driver->printContent("\n</BODY></HTML>\n");
    }
    void handleRequest() {
        // ignore any parameters for now. TODO: acutally handle the request
        _driver->printHeader(false);
        _driver->printContent("[\n");
        sendUpdates();
        _driver->printContent("\n]\n");
    }
protected:
    const char* _title;
};
