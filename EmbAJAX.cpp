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

#include <cstdarg> // For va_args in _printContentF.

#define ITOA_BUFLEN 8

// statics
EmbAJAXOutputDriverBase *EmbAJAXBase::_driver;
char EmbAJAXBase::itoa_buf[ITOA_BUFLEN];
constexpr const char EmbAJAXBase::null_string[1];

////////////////////////////// EmbAJAXOutputDriverBase ////////////////////

void EmbAJAXOutputDriverBase::_printFiltered(const char* value, QuoteMode quoted, bool HTMLescaped) {
    if (quoted) _printChar('"');
    const char *pos = value;
    while(*pos != '\0') {
        if ((quoted == JSQuoted) && (*pos == '"' || *pos == '\\')) {
            _printChar('\\');
            _printChar(*pos);
        } else if ((quoted == HTMLQuoted) && (*pos == '"')) {
            _printContent("&quot;");
        } else if (HTMLescaped && (*pos == '<')) {
            _printContent("&lt;");
        } else if (HTMLescaped && (*pos == '&')) {
            _printContent("&amp;");
        } else {
            _printChar(*pos);
        }
        ++pos;
    }
    if (quoted) _printChar('"');
}

void EmbAJAXOutputDriverBase::commitBuffer() {
    _buf[_bufpos] = '\0';
    printContent(_buf);
    _bufpos = 0;
}

void EmbAJAXOutputDriverBase::_printChar(const char value) {
    if (_bufpos >= 63) commitBuffer();
    _buf[_bufpos++] = value;
}

void EmbAJAXOutputDriverBase::_printContent(const char* value) {
    // NOTE: The assumption, here is that frequent (char-by-char) calls to printContent() _could_ be expensive, depending on the server
    //       implementation. Thus, a buffer is used to enable printing in larger chunks.
    const char *pos = value;
    while(*pos != '\0') {
        _printChar(*pos);
        ++pos;
    }
}

void EmbAJAXOutputDriverBase::_printContentF(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const char *pos = fmt;
    while(true) {
        const char c = *pos++;
        if (c == '\0') break;
        else if (c == JS_QUOTED_STRING_ARG[0]) {
            _printFiltered(va_arg(args, char*), JSQuoted, false);
        } else if (c == HTML_QUOTED_STRING_ARG[0]) {
            _printFiltered(va_arg(args, char*), HTMLQuoted, false);
        } else if (c == HTML_ESCAPED_STRING_ARG[0]) {
            _printFiltered(va_arg(args, char*), NotQuoted, true);
        } else if (c == INTEGER_VALUE_ARG[0]) {
            char buf[12];
            _printContent(itoa(va_arg(args, int), buf, 10));
        } else if (c == PLAIN_STRING_ARG[0]) {
            _printContent(va_arg(args, char*));
        } else {
            _printChar(c);
        }
    }
    va_end(args);
    commitBuffer();
}

void EmbAJAXOutputDriverBase::printAttribute(const char* name, const char* value) {
    _printContentF(" " PLAIN_STRING_ARG "=" HTML_QUOTED_STRING_ARG, name, value);
}

void EmbAJAXOutputDriverBase::printAttribute(const char* name, const int32_t value) {
    _printContentF(" " PLAIN_STRING_ARG "=" INTEGER_VALUE_ARG, name, value);
}

//////////////////////// EmbAJAXConnectionIndicator ///////////////////////

void EmbAJAXConnectionIndicator::print() const {
    _driver->printFormatted("<div class=\"EmbAJAXStatus\"><span>", PLAIN_STRING(_content_ok), "</span><span>", PLAIN_STRING(_content_fail), "</span><script>\n"
                           "window.ardujaxsh = { 'div': document.scripts[document.scripts.length-1].parentNode,\n"
                           "'good': 0,\n"
                           "'tid': null,\n"
                           "'toggle': function(on) { this.div.children[on].style.display = 'none'; this.div.children[1-on].style.display = 'inline'; this.good = on; },\n"
                           "'in': function() { clearTimeout(this.tid); this.tid = window.setTimeout(this.toggle.bind(this, 0), 5000); if(!this.good) {this.toggle(1);} }\n"
                           "};\nwindow.ardujaxsh.in();\n</script></div>");
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
    _driver->printFormatted("{\n\"id\": ", JS_QUOTED_STRING(_id), ",\n\"changes\": [");
    uint8_t i = 0;
    while (true) {
        const char* pid = valueProperty(i);
        const char* pval = value(i);
        if (!pid || !pval) break;

        if (i != 0) _driver->printContent(",");
        _driver->printFormatted("[", JS_QUOTED_STRING(pid), ", ");
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

void EmbAJAXElement::printTextInput(size_t SIZE, const char* _value) const {
    _driver->printFormatted("<input type=\"text\" id=", HTML_QUOTED_STRING(_id), " maxLength=", INTEGER_VALUE(SIZE-1),
                            " size=", INTEGER_VALUE(min(max((size_t) SIZE, (size_t) 11), (size_t) 41) - 1), // Arbitray limit for rendered width of text fields: 10..40 chars
                            " value=", HTML_QUOTED_STRING(_value), " onInput=\"doRequest(this.id, this.value);\"/>");
}

//////////////////////// EmbAJAXContainer ////////////////////////////////////

void EmbAJAXBase::printChildren(EmbAJAXBase** _children, size_t NUM) const {
    for (size_t i = 0; i < NUM; ++i) {
        _children[i]->print();
    }
}

bool EmbAJAXBase::sendUpdates(EmbAJAXBase** _children, size_t NUM, uint16_t since, bool first) {
    for (size_t i = 0; i < NUM; ++i) {
        bool sent = _children[i]->sendUpdates(since, first);
        if (sent) first = false;
    }
    return !first;
}

EmbAJAXElement* EmbAJAXBase::findChild(EmbAJAXBase** _children, size_t NUM, const char*id) const {
    for (size_t i = 0; i < NUM; ++i) {
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
    _driver->printFormatted("<span id=", HTML_QUOTED_STRING(_id), ">");
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
    _driver->printFormatted("<input type=\"range\" id=", HTML_QUOTED_STRING(_id), " min=", INTEGER_VALUE(_min), " max=", INTEGER_VALUE(_max), " value=", INTEGER_VALUE(_value),
                           " oninput=\"doRequest(this.id, this.value);\" onchange=\"oninput();\"/>");
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
    _driver->printFormatted("<input type=\"color\" id=", HTML_QUOTED_STRING(_id), " value=", HTML_QUOTED_STRING(value()),
                           " oninput=\"doRequest(this.id, this.value);\" onchange=\"oninput();\"/>");
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
    _flags |= (1 << EmbAJAXBase::HTMLAllowed); // like setBasicProperty(EmbAJAXBase::HTMLAllowed, true); but without changing value or presuming a driver instance
}

void EmbAJAXPushButton::print() const {
    _driver->printFormatted("<button type=\"button\" id=", HTML_QUOTED_STRING(_id),
                           " onClick=\"doRequest(this.id, 'p', 2);\">"); // 2 -> not mergeable -> so we can count individual presses, even if they happen fast
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
    _driver->printFormatted("<button type=\"button\" id=", HTML_QUOTED_STRING(_id), ">");
    _driver->printFiltered(_label, EmbAJAXOutputDriverBase::NotQuoted, valueNeedsEscaping());
    _driver->printFormatted("</button>"
                          "<script>\n"
                          "{let btn=document.getElementById(", JS_QUOTED_STRING(_id), ");\n"
                          "btn.onmousedown = btn.ontouchstart = function() { clearInterval(this.pinger); this.pinger=setInterval(function() {doRequest(this.id, 'p');}.bind(this),", INTEGER_VALUE((int) (_timeout / 1.5)), "); doRequest(this.id, 'p'); return false; };\n"
                          "btn.onmouseup = btn.ontouchend = btn.onmouseleave = function() { clearInterval(this.pinger); doRequest(this.id, 'r'); return false;};}\n"
                          "</script>");
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
    radiogroup = nullptr;
}

void EmbAJAXCheckButton::print() const {
    _driver->printFormatted("<span class=", HTML_QUOTED_STRING(radiogroup ? "radio" : "checkbox"), ">" // <input> and <label> inside a common span to support hiding, better.
                                                                                                       // Also, assign a class to the surrounding span to ease styling via CSS.
                           "<input id=", HTML_QUOTED_STRING(_id), " type=", HTML_QUOTED_STRING(radiogroup ? "radio" : "checkbox"),
                           " value=\"t\" onChange=\"doRequest(this.id, this.checked ? 't' : 'f');\"");
    if (radiogroup) _driver->printAttribute("name", radiogroup->_name);
    if (_checked) _driver->printContent(" checked=\"true\"");
    // Note: Internal <span> element for more flexbility in styling the control
    _driver->printFormatted("/><label for=", HTML_QUOTED_STRING(_id), ">", PLAIN_STRING(_label), // NOTE: Not escaping _label, so user can insert HTML.
                           "</label></span>");
}

const char* EmbAJAXCheckButton::value(uint8_t which) const {
    if (which == EmbAJAXBase::Value) return _checked ? "true" : "";
    return EmbAJAXElement::value(which);
}

void EmbAJAXCheckButton::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _checked = (buf[0] == 't');
    if (_checked && radiogroup) radiogroup->selectButton(this);
}

const char* EmbAJAXCheckButton::valueProperty(uint8_t which) const {
    if (which == EmbAJAXBase::Visibility) return "parentNode.style.display";
    if (which == EmbAJAXBase::Value) return "checked";
    return EmbAJAXElement::valueProperty(which);
}

void EmbAJAXCheckButton::setChecked(bool checked) {
    if (_checked == checked) return;
    _checked = checked;
    if (radiogroup && checked) radiogroup->selectButton(this);
    setChanged();
}

//////////////////////// EmbAJAXOptionSelect(Base) ///////////////

void EmbAJAXOptionSelectBase::print(const char* const* _labels, uint8_t NUM) const {
    _driver->printFormatted("<select id=", HTML_QUOTED_STRING(_id), " onChange=\"doRequest(this.id, this.value)\">\n");
    for(uint8_t i = 0; i < NUM; ++i) {
        _driver->printFormatted("<option value=", INTEGER_VALUE(i), ">", HTML_QUOTED_STRING(_labels[i]), "</option>\n");
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

void EmbAJAXBase::printPage(EmbAJAXBase** _children, size_t NUM, const char* _title, const char* _header_add, uint16_t _min_interval) const {
#if EMBAJAX_DEBUG > 2
    time_t start = millis();
#endif
    _driver->printHeader(true);
    _driver->printFormatted("<!DOCTYPE html>\n<HTML><HEAD><TITLE>", PLAIN_STRING(_title), "</TITLE>\n<SCRIPT>\n"

                            "var serverrevision = 0;\n"
                            "var request_queue = [];\n"   // requests waiting to be sent
                            // message types: 1: regular: request may be overridden by subsequent value changes on the same id - merge if in queue
                            //                2: semi-distinct: request may override type 1 requests for the same id, but will never be overridden (button clicks)
                            //                3: fully-distinct: request may not be merged with other requests of the same id at all
                            "function doRequest(id, value, mtype=1) {\n"
                            "    var req = {id: id, value: value, mtype: mtype};\n"
                            "    const i = request_queue.findIndex((x) => (x.id == id && x.mtype == 1));\n"
                            "    if (i >= 0 && (mtype < 3)) request_queue[i] = req;\n"
                            "    else request_queue.push(req);\n"
                            "    window.setTimeout(sendQueued, 0);\n"  // NOTE: often events will be generated twice (e.g. onInput+onChange). Wait for the second to come in, before sending
                            "}\n"

                            "var num_waiting = 0;\n"      // number of requests sent, with no reply received, yet
                            "var prev_request = 0;\n"
                            "function sendQueued() {\n"
                            "    var now = new Date().getTime();\n"
                            "    if (num_waiting > 0 || (now - prev_request < ", INTEGER_VALUE(_min_interval), ")) return;\n"
                            "    var e = request_queue.shift();\n"
                            "    if (!e && (now - prev_request < 1000)) return;\n"
                            "    if (!e) e = {id: '', value: ''};\n" //Nothing in queue, but last request more than 1000 ms ago? Send a ping to query for updates
                            "    var req = new XMLHttpRequest();\n"
                            "    req.timeout = 10000;\n"   // probably disconnected. Don't stack up request objects forever.
                            "    req.onload = function() {\n"
                            "       doUpdates(JSON.parse(req.responseText));\n"
                            "       if(window.ardujaxsh) window.ardujaxsh.in();\n"
                            "       --num_waiting;\n"
                            "    }\n"
                            "    req.onerror = req.ontimeout = function() {\n" // if transmission failed, assume we are out of sync
                            "       serverrevision = 0;\n" // this will cause the server to re-send _all_ element states on the next poll()
                            "       --num_waiting;\n"
                            "    };\n"
                            "    ++num_waiting; prev_request = now;\n"
                            "    req.open('POST', document.URL, true);\n"
                            "    req.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');\n"
                            "    req.send('id=' + e.id + '&value=' + encodeURIComponent(e.value) + '&revision=' + serverrevision);\n"
                            "}\n"
                            "window.setInterval(sendQueued, ", INTEGER_VALUE(_min_interval/2+1), ");\n"

                            "function doUpdates(response) {\n"
                            "    serverrevision = response.revision;\n"
                            "    var updates = response.updates;\n"
                            "    for(i = 0; i < updates.length; i++) {\n"
                            "       element = document.getElementById(updates[i].id);\n"
                            "       changes = updates[i].changes;\n"
                            "       for(j = 0; j < changes.length; ++j) {\n"
                            "          var spec = changes[j][0].split('.');\n"
                            "          var prop = element;\n"
#if EMBAJAX_DEBUG > 2
                            "          console.log('Received change at revision ' + serverrevision + ': ' + updates[i].id + '/' + spec + '=' + changes[j][1]);\n"
#endif
                            "          for(k = 0; k < (spec.length-1); ++k) {\n"   // resolve nested attributes such as style.display
                            "              prop = prop[spec[k]];\n"
                            "          }\n"
                            "          prop[spec[spec.length-1]] = changes[j][1];\n"
                            "       }\n"
                            "    }\n"
                            "}\n"

                            "</SCRIPT>\n", PLAIN_STRING(_header_add),
                            "</HEAD>\n<BODY><FORM autocomplete=\"off\" onSubmit=\"return false;\">\n");
                            // NOTE: The nasty thing about autocomplete is that it does not trigger onChange() functions, but also the
                            // "restore latest settings after client reload" is questionable in our use-case.

    printChildren(_children, NUM);

    _driver->printContent("\n</FORM></BODY></HTML>\n");
#if EMBAJAX_DEBUG > 2
    auto diff = millis() - start;
    Serial.print("Page rendered in ");
    Serial.print(diff);
    Serial.println("ms");
#endif
}

void EmbAJAXBase::handleRequest(EmbAJAXBase** _children, size_t NUM, void (*change_callback)()) {
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
#if EMBAJAX_DEBUG > 2
        Serial.print("Updating ");
        Serial.println(id);
        Serial.print("old revision ");
        Serial.print(element->revision);
        Serial.print(" old value ");
        Serial.println(element->value());
#endif
        element->updateFromDriverArg("value");
        element->setChanged();                  // See bottom of function for an explanation on revision handling here, and in general
        element->revision = client_revision;
        if (change_callback) change_callback();
#if EMBAJAX_DEBUG > 2
        Serial.print("(temp) new revision ");
        Serial.print(element->revision);
        Serial.print(" new value ");
        Serial.println(element->value());
#endif
    }
    _driver->nextRevision();
#if EMBAJAX_DEBUG > 2
    if (element || (EMBAJAX_DEBUG > 3)) {
        Serial.print("Update done. Client revision ");
        Serial.print(client_revision);
        Serial.print(" driver revision ");
        Serial.println(_driver->revision());
    }
#endif

    // then relay value changes that have occured in the server (possibly in response to those sent)
    _driver->printHeader(false);
    _driver->printFormatted("{\"revision\": ", INTEGER_VALUE(_driver->revision()), ",\n\"updates\": [\n");
    sendUpdates(_children, NUM, client_revision, true);
    _driver->printContent("\n]}\n");

    /* Explanation on revision handling:
     * Bascis - Revision signifies what changes a particular client has already seen. Each client keeps a separate revision number. Each element hold the reivison number of
     *          its latest change. Finally the server remembers the highest revision number that has _not yet_ been synced to any client (EmbAJAXOutputDriverBase::next_tevision).
     *          E.g. Element A was changed at revision 4, Element B at revision 7. Client knows revision 5 -> it will be sent the new value of Element B, but not A.
     *          Now, when Element A is changed, its revision will be upped to 8 (this happens in setChanged(). Subsequent changes will also get that revision number _until_
     *          a client syncs to that revision. Only now, subsequent changes to any element will get revision number 9. This keeps the numbers from going up very fast.
     * Here   - An important case to handle, is if a value change is being sent _from_ a client. I response that several further values, and even the value of the element itself
     *          may be changed. We want to sync all of that _except_ the value which was sent by the client itself. This is rather important, as messages will always arrive with
     *          at least a few ms delay. If, e.g. the user is typing in a text input, syncing back the change could easily happen _after_ the user has already typed another key.
     *          This key would then get "swallowed".
     *          To avoid syncing back this change, while still making sure any secondary change is synced: We first call setChanged() (so that the driver is aware that a new
     *          revision may be needed). Then, we re-set the revision to the revision number of the client. Usually it will stay that way, unless secondary changes trigger another
     *          update. Finally, after syncing back changes, we increase the revision, again, such that all further clients will be updated, appropriately. */
    if (element) element->revision = _driver->revision();
}
