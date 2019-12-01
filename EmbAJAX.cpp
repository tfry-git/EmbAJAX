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

// Avoid auto-including an output driver, here.
#define EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION

#include "EmbAJAX.h"

#define ITOA_BUFLEN 8

// statics
EmbAJAXOutputDriverBase *EmbAJAXBase::_driver;
char EmbAJAXBase::itoa_buf[ITOA_BUFLEN];

////////////////////////////// EmbAJAXOutputDriverBase ////////////////////

void EmbAJAXOutputDriverBase::printFiltered(const char* value, QuoteMode quoted, bool HTMLescaped) {
    // NOTE: The assumption, here is that frequent (char-by-char) calls to printContent() _could_ be expensive, depending on the server
    //       implementation. Thus, a buffer is used to enable printing in larger chunks.
    char buf[32];
    uint bufpos = 0;
    if (quoted) buf[bufpos++] = '"';
    const char *pos = value;
    while(*pos != '\0') {
        if (bufpos > 23) {  // == Not enough room for the worst case, i.e. "&quot;" + quote-end + terminating '\0'
            buf[bufpos] = '\0';
            printContent(buf);
            bufpos = 0;
            continue;
        }
        if ((quoted == JSQuoted) && (*pos == '"' || *pos == '\\')) {
            buf[bufpos++] = '\\';
            buf[bufpos++] = *pos;
        } else if ((quoted == HTMLQuoted) && (*pos == '"')) {
            buf[bufpos++] = '&';
            buf[bufpos++] = 'q';
            buf[bufpos++] = 'u';
            buf[bufpos++] = 'o';
            buf[bufpos++] = 't';
            buf[bufpos++] = ';';
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

void EmbAJAXOutputDriverBase::printAttribute(const char* name, const char* value) {
    printContent(" ");
    printContent(name);
    printContent("=");
    printHTMLQuoted(value);
}

void EmbAJAXOutputDriverBase::printAttribute(const char* name, const int32_t value) {
    printContent(" ");
    printContent(name);
    printContent("=");
    char buf[12];
    printContent(itoa(value, buf, 10));
}

//////////////////////// EmbAJAXConnectionIndicator ///////////////////////

void EmbAJAXConnectionIndicator::print() const {
    _driver->printContent("<div><script>\n"
                          "window.ardujaxsh = { 'div': document.scripts[document.scripts.length-1].parentNode,\n"
                          "'misses': 0,\n"
                          "'in': function() { if(this.misses) { this.misses = 0; this.div.innerHTML=");
    _driver->printJSQuoted(_content_ok ? _content_ok : "<span class=\"EmbAJAXStatusOK\" style=\"background-color:green;\">OK</span>");
    _driver->printContent (";}},\n"
                           "'out': function() {if (this.misses < 5) { if(++(this.misses) >= 5) this.div.innerHTML=");
    _driver->printJSQuoted(_content_fail ? _content_fail : "<span class=\"EmbAJAXStatusOK\" style=\"background-color:red;\">FAIL</span>");
    _driver->printContent(";}}\n"
                          "}\n</script></div>");
}

////////////////////////////// EmbAJAXElement /////////////////////////////

/** @param id: The id for the element. Note that the string is not copied. Do not use a temporary string in this place. Also, do keep it short. */
EmbAJAXElement::EmbAJAXElement(const char* id) : EmbAJAXBase() {
    _id = id;
    _flags = 1 << EmbAJAXBase::Visibility | 1 << EmbAJAXBase::Enabledness;
    revision = 1;
}

bool EmbAJAXElement::sendUpdates(uint16_t since, bool first) {
    if (!changed(since)) return false;
    if (!first) _driver->printContent(",\n");
    _driver->printContent("{\n\"id\": ");
    _driver->printJSQuoted(_id);
    _driver->printContent(",\n\"changes\": [");
    uint8_t i = 0;
    while (true) {
        const char* pid = valueProperty(i);
        const char* pval = value(i);
        if (!pid || !pval) break;

        if (i != 0) _driver->printContent(",");
        _driver->printContent("[");
        _driver->printJSQuoted(pid);
        _driver->printContent(", ");
        _driver->printFiltered(pval, EmbAJAXOutputDriverBase::JSQuoted, valueNeedsEscaping(i));
        _driver->printContent("]");

        ++i;
    }
    _driver->printContent("]\n}");
    return true;
}

void EmbAJAXElement::setBasicProperty(uint8_t num, bool status) {
    uint8_t status_bit = 1 << num;
    if (status == (bool) (_flags & status_bit)) return;
    if (status) _flags |= status_bit;
    else _flags -= _flags & status_bit;
    setChanged();
}

void EmbAJAXElement::setChanged() {
    revision = _driver->setChanged();
}

bool EmbAJAXElement::changed(uint16_t since) {
    if ((revision + 40000) < since) revision = since + 1;    // basic overflow protection. Results in sending _all_ states at least every 40000 request cycles
    return (revision > since);
}

void EmbAJAXElement::printTextInput(uint SIZE, const char* _value) const {
    _driver->printContent("<input type=\"text\"");
    _driver->printAttribute("id", _id);
    _driver->printAttribute("maxLength", SIZE-1);
    _driver->printAttribute("size", min(max(abs(SIZE-1), 10),40));  // Arbitray limit for rendered width of text fields: 10..40 chars
    _driver->printAttribute("value", _value);
    // Using onChange to update is too awkward. Using plain onInput would generate too may requests (and often result in "eaten" characters). Instead,
    // as a compromise, we arrange for an update one second after the last key was pressed.
    _driver->printContent(" onInput=\"clearTimeout(this.debouncer); this.debouncer=setTimeout(function() {doRequest(this.id, this.value);}.bind(this),1000);\"/>");
}

//////////////////////// EmbAJAXContainer ////////////////////////////////////

void EmbAJAXBase::printChildren(EmbAJAXBase** _children, uint NUM) const {
    for (uint i = 0; i < NUM; ++i) {
        _children[i]->print();
    }
}

bool EmbAJAXBase::sendUpdates(EmbAJAXBase** _children, uint NUM, uint16_t since, bool first) {
    for (uint i = 0; i < NUM; ++i) {
        bool sent = _children[i]->sendUpdates(since, first);
        if (sent) first = false;
    }
    return !first;
}

EmbAJAXElement* EmbAJAXBase::findChild(EmbAJAXBase** _children, uint NUM, const char*id) const {
    for (uint i = 0; i < NUM; ++i) {
        EmbAJAXElement* child = _children[i]->toElement();
        if (child) {
            if (strcmp(id, child->id()) == 0) return child;
        }
        child = _children[i]->findChild (id);
        if (child) return child;
    }
    return 0;
}

//////////////////////// EmbAJAXMutableSpan /////////////////////////////

void EmbAJAXMutableSpan::print() const {
    _driver->printContent("<span");
    _driver->printAttribute("id", _id);
    _driver->printContent(">");
    if (_value) _driver->printFiltered(_value, EmbAJAXOutputDriverBase::NotQuoted, valueNeedsEscaping());
    _driver->printContent("</span>\n");
}

const char* EmbAJAXMutableSpan::value(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return _value;
    return EmbAJAXElement::value(which);
}

bool EmbAJAXMutableSpan::valueNeedsEscaping(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return !basicProperty(EmbAJAXBase::HTMLAllowed);
    return EmbAJAXElement::valueNeedsEscaping(which);
}

const char* EmbAJAXMutableSpan::valueProperty(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return "innerHTML";
    return EmbAJAXElement::valueProperty(which);
}

void EmbAJAXMutableSpan::setValue(const char* value, bool allowHTML) {
    // TODO: Ideally we'd special case setValue() with old value == new value (noop). However, since often both old and new values are kept in the same char buffer,
    // we cannot really compare the strings - without keeping a copy, at least. Should we?
    _value = value;
    setBasicProperty(EmbAJAXBase::HTMLAllowed, allowHTML);
    setChanged();
}

//////////////////////// EmbAJAXSlider /////////////////////////////

EmbAJAXSlider::EmbAJAXSlider(const char* id, int16_t min, int16_t max, int16_t initial) : EmbAJAXElement(id) {
    _value = initial;
    _min = min;
    _max = max;
}

void EmbAJAXSlider::print() const {
    _driver->printContent("<input type=\"range\"");
    _driver->printAttribute("id", _id);
    _driver->printAttribute("min", _min);
    _driver->printAttribute("max", _max);
    _driver->printAttribute("value", _value);
    _driver->printContent(" onChange=\"doRequest(this.id, this.value);\"/>");
}

const char* EmbAJAXSlider::value(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return itoa(_value, itoa_buf, 10);
    return EmbAJAXElement::value(which);
}

const char* EmbAJAXSlider::valueProperty(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return "value";
    return EmbAJAXElement::valueProperty(which);
}

void EmbAJAXSlider::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _value = atoi(buf);
}

void EmbAJAXSlider::setValue(int16_t value) {
    _value = value;
    setChanged();
}

//////////////////////// EmbAJAXColorPicker /////////////////////////

EmbAJAXColorPicker::EmbAJAXColorPicker(const char* id, uint8_t r, uint8_t g, uint8_t b) : EmbAJAXElement(id) {
    _r = r;
    _g = g;
    _b = b;
}

void EmbAJAXColorPicker::print() const {
    _driver->printContent("<input type=\"color\"");
    _driver->printAttribute("id", _id);
    _driver->printAttribute("value", value());
    // see EmbAJAXTextInput::print(): Use onInput(), instead of onChange().
    _driver->printContent(" onInput=\"clearTimeout(this.debouncer); this.debouncer=setTimeout(function() {doRequest(this.id, this.value);}.bind(this),1000);\"/>");
}

// helper to make sure we get exactly two hex digits for any input
char* color_itoa(uint8_t c, char* buf) {
    itoa(c >> 4, buf, 16);
    itoa(c & 0x0F, buf + sizeof(char), 16);
    return buf;
}

const char* EmbAJAXColorPicker::value(uint8_t which) const {
    if (which != EmbAJAXBase::Value) return EmbAJAXElement::value(which);

    itoa_buf[0] = '#';
    color_itoa(_r, &itoa_buf[1]);
    color_itoa(_g, &itoa_buf[3]);
    color_itoa(_b, &itoa_buf[5]);
    return itoa_buf;
}

const char* EmbAJAXColorPicker::valueProperty(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return "value";
    return EmbAJAXElement::value(which);
}

void EmbAJAXColorPicker::setColor(uint8_t r, uint8_t g, uint8_t b) {
    _r = r;
    _g = g;
    _b = b;
    setChanged();
}

uint8_t EmbAJAXColorPicker::red() const {
    return _r;
}

uint8_t EmbAJAXColorPicker::green() const {
    return _g;
}

uint8_t EmbAJAXColorPicker::blue() const {
    return _b;
}

// simple helper, because I don't want to pull in strtol
uint8_t single_hex_atoi(char c) {
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    return 0;
}

void EmbAJAXColorPicker::updateFromDriverArg(const char* argname)  {
    char buf[9];
    _driver->getArg(argname, buf, 8);
    if ((strlen (buf) != 7) || (buf[0] != '#')) { // format error. Set changed in order to sync back to client
        setChanged();
    }
    _r = (single_hex_atoi(buf[1]) << 4) + single_hex_atoi(buf[2]);
    _g = (single_hex_atoi(buf[3]) << 4) + single_hex_atoi(buf[4]);
    _b = (single_hex_atoi(buf[5]) << 4) + single_hex_atoi(buf[6]);
}

//////////////////////// EmbAJAXPushButton /////////////////////////////

EmbAJAXPushButton::EmbAJAXPushButton(const char* id, const char* label, void (*callback)(EmbAJAXPushButton*)) : EmbAJAXElement (id) {
    _label = label;
    _callback = callback;
    setBasicProperty(EmbAJAXBase::HTMLAllowed, true);
}

void EmbAJAXPushButton::print() const {
    _driver->printContent("<button type=\"button\"");
    _driver->printAttribute("id", _id);
    _driver->printContent(" onClick=\"doRequest(this.id, 'p');\">");
    _driver->printFiltered(_label, EmbAJAXOutputDriverBase::NotQuoted, valueNeedsEscaping());
    _driver->printContent("</button>");
}

void EmbAJAXPushButton::setText(const char* label, bool allowHTML) {
    _label = label;
    setBasicProperty(EmbAJAXBase::HTMLAllowed, allowHTML);
    setChanged();
}

const char* EmbAJAXPushButton::value(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return _label;
    return EmbAJAXElement::value(which);
}

bool EmbAJAXPushButton::valueNeedsEscaping(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return !basicProperty(EmbAJAXBase::HTMLAllowed);
    return EmbAJAXElement::valueNeedsEscaping(which);
}

const char* EmbAJAXPushButton::valueProperty(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return "innerHTML";
    return EmbAJAXElement::valueProperty(which);
}

void EmbAJAXPushButton::updateFromDriverArg(const char* argname) {
    if (_callback) _callback(this);
}

//////////////////////// EmbAJAXḾomentaryButton /////////////////////////////

EmbAJAXMomentaryButton::EmbAJAXMomentaryButton(const char* id, const char* label, uint16_t timeout, void (*callback)(EmbAJAXPushButton*)) : EmbAJAXPushButton(id, label, callback) {
    latest_ping = 0;
    _timeout = timeout;
}

void EmbAJAXMomentaryButton::print() const {
    _driver->printContent("<button type=\"button\"");
    _driver->printAttribute("id", _id);
    _driver->printContent(" onMouseDown=\"this.pinger=setInterval(function() {doRequest(this.id, 'p');}.bind(this),");
    _driver->printContent(itoa(_timeout / 1.5, itoa_buf, 10));
    _driver->printContent("); doRequest(this.id, 'p');\" onMouseUp=\"clearInterval(this.pinger); doRequest(this.id, 'r');\" onMouseLeave=\"clearInterval(this.pinger); doRequest(this.id, 'r');\">");
    _driver->printFiltered(_label, EmbAJAXOutputDriverBase::NotQuoted, valueNeedsEscaping());
    _driver->printContent("</button>");
}

EmbAJAXMomentaryButton::Status EmbAJAXMomentaryButton::status() const {
    if (latest_ping == 0) return Released;
    if ((millis () - latest_ping) < _timeout) return Pressed;
    return MaybePressed;
}

void EmbAJAXMomentaryButton::updateFromDriverArg(const char* argname) {
    char buf[8];
    _driver->getArg(argname, buf, 8);
    if (buf[0] == 'p') {
        latest_ping = millis();
        if (!latest_ping) latest_ping = 1; // paranoid overflow protection
    } else {
        latest_ping = 0;
    }
    EmbAJAXPushButton::updateFromDriverArg(argname);
}

//////////////////////// EmbAJAXCheckButton /////////////////////////////

EmbAJAXCheckButton::EmbAJAXCheckButton(const char* id, const char* label, bool checked) : EmbAJAXElement(id) {
    _label = label;
    _checked = checked;
    radiogroup = 0;
}

void EmbAJAXCheckButton::print() const {
    _driver->printContent("<span"); // <input> and <label> inside a common span to support hiding, better
    _driver->printAttribute("class", radiogroup ? "radio" : "checkbox");  // also, assign a class to the surrounding span to ease styling via CSS
    _driver->printContent("><input");
    _driver->printAttribute("id", _id);
    _driver->printAttribute("type", radiogroup ? "radio" : "checkbox");
    if (radiogroup) {
        _driver->printAttribute("name", radiogroup->_name);
    }
    _driver->printContent(" value=\"t\" onChange=\"doRequest(this.id, this.checked ? 't' : 'f');\"");
    if (_checked) _driver->printContent(" checked=\"true\"");
    _driver->printContent ("/><label");  // Note: Internal <span> element for more flexbility in styling the control
    _driver->printAttribute("for", _id);
    _driver->printContent(">");
    _driver->printContent(_label);  // NOTE: Not escaping anything, so user can insert HTML.
    _driver->printContent("</label></span>");
}

const char* EmbAJAXCheckButton::value(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return _checked ? "true" : "";
    return EmbAJAXElement::value(which);
}

void EmbAJAXCheckButton::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _checked = (buf[0] == 't');
    if (_checked && radiogroup) radiogroup->selectOption(this);
}

const char* EmbAJAXCheckButton::valueProperty(uint8_t which) const {
    if (which == EmbAJAXBase::Visibility) return "parentNode.style.display";
    if (which == EmbAJAXBase::Value) return "checked";
    return EmbAJAXElement::valueProperty(which);
}

void EmbAJAXCheckButton::setChecked(bool checked) {
    if (_checked == checked) return;
    _checked = checked;
    if (radiogroup && checked) radiogroup->selectOption(this);
    setChanged();
}

//////////////////////// EmbAJAXOptionSelect(Base) ///////////////

void EmbAJAXOptionSelectBase::print(const char* const* _labels, uint8_t NUM) const {
    _driver->printContent("<select");
    _driver->printAttribute("id", _id);
    _driver->printContent(" onChange=\"doRequest(this.id, this.value)\">\n");
    for(uint8_t i = 0; i < NUM; ++i) {
        _driver->printContent("<option");
        _driver->printAttribute("value", i);
        _driver->printContent(">");
        _driver->printContent(_labels[i]);
        _driver->printContent("</option>\n");
    }
    _driver->printContent("</select>");
}

void EmbAJAXOptionSelectBase::selectOption(uint8_t num) {
    _current_option = num;
    setChanged();
}

uint8_t EmbAJAXOptionSelectBase::selectedOption() const {
    return _current_option;
}

const char* EmbAJAXOptionSelectBase::value(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return (itoa(_current_option, itoa_buf, 10));
    return EmbAJAXElement::value(which);
}

const char* EmbAJAXOptionSelectBase::valueProperty(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return ("value");
    return EmbAJAXElement::valueProperty(which);
}

void EmbAJAXOptionSelectBase::updateFromDriverArg(const char* argname) {
    _current_option = atoi(_driver->getArg(argname, itoa_buf, ITOA_BUFLEN));
}

//////////////////////// EmbAJAXPage /////////////////////////////

void EmbAJAXBase::printPage(EmbAJAXBase** _children, uint NUM, const char* _title, const char* _header_add) const {
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
    _driver->printContent("</HEAD>\n<BODY><FORM autocomplete=\"off\" onSubmit=\"return false;\">\n");  // NOTE: The nasty thing about autocomplete is that it does not trigger onChange() functions,
                                                                                                       // but also the "restore latest settings after client reload" is questionable in our use-case.
    printChildren(_children, NUM);

    _driver->printContent("\n</FORM></BODY></HTML>\n");
}

void EmbAJAXBase::handleRequest(EmbAJAXBase** _children, uint NUM, void (*change_callback)()) {
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
    EmbAJAXElement *element = (id[0] == '\0') ? 0 : findChild(id);
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
