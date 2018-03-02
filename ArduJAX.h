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
#ifndef ARDUJAX_H
#define ARDUJAX_H

#include <Arduino.h>

#define ARDUJAX_MAX_ID_LEN 16

class ArduJAXOutputDriverBase;
class ArduJAXElement;
class ArduJAXContainer;

class ArduJAXBase {
public:
    virtual void print() const = 0;
    static void setDriver(ArduJAXOutputDriverBase *driver) {
        _driver = driver;
    }
    /** serialize pending changes for the client. Virtual so you could customize it, completely, but
     *  instead you probably want to override ArduJAXElement::valueProperty(), only, instead.
     *
     *  @param since revision number last sent to the server. Send only changes that occured since this revision.
     *  @param first if false, @em and this object writes any update, it should write a ',', first.
     *  @returns true if anything has been written, false otherwise.
     */
    virtual bool sendUpdates(uint16_t since, bool first=true) {
        return false;
    }
    /** Cast this object to ArduJAXElement if it is a controllable element.
     *  @return 0, if this is not a controllable element. */
    virtual ArduJAXElement* toElement() {
        return 0;
    }
    /** Cast this object to ArduJAXContainer if it is a container.
     *  @return 0, if this is not a container. */
    virtual ArduJAXContainer* toContainer() {
        return 0;
    }
protected:
    static ArduJAXOutputDriverBase *_driver;
    static char itoa_buf[8];
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
    virtual const char* getArg(const char* name, char* buf, int buflen) = 0;
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

#if defined (ESP8266)  // TODO: Move this to extra header
#include <ESP8266WebServer.h>
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
    const char* getArg(const char* name, char* buf, int buflen) override {
        _server->arg(name).toCharArray (buf, buflen);
        return buf;
    };
private:
    ESP8266WebServer *_server;
};
#endif

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
    /** Recursively look for a child (hopefully, there is only one) of the given id, and return a pointer to it. */
    ArduJAXElement* findChild(const char*id) const;
    ArduJAXContainer *toContainer() override {
        return this;
    }
protected:
    ArduJAXList _children;
};

/** This class represents a chunk of static HTML that will not be changed / cannot be interacted with. Neither from the client, nor from the server.
 *  This does not have to correspond to a complete HTML element, it can be any fragment. */
class ArduJAXStatic : public ArduJAXBase {
public:
    /** ctor. Note: Content string is not copied. Don't make this a temporary. */
    ArduJAXStatic(const char* content) {
        _content = content;
    }
    void print() const override {
        _driver->printContent(_content);
    }
protected:
    const char* _content;
};

/** Abstract Base class for objects that can be changed, either from the server, or from both the client and the server both.
 *  To create a derived class, you will need to provide appropriate implementations of print(), value(), and valueProperty().
 *  Further, you will most likely want to add a function like setValue() for control from the server. If the element is to
 *  receive updates from the client side, you will a) have to include an appropriate onChange-call in print(), and b) provide
 *  a non-empty implementation of updateFromDriverArg().
 *
 *  Best look at a simple example such as ArduJAXActiveSpan or ArduJAXSlider for details.
 */
class ArduJAXElement : public ArduJAXBase {
public:
    /** @param id: The id for the element. Note that the string is not copied. Do not use a temporary string in this place. Also, do keep it short. */
    ArduJAXElement(const char* id);

    const char* id() const {
        return _id;
    }
    bool sendUpdates(uint16_t since, bool first=true) override;
    void setVisible(bool visible);

    /** const char representation of the current server side value. Must be implemented in derived class.
     *  Note: deliberately not const, to allow for non-const conversion and caching. */
    virtual const char* value() = 0;

    /** override this in your derived class to allow updates to be propagated from client to server (if wanted).
     *  The implementation should _not_ call setChanged(). */
    virtual void updateFromDriverArg(const char* argname) {
        return;
    }

    /** The JS property that will have to be set on the client */
    virtual const char* valueProperty() const = 0;

    /** To allow addition of further properties to sync to the client, in derived classes */
    virtual uint8_t additionalPropertyCount() const {
        return 0;
    }

    /** To allow addition of further properties to sync to the client, in derived classes.
     *  NOTE: do call the base implementation when overriding: It handles the built-in properties
     *  with negative numbering; */
    virtual const char* propertyId(int8_t num) const {
        if (num == -1) return (valueProperty());
        if (num == -2) return ("style.display");
        return "";
    }

    /** To allow addition of further properties to sync to the client, in derived classes.
     *  NOTE: do call the base implementation when overriding: It handles the built-in properties
     *  with negative numbering; */
    virtual const char* propertyValue(int8_t num) {
        if (num == -1) return (value());
        if (num == -2) return (_flags & StatusVisible ? "initial" : "none");
        return "";
    }
    ArduJAXElement *toElement() override {
        return this;
    }
protected:
friend class ArduJAXPage;
    const char* _id;
    void setChanged();
    bool changed(uint16_t since);
    enum {
        StatusVisible = 1,
        StatusChanged = 2
    };
private:
    byte _flags;
    uint16_t revision;
};

/** An HTML span element with content that can be updated from the server (not the client) */
class ArduJAXMutableSpan : public ArduJAXElement {
public:
    ArduJAXMutableSpan(const char* id) : ArduJAXElement(id) {
        _value = 0;
    }
    void print() const override;
    const char* value() override {
        return _value;
    }
    const char* valueProperty() const override {
        return "innerHTML";
    }
    /** Set the <span>s content to the given value. Note: The string is not copied, so don't make this a temporary. */
    void setValue(const char* value) {
        _value = value;
        setChanged();
    }
private:
    const char* _value;
};

/** An HTML span element with content that can be updated from the server (not the client) */
class ArduJAXSlider : public ArduJAXElement {
public:
    ArduJAXSlider(const char* id, int16_t min, int16_t max, int16_t initial) : ArduJAXElement(id) {
        _value = initial;
        _min = min;
        _max = max;
    }
    void print() const override;
    const char* value() override;
    const char* valueProperty() const override {
        return "value";
    }
    void setValue(int16_t value) {
        _value = value;
        setChanged();
    }
    void updateFromDriverArg(const char* argname) override;
private:
    int16_t _min, _max, _value;
};

/** This is the main interface class. Create a web-page with a list of elements on it, and arrange for
 *  print() (for page loads) adn handleRequest() (for AJAX calls) to be called on requests. By default,
 *  both page loads, and AJAX are handled on the same URL, but the first via GET, and the second
 *  via POST. */
class ArduJAXPage : public ArduJAXContainer {
public:
    /** Create a web page.
     *  @param children list of elements on the page
     *  @param title title (may be 0). This string is not copied, please do not use a temporary string.
     *  @param header_add literal text (may be 0) to be added to the header, e.g. CSS (linked or in-line). This string is not copied, please do not use a temporary string). */
    ArduJAXPage(ArduJAXList children, const char* title, const char* header_add = 0) : ArduJAXContainer(children) {
        _title = title;
        _header_add = 0;
    }
    void print() const override {
        _driver->printHeader(true);
        _driver->printContent("<HTML><HEAD><TITLE>");
        if (_title) _driver->printContent(_title);
        _driver->printContent("</TITLE>\n<SCRIPT>\n");
        _driver->printContent("var serverrevision = 0;\n"
                              "function doRequest(id='', value='') {\n"
                              "    var req = new XMLHttpRequest();\n"
                              "    req.onload = function() {\n"
                              "       doUpdates(JSON.parse(req.responseText));\n"
                              "    }\n"
                              "    req.open('POST', document.URL, true);\n"
                              "    req.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');\n"
                              "    req.send('id=' + id + '&value=' + encodeURIComponent(value) + '&revision=' + serverrevision);\n"
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
                              "    doRequest();\n"  // poll == request without id
                              "}\n"
                              "setInterval(doPoll,1000);\n");
        _driver->printContent("</SCRIPT>\n");
        if (_header_add) _driver->printContent(_header_add);
        _driver->printContent("</HEAD>\n<BODY>\n");
        printChildren();
        _driver->printContent("\n</BODY></HTML>\n");
    }
    /** Handle AJAX client request. You should arrange for this function to be called, whenever there is a POST request
     *  to whichever URL you served the page itself, from. */
    void handleRequest() {
        char conversion_buf[ARDUJAX_MAX_ID_LEN];

        // handle value changes sent from client
        uint16_t client_revision = atoi(_driver->getArg("revision", conversion_buf, ARDUJAX_MAX_ID_LEN));
        const char *id = _driver->getArg("id", conversion_buf, ARDUJAX_MAX_ID_LEN);
        ArduJAXElement *element = (id[0] == '\0') ? 0 : findChild(id);
        if (element) {
            element->updateFromDriverArg("value");
        }

        // then relay value changes that have occured in the server (possibly in response to those sent)
        _driver->printHeader(false);
        _driver->printContent("{\"revision\": ");
        _driver->printContent(itoa(_driver->revision(), conversion_buf, 10));
        _driver->printContent(",\n\"updates\": [\n");
        sendUpdates(client_revision);
        _driver->printContent("\n]}\n");

        if (element) {
            element->setChanged(); // So changes sent from one client will be synced to the other clients
        }
        _driver->nextRevision();
    }
protected:
    const char* _title;
    const char* _header_add;
};

#endif
