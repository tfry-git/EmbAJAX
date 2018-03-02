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

class ArduJAXOutputDriverBase;
class ArduJAXBase {
public:
    virtual void print() const = 0;
    static void setDriver(ArduJAXOutputDriverBase *driver) {
        _driver = driver;
    }
    /** serialize pending changes for the client. Virtual so you could customize it, completely, but
     *  instead you probably want to override ArduJAXControllable::valueProperty(), only, instead.
     *
     *  @param since revision number last sent to the server. Send only changes that occured since this revision.
     *  @param first if false, @em and this object writes any update, it should write a ',', first.
     *  @returns true if anything has been written, false otherwise.
     */
    virtual bool sendUpdates(uint16_t since, bool first=true) {
        return false;
    }
protected:
    static ArduJAXOutputDriverBase *_driver;
};

/** Output driver as an abstraction over the server read/write commands.
 *  You will have to instantiate exactly one object of exactly one implementation,
 *  before using and ArduJAX classes. */
class ArduJAXOutputDriverBase {
public:
    ArduJAXOutputDriverBase() {
        _revision = 0;
        next_revision = _revision;
    }
    virtual void printHeader(bool html) = 0;
    virtual void printContent(const char *content) = 0;
    virtual String getArg(const char* name) = 0;
    uint16_t revision() const {
        return _revision;
    }
    uint16_t setChanged() {
        next_revision = _revision+1;
        return (next_revision);
    }
    void nextRevision() {
        _revision = next_revision;
    }
private:
    uint16_t _revision;
    uint16_t next_revision;
};

#if defined (ESP8266)
class ArduJAXOutputDriverESP8266 : public ArduJAXOutputDriverBase {
public:
    ArduJAXOutputDriverESP8266(ESP8266WebServer *server) {
        ArduJAXBase::setDriver(this);
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
    String getArg(const char* name) override {
        return _server->arg(name);
    };
private:
    ESP8266WebServer *_server;
};
#endif

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

/** Base class for groups of objects */
class ArduJAXContainer : public ArduJAXBase {
public:
    ArduJAXContainer(ArduJAXList children) {
        _children = children;
    };
    void printChildren() const {
        for (int i = 0; i < _children.count; ++i) {
            _children.members[i]->print();
        }
    }
    bool sendUpdates(uint16_t since, bool first=true) override {
        for (int i = 0; i < _children.count; ++i) {
            first = first & !_children.members[i]->sendUpdates(since, first);
        }
        return first;
    }
protected:
    ArduJAXList _children;
};

/** This class represents a chunk of static HTML that will not be changed / cannot be interacted with. Neither from the client, nor from the server. */
class ArduJAXStatic : public ArduJAXBase {
public:
    ArduJAXStatic(const char* content) {
        _content = content;
    }
    void print() const override {
        _driver->printContent(_content);
    }
protected:
    const char* _content;
};

/** Base class for objects that can be changed, either from the client, or from the server, or both. The base class implements an HTML "span" element.
 *  The inner content of that span can be changed using setValue(). */
class ArduJAXControllable : public ArduJAXBase {
public:
    ArduJAXControllable(const char* id) {
        _id = id;
        _flags = StatusVisible;
        _value = "";
        revision = 0;
    }
    void print() const override {
        _driver->printContent("<span id=\"");
        _driver->printContent(_id);
        _driver->printContent("\">");
        _driver->printContent(_value);
        _driver->printContent("</span>\n");
    }
    bool sendUpdates(uint16_t since, bool first=true) override {
        if (!changed(since)) return false;
         if (!first) _driver->printContent(",\n");
        _driver->printContent("{\n\"id\": \"");
        _driver->printContent(_id);
        _driver->printContent("\",\n\"changes\": [");
        for (int8_t i = -2; i < additionalPropertyCount(); ++i) {
            if (i != -2) _driver->printContent(",");
           _driver->printContent("{\n\"set\": \"");
           _driver->printContent(properyId(i));
           _driver->printContent("\",\n\"value\": \"");
           _driver->printContent(properyValue(i));
           _driver->printContent("\"\n}");
        }
        _driver->printContent("]\n}");
        setChanged();
        return true;
    }
    void setVisible(bool visible) {
        if (visible == _flags & StatusVisible) return;
        setChanged();
        if (visible) _flags |= StatusVisible;
        else _flags -= _flags & StatusVisible;
    }
    virtual void setValue(const char* new_value) {
        _value = new_value;
        setChanged();
    }
    const char* value() const {
        return _value;
    }
    virtual const char* valueProperty() const {
        return "innerHTML";
    }
    /** To allow addition of further properties to sync to the client, in derived classes */
    virtual uint8_t additionalPropertyCount() const {
        return 0;
    }
    /** To allow addition of further properties to sync to the client, in derived classes.
     *  NOTE: do call the base implementation when overriding: It handles the built-in properties
     *  with negative numbering; */
    virtual const char* properyId(int8_t num) const {
        if (num == -1) return (valueProperty());
        if (num == -2) return ("style.display");
        return "";
    }
    /** To allow addition of further properties to sync to the client, in derived classes.
     *  NOTE: do call the base implementation when overriding: It handles the built-in properties
     *  with negative numbering; */
    virtual const char* properyValue(int8_t num) const {
        if (num == -1) return (_value);
        if (num == -2) return (_flags & StatusVisible ? "initial" : "none");
        return "";
    }
protected:
friend class ArduJAXPage;
    const char* _id;
    const char* _value;
    void setChanged() {
        revision = _driver->setChanged();
    }
    bool changed(uint16_t since) {
        if ((revision + 20000) < since) revision = since + 1;    // basic overflow protection. Results in sending _all_ states at least every 20000 request cycles
        return (revision > since);
    }
    enum {
        StatusVisible = 1,
        StatusChanged = 2
    };
private:
    byte _flags;
    uint16_t revision;
};

class ArduJAXPage : public ArduJAXContainer {
public:
    ArduJAXPage(ArduJAXList children, const char* title) : ArduJAXContainer(children) {
        _title = title;
    }
    void print() const override {
        _driver->printHeader(true);
        _driver->printContent("<HTML><HEAD><TITLE>");
        _driver->printContent(_title);
        _driver->printContent("</TITLE>\n<SCRIPT>\n");
        _driver->printContent("var serverrevision = 0;\n"
                              "function doRequest(request='', id='', value='') {\n"
                              "    var req = new XMLHttpRequest();\n"
                              "    req.onload = function() {\n"
                              "       doUpdates(JSON.parse(req.responseText));\n"
                              "    }\n"
                              "    req.open('POST', document.URL, true);\n"
                              "    req.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');\n"
                              "    req.send('request=' + request + '&id=' + id + '&value=' + value + '&revision=' + serverrevision);\n"
                              "}\n");
        _driver->printContent("function doUpdates(response) {\n"
                              "    serverrevision = response.revision;\n"
                              "    var updates = response.updates;\n"
                              "    for(i = 0; i < updates.length; i++) {\n"
                              "       element = document.getElementById(updates[i].id);\n"
                              "       changes = updates[i].changes;\n"
                              "       for(j = 0; j < changes.length; ++j) {\n"
                              "          var spec = changes[j].set.split('.');\n"
                              "          var prop = element;\n"
                              "          for(k = 0; k < (spec.length-1); ++k) {\n"   // resolve nested attributes such as style.display
                              "              prop = prop[spec[k]];\n"
                              "          }\n"
                              "          prop[spec[spec.length-1]] = changes[j].value;\n"
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
        // ignore any parameters for now. TODO: actually handle the request other than poll
        _driver->printHeader(false);
        _driver->printContent("{\"revision\": ");
        String dummy(_driver->revision());
        _driver->printContent(dummy.c_str());
        _driver->printContent(",\n\"updates\": [\n");
        uint16_t client_revision = atoi(_driver->getArg("revision").c_str());
        sendUpdates(client_revision);
        _driver->printContent("\n]}\n");
        _driver->nextRevision();
    }
protected:
    const char* _title;
};
