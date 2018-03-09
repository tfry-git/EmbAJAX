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

#include "ArduJAX.h"

#define ITOA_BUFLEN 8

// statics
ArduJAXOutputDriverBase *ArduJAXBase::_driver;
char ArduJAXBase::itoa_buf[ITOA_BUFLEN];

////////////////////////////// ArduJAXOutputDriverBase ////////////////////

void ArduJAXOutputDriverBase::printFiltered(const char* value, bool quoted, bool HTMLescaped) {
    // NOTE: The assumption, here is that frequent (char-by-char) calls to printContent() _could_ be expensive, depending on the server
    //       implementation. Thus, a buffer is used to enable printing in larger chunks.
    char buf[32];
    uint bufpos = 0;
    if (quoted) buf[bufpos++] = '"';
    const char *pos = value;
    while(*pos != '\0') {
        if (bufpos > 24) {  // == Not enough room for the worst case, i.e. "&amp;" + quote-end + terminating '\0'
            buf[bufpos] = '\0';
            printContent(buf);
            bufpos = 0;
            continue;
        }
        if (quoted && (*pos == '"' || *pos == '\\')) {
            buf[bufpos++] = '\\';
            buf[bufpos++] = *pos;
        } else if (HTMLescaped && (*pos == '<')) {
            buf[bufpos++] = '&';
            buf[bufpos++] = 'l';
            buf[bufpos++] = 't';
            buf[bufpos++] = ';';
        } else if (HTMLescaped && (*pos == '&')) {
            buf[bufpos++] = '&';
            buf[bufpos++] = 'a';
            buf[bufpos++] = 'm';
            buf[bufpos++] = 'p';
            buf[bufpos++] = ';';
        } else {
            buf[bufpos++] = *pos;
        }
        ++pos;
    }
    if (quoted) buf[bufpos++] = '"';
    buf[bufpos++] = '\0';
    printContent(buf);
}

//////////////////////// ArduJAXConnectionIndicator ///////////////////////

void ArduJAXConnectionIndicator::print() const {
    _driver->printContent("<div><script>\n"
                          "window.ardujaxsh = { 'div': document.scripts[document.scripts.length-1].parentNode,\n"
                          "'misses': 0,\n"
                          "'in': function() { if(this.misses) { this.misses = 0; this.div.innerHTML=");
    _driver->printQuoted(_content_ok ? _content_ok : "<span class=\"ArduJAXStatusOK\" style=\"background-color:green;\">OK</span>");
    _driver->printContent (";}},\n"
                           "'out': function() {if (this.misses < 5) { if(++(this.misses) >= 5) this.div.innerHTML=");
    _driver->printQuoted(_content_fail ? _content_fail : "<span class=\"ArduJAXStatusOK\" style=\"background-color:red;\">FAIL</span>");
    _driver->printContent(";}}\n"
                          "}\n</script></div>");
}

////////////////////////////// ArduJAXElement /////////////////////////////

/** @param id: The id for the element. Note that the string is not copied. Do not use a temporary string in this place. Also, do keep it short. */
ArduJAXElement::ArduJAXElement(const char* id) : ArduJAXBase() {
    _id = id;
    _flags = 1 << ArduJAXBase::Visibility | 1 << ArduJAXBase::Enabledness;
    revision = 1;
}

bool ArduJAXElement::sendUpdates(uint16_t since, bool first) {
    if (!changed(since)) return false;
    if (!first) _driver->printContent(",\n");
    _driver->printContent("{\n\"id\": ");
    _driver->printQuoted(_id);
    _driver->printContent(",\n\"changes\": [");
    uint8_t i = 0;
    while (true) {
        const char* pid = valueProperty(i);
        const char* pval = value(i);
        if (!pid || !pval) break;

        if (i != 0) _driver->printContent(",");
        _driver->printContent("[");
        _driver->printQuoted(pid);
        _driver->printContent(", ");
        _driver->printFiltered(pval, true, valueNeedsEscaping(i));
        _driver->printContent("]");

        ++i;
    }
    _driver->printContent("]\n}");
    return true;
}

void ArduJAXElement::setBasicProperty(uint8_t num, bool status) {
    uint8_t status_bit = 1 << num;
    if (status == (bool) (_flags & status_bit)) return;
    if (status) _flags |= status_bit;
    else _flags -= _flags & status_bit;
    setChanged();
}

void ArduJAXElement::setChanged() {
    revision = _driver->setChanged();
}

bool ArduJAXElement::changed(uint16_t since) {
    if ((revision + 40000) < since) revision = since + 1;    // basic overflow protection. Results in sending _all_ states at least every 40000 request cycles
    return (revision > since);
}

void ArduJAXElement::printTextInput(uint SIZE, const char* _value) const {
    _driver->printContent("<input id=");
    _driver->printQuoted(_id);
    _driver->printContent(" type=\"text\" maxLength=\"");
    _driver->printContent(itoa(SIZE-1, itoa_buf, 10));
    _driver->printContent("\" size=");
    _driver->printContent(itoa(min(max(abs(SIZE-1), 10),40), itoa_buf, 10));  // Arbitray limit for rendered width of text fields: 10..40 chars
    _driver->printContent("\" value=");
    _driver->printQuoted(_value);
    // Using onChange to update is too awkward. Using plain onInput would generate too may requests (and often result in "eaten" characters). Instead,
    // as a compromise, we arrange for an update one second after the last key was pressed.
    _driver->printContent(" onInput=\"var that=this; clearTimeout(that.debouncer); that.debouncer=setTimeout(function() {doRequest(that.id, that.value);},1000);\"/>");
}

//////////////////////// ArduJAXContainer ////////////////////////////////////

void ArduJAXBase::printChildren(ArduJAXBase** _children, uint NUM) const {
    for (uint i = 0; i < NUM; ++i) {
        _children[i]->print();
    }
}

bool ArduJAXBase::sendUpdates(ArduJAXBase** _children, uint NUM, uint16_t since, bool first) {
    for (uint i = 0; i < NUM; ++i) {
        bool sent = _children[i]->sendUpdates(since, first);
        if (sent) first = false;
    }
    return !first;
}

ArduJAXElement* ArduJAXBase::findChild(ArduJAXBase** _children, uint NUM, const char*id) const {
    for (uint i = 0; i < NUM; ++i) {
        ArduJAXElement* child = _children[i]->toElement();
        if (child) {
            if (strcmp(id, child->id()) == 0) return child;
        }
        child = _children[i]->findChild (id);
        if (child) return child;
    }
    return 0;
}

//////////////////////// ArduJAXMutableSpan /////////////////////////////

void ArduJAXMutableSpan::print() const {
    _driver->printContent("<span id=");
    _driver->printQuoted(_id);
    _driver->printContent(">");
    if (_value) _driver->printFiltered(_value, false, valueNeedsEscaping());
    _driver->printContent("</span>\n");
}

const char* ArduJAXMutableSpan::value(uint8_t which) const {
    if (which == ArduJAXBase::Value) return _value;
    return ArduJAXElement::value(which);
}

bool ArduJAXMutableSpan::valueNeedsEscaping(uint8_t which) const {
    if (which == ArduJAXBase::Value) return !basicProperty(ArduJAXBase::HTMLAllowed);
    return ArduJAXElement::valueNeedsEscaping(which);
}

const char* ArduJAXMutableSpan::valueProperty(uint8_t which) const {
    if (which == ArduJAXBase::Value) return "innerHTML";
    return ArduJAXElement::valueProperty(which);
}

void ArduJAXMutableSpan::setValue(const char* value, bool allowHTML) {
    // TODO: Ideally we'd special case setValue() with old value == new value (noop). However, since often both old and new values are kept in the same char buffer,
    // we cannot really compare the strings - without keeping a copy, at least. Should we?
    _value = value;
    setBasicProperty(ArduJAXBase::HTMLAllowed, allowHTML);
    setChanged();
}

//////////////////////// ArduJAXSlider /////////////////////////////

ArduJAXSlider::ArduJAXSlider(const char* id, int16_t min, int16_t max, int16_t initial) : ArduJAXElement(id) {
    _value = initial;
    _min = min;
    _max = max;
}

void ArduJAXSlider::print() const {
    _driver->printContent("<input id=");
    _driver->printQuoted(_id);
    _driver->printContent(" type=\"range\" min=\"");
    _driver->printContent(itoa(_min, itoa_buf, 10));
    _driver->printContent("\" max=\"");
    _driver->printContent(itoa(_max, itoa_buf, 10));
    _driver->printContent("\" value=\"");
    _driver->printContent(itoa(_value, itoa_buf, 10));
    _driver->printContent("\" onChange=\"doRequest(this.id, this.value);\"/>");
}

const char* ArduJAXSlider::value(uint8_t which) const {
    if (which == ArduJAXBase::Value) return itoa(_value, itoa_buf, 10);
    return ArduJAXElement::value(which);
}

const char* ArduJAXSlider::valueProperty(uint8_t which) const {
    if (which == ArduJAXBase::Value) return "value";
    return ArduJAXElement::valueProperty(which);
}

void ArduJAXSlider::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _value = atoi(buf);
}

void ArduJAXSlider::setValue(int16_t value) {
    _value = value;
    setChanged();
}

//////////////////////// ArduJAXPushButton /////////////////////////////

ArduJAXPushButton::ArduJAXPushButton(const char* id, const char* label, void (*callback)(ArduJAXPushButton*)) : ArduJAXElement (id) {
    _label = label;
    _callback = callback;
    setBasicProperty(ArduJAXBase::HTMLAllowed, true);
}

void ArduJAXPushButton::print() const {
    _driver->printContent("<button type=\"button\" id=");
    _driver->printQuoted(_id);
    _driver->printContent(" onClick=\"doRequest(this.id, 'p');\">");
    _driver->printFiltered(_label, false, valueNeedsEscaping());
    _driver->printContent("</button>");
}

void ArduJAXPushButton::setText(const char* label, bool allowHTML) {
    _label = label;
    setBasicProperty(ArduJAXBase::HTMLAllowed, allowHTML);
    setChanged();
}

const char* ArduJAXPushButton::value(uint8_t which) const {
    if (which == ArduJAXBase::Value) return _label;
    return ArduJAXElement::value(which);
}

bool ArduJAXPushButton::valueNeedsEscaping(uint8_t which) const {
    if (which == ArduJAXBase::Value) return !basicProperty(ArduJAXBase::HTMLAllowed);
    return ArduJAXElement::valueNeedsEscaping(which);
}

const char* ArduJAXPushButton::valueProperty(uint8_t which) const {
    if (which == ArduJAXBase::Value) return "innerHTML";
    return ArduJAXElement::valueProperty(which);
}

void ArduJAXPushButton::updateFromDriverArg(const char* argname) {
    _callback(this);
}

//////////////////////// ArduJAXCheckButton /////////////////////////////

ArduJAXCheckButton::ArduJAXCheckButton(const char* id, const char* label, bool checked) : ArduJAXElement(id) {
    _label = label;
    _checked = checked;
    radiogroup = 0;
}

void ArduJAXCheckButton::print() const {
    _driver->printContent("<input id=");
    _driver->printQuoted(_id);
    _driver->printContent(" type=");
    if (radiogroup) {
        _driver->printContent("\"radio\"");
        _driver->printContent("\" name=");
        _driver->printQuoted(radiogroup->_name);
    }
    else _driver->printContent("\"checkbox\"");
    _driver->printContent(" value=\"t\" onChange=\"doRequest(this.id, this.checked ? 't' : 'f');\"");
    if (_checked) _driver->printContent(" checked=\"true\"");
    _driver->printContent ("/><label for=\"");
    _driver->printQuoted(_id);
    _driver->printContent("\">");
    _driver->printContent(_label);  // NOTE: Not escaping anything, so user can insert HTML.
    _driver->printContent("</label>");
}

const char* ArduJAXCheckButton::value(uint8_t which) const {
    if (which == ArduJAXBase::Value) return _checked ? "true" : "";
    return ArduJAXElement::value(which);
}

void ArduJAXCheckButton::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _checked = (buf[0] == 't');
    if (_checked && radiogroup) radiogroup->selectOption(this);
}

const char* ArduJAXCheckButton::valueProperty(uint8_t which) const {
    if (which == ArduJAXBase::Value) return "checked";
    return ArduJAXElement::valueProperty(which);
}

void ArduJAXCheckButton::setChecked(bool checked) {
    if (_checked == checked) return;
    _checked = checked;
    if (radiogroup && checked) radiogroup->selectOption(this);
    setChanged();
}

//////////////////////// ArduJAXOptionSelect(Base) ///////////////

void ArduJAXOptionSelectBase::print(const char* const* _labels, uint8_t NUM) const {
    _driver->printContent("<select id=");
    _driver->printQuoted(_id);
    _driver->printContent(" onChange=\"doRequest(this.id, this.value)\">\n");
    for(uint8_t i = 0; i < NUM; ++i) {
        _driver->printContent("<option value=\"");
        _driver->printContent(itoa(i, itoa_buf, 10));
        _driver->printContent("\">");
        _driver->printContent(_labels[i]);
        _driver->printContent("</option>\n");
    }
    _driver->printContent("</select>");
}

void ArduJAXOptionSelectBase::selectOption(uint8_t num) {
    _current_option = num;
    setChanged();
}

uint8_t ArduJAXOptionSelectBase::selectedOption() const {
    return _current_option;
}

const char* ArduJAXOptionSelectBase::value(uint8_t which) const {
    if (which == ArduJAXBase::Value) return (itoa(_current_option, itoa_buf, 10));
    return ArduJAXElement::value(which);
}

const char* ArduJAXOptionSelectBase::valueProperty(uint8_t which) const {
    if (which == ArduJAXBase::Value) return ("value");
    return ArduJAXElement::valueProperty(which);
}

void ArduJAXOptionSelectBase::updateFromDriverArg(const char* argname) {
    _current_option = atoi(_driver->getArg(argname, itoa_buf, ITOA_BUFLEN));
}

//////////////////////// ArduJAXPage /////////////////////////////

void ArduJAXBase::printPage(ArduJAXBase** _children, uint NUM, const char* _title, const char* _header_add) const {
    _driver->printHeader(true);
    _driver->printContent("<HTML><HEAD><TITLE>");
    if (_title) _driver->printContent(_title);
    _driver->printContent("</TITLE>\n<SCRIPT>\n");
    _driver->printContent("var serverrevision = 0;\n"
                            "function doRequest(id='', value='') {\n"
                            "    var req = new XMLHttpRequest();\n"
                            "    req.timeout = 10000;\n"   // probably disconnected. Don't stack up request objects forever.
                            "    if(window.ardujaxsh) window.ardujaxsh.out();\n"
                            "    req.onload = function() {\n"
                            "       doUpdates(JSON.parse(req.responseText));\n"
                            "       if(window.ardujaxsh) window.ardujaxsh.in();\n"
                            "    }\n"
                            "    if(id) {\n"  // if we fail to transmit a UI change to the server, for whatever reason, we will be out of sync
                            "       req.onerror = req.ontimeout = function() {\n"
                            "          serverrevision = 0;\n" // this will cause the server to re-send _all_ element states on the next poll()
                            "       }\n"
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
                            "          var spec = changes[j][0].split('.');\n"
                            "          var prop = element;\n"
                            "          for(k = 0; k < (spec.length-1); ++k) {\n"   // resolve nested attributes such as style.display
                            "              prop = prop[spec[k]];\n"
                            "          }\n"
                            "          prop[spec[spec.length-1]] = changes[j][1];\n"
                            "       }\n"
                            "    }\n"
                            "}\n");
    _driver->printContent("function doPoll() {\n"
                            "    doRequest();\n"  // poll == request without id
                            "}\n"
                            "setInterval(doPoll,1000);\n");
    _driver->printContent("</SCRIPT>\n");
    if (_header_add) _driver->printContent(_header_add);
    _driver->printContent("</HEAD>\n<BODY><FORM autocomplete=\"off\">\n");  // NOTE: The nasty thing about autocomplete is that it does not trigger onChange() functions,
                                                                            // but also the "restore latest settings after client reload" is questionable in our use-case.
    printChildren(_children, NUM);

    _driver->printContent("\n</FORM></BODY></HTML>\n");
}

void ArduJAXBase::handleRequest(ArduJAXBase** _children, uint NUM, void (*change_callback)()) {
    char conversion_buf[ARDUJAX_MAX_ID_LEN];

    // handle value changes sent from client
    uint16_t client_revision = atoi(_driver->getArg("revision", conversion_buf, ARDUJAX_MAX_ID_LEN));
    if (client_revision > _driver->revision()) {
        // This could happen on overflow, or if the server has rebooted, but not the client.
        // Setting revision to 0, here, means that all elements are considered changed, and will be
        // synced to the client.
        client_revision = 0;
    }
    const char *id = _driver->getArg("id", conversion_buf, ARDUJAX_MAX_ID_LEN);
    ArduJAXElement *element = (id[0] == '\0') ? 0 : findChild(id);
    if (element) {
        element->updateFromDriverArg("value");
        element->setChanged(); // So changes sent from one client will be synced to the other clients
        if (change_callback) change_callback();
    }
    _driver->nextRevision();

    // then relay value changes that have occured in the server (possibly in response to those sent)
    _driver->printHeader(false);
    _driver->printContent("{\"revision\": ");
    _driver->printContent(itoa(_driver->revision(), conversion_buf, 10));
    _driver->printContent(",\n\"updates\": [\n");
    sendUpdates(_children, NUM, client_revision, true);
    _driver->printContent("\n]}\n");
}
