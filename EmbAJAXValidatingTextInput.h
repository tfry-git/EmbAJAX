/**
 * 
 * EmbAJAX - Simplistic framework for creating and handling displays and controls on a WebPage served by an Arduino (or other small device).
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

#ifndef EMBAJAXVALIDATINGTEXTINPUT_H
#define EMBAJAXVALIDATINGTEXTINPUT_H

#include <EmbAJAX.h>

template<size_t SIZE> class EmbAJAXValidatingTextInput : public EmbAJAXTextInput<SIZE> {
public:
    EmbAJAXValidatingTextInput(const char* id) : EmbAJAXTextInput<SIZE>(id) {
        _attributes = 0;
        _placeholder = 0;
        _pattern = 0;
    }
    void print() const override {
        EmbAJAXBase::_driver->printContent("<input type=\"text\"");
        EmbAJAXBase::_driver->printAttribute("id", EmbAJAXTextInput<SIZE>::_id);
        EmbAJAXBase::_driver->printAttribute("maxLength", SIZE-1);
        EmbAJAXBase::_driver->printAttribute("size", min(max(abs(SIZE-1), 10),40));  // Arbitray limit for rendered width of text fields: 10..40 chars
        if (EmbAJAXTextInput<SIZE>::_value[0] != '\0') {
            EmbAJAXBase::_driver->printAttribute("value", EmbAJAXTextInput<SIZE>::_value);
        }
        if (_placeholder != 0) {
            EmbAJAXBase::_driver->printAttribute("placeholder", _placeholder);
        }
        if (_attributes != 0) {
            EmbAJAXBase::_driver->printContent(_attributes);
        }
        if (_pattern != 0) {
            EmbAJAXBase::_driver->printAttribute("pattern", _pattern);
        }
        // Using onChange to update is too awkward. Using plain onInput would generate too may requests (and often result in "eaten" characters). Instead,
        // as a compromise, we arrange for an update one second after the last key was pressed.
        EmbAJAXBase::_driver->printContent(" onInput=\"clearTimeout(this.debouncer); this.debouncer=setTimeout(function() {doRequest(this.id, this.value);}.bind(this),1000); this.checkValidity();\"/>");
    }
    /** Set a placeholder text (will be shown, when the input is empty) */
    void setPlaceholder(const char* placeholder) {
        _placeholder = placeholder;
    }
    /** Set a placeholder text (will be shown, when the input is empty)
     * 
     *  Example: @code
     *     setPattern("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}"); // IPv4-Address
     *  @endcode
     */
    void setPattern(const char* pattern) {
        _pattern = pattern;
    }
    /** Specify custom attributes (other than placeholder and pattern)
     *  to be inserted in the \<input>-tag in the generated HTML.
     * 
     *  Example: @code
     *     setCustomAttributes("min=\"1\" max=\"100\"");
     *  @endcode
     */
    void setCustomValidationAttributes(const char* attributes) {
        _attributes = attributes;
    }
private:
    const char* _attributes;
    const char* _placeholder;
    const char* _pattern;
};

#endif
