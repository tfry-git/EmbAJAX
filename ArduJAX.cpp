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

ArduJAXElement* ArduJAXContainer::findChild(const char*id) const {
    for (int i = 0; i < _children.count; ++i) {
        ArduJAXElement* child = _children.members[i]->toElement();
        if (child) {
            if (strcmp(id, child->id()) == 0) return child;
        }
        ArduJAXContainer* childlist = _children.members[i]->toContainer();
        if (childlist) {
            child = childlist->findChild(id);
            if (child) return child;
        }
    }
}

/** @param id: The id for the element. Note that the string is not copied. Do not use a temporary string in this place. Also, do keep it short. */
ArduJAXElement::ArduJAXElement(const char* id) : ArduJAXBase() {
    _id = id;
    _flags = StatusVisible;
    revision = 0;
}

bool ArduJAXElement::sendUpdates(uint16_t since, bool first) {
    if (!changed(since)) return false;
    if (!first) _driver->printContent(",\n");
    _driver->printContent("{\n\"id\": \"");
    _driver->printContent(_id);
    _driver->printContent("\",\n\"changes\": [");
    for (int8_t i = -2; i < additionalPropertyCount(); ++i) {
        if (i != -2) _driver->printContent(",");
        _driver->printContent("{\n\"set\": \"");
        _driver->printContent(propertyId(i));
        _driver->printContent("\",\n\"value\": \"");
        _driver->printContent(propertyValue(i));  // TODO: This will need quote-escaping. Probably best implemented in a dedicated function of the driver.
        _driver->printContent("\"\n}");
    }
    _driver->printContent("]\n}");
    setChanged();
    return true;
}

void ArduJAXElement::setVisible(bool visible) {
    if (visible == _flags & StatusVisible) return;
    setChanged();
    if (visible) _flags |= StatusVisible;
    else _flags -= _flags & StatusVisible;
}

void ArduJAXElement::setChanged() {
    revision = _driver->setChanged();
}

bool ArduJAXElement::changed(uint16_t since) {
    if ((revision + 40000) < since) revision = since + 1;    // basic overflow protection. Results in sending _all_ states at least every 40000 request cycles
    return (revision > since);
}


//////////////////////// ArduJAXMutableSpan /////////////////////////////

void ArduJAXMutableSpan::print() const {
    _driver->printContent("<span id=\"");
    _driver->printContent(_id);
    _driver->printContent("\">");
    if (_value) _driver->printContent(_value);  // TODO: quoting
    _driver->printContent("</span>\n");
}

const char* ArduJAXMutableSpan::value() {
    return _value;
}

const char* ArduJAXMutableSpan::valueProperty() const {
    return "innerHTML";
}

void ArduJAXMutableSpan::setValue(const char* value) {
    _value = value;
    setChanged();
}


//////////////////////// ArduJAXSlider /////////////////////////////

ArduJAXSlider::ArduJAXSlider(const char* id, int16_t min, int16_t max, int16_t initial) : ArduJAXElement(id) {
    _value = initial;
    _min = min;
    _max = max;
}

void ArduJAXSlider::print() const {
    _driver->printContent("<input id=\"");
    _driver->printContent(_id);
    _driver->printContent("\" type=\"range\" min=\"");
    _driver->printContent(itoa(_min, itoa_buf, 10));
    _driver->printContent("\" max=\"");
    _driver->printContent(itoa(_max, itoa_buf, 10));
    _driver->printContent("\" value=\"");
    _driver->printContent(itoa(_value, itoa_buf, 10));
    _driver->printContent("\" onChange=\"doRequest(this.id, this.value);\"/>");
}

const char* ArduJAXSlider::value() {
    itoa(_value, itoa_buf, 10);
}

void ArduJAXSlider::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _value = atoi(buf);
}

const char* ArduJAXSlider::valueProperty() const {
    return "value";
}

void ArduJAXSlider::setValue(int16_t value) {
    _value = value;
    setChanged();
}

//////////////////////// ArduJAXCheckButton /////////////////////////////

ArduJAXCheckButton::ArduJAXCheckButton(const char* id, const char* label, bool checked) : ArduJAXElement(id) {
    _label = label;
    _checked = checked;
}

void ArduJAXCheckButton::print() const {
    _driver->printContent("<input id=\"");
    _driver->printContent(_id);
    _driver->printContent("\" type=\"checkbox\" autocomplete=\"off\" value=\"t\"");
    if (_checked) _driver->printContent(" checked=\"true\"");
    _driver->printContent(" onChange=\"doRequest(this.id, this.checked ? 't' : 'f');\"/>");
    _driver->printContent("<label for=\"");
    _driver->printContent(_id);
    _driver->printContent("\">");
    _driver->printContent(_label);
    _driver->printContent("</label>");
}

const char* ArduJAXCheckButton::value() {
    return _checked ? "true" : "";
}

void ArduJAXCheckButton::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _checked = (buf[0] == 't');
}

const char* ArduJAXCheckButton::valueProperty() const {
    return "checked";
}

void ArduJAXCheckButton::setChecked(bool checked) {
    _checked = checked;
    setChanged();
}
