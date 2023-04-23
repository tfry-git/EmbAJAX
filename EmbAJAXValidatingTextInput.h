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

#include "EmbAJAX.h"

template<size_t SIZE> class EmbAJAXValidatingTextInput : public EmbAJAXTextInput<SIZE> {
public:
    EmbAJAXValidatingTextInput(const char* id) : EmbAJAXTextInput<SIZE>(id) {
        _attributes = EmbAJAXBase::null_string;
        _placeholder = nullptr;
        _pattern = nullptr;
    }
    void print() const override {
        EmbAJAXBase::_driver->printContentF("<input type=\"text\" id=" HTML_QUOTED_STRING_ARG " maxLength=" INTEGER_VALUE_ARG " size=" INTEGER_VALUE_ARG " " PLAIN_STRING_ARG,
                                           EmbAJAXTextInput<SIZE>::_id, SIZE-1, min(max(SIZE, (size_t) 11), (size_t) 41) - 1, _attributes);
        if (EmbAJAXTextInput<SIZE>::_value[0] != '\0') {
            EmbAJAXBase::_driver->printAttribute("value", EmbAJAXTextInput<SIZE>::_value);
        }
        if (_placeholder != 0) {
            EmbAJAXBase::_driver->printAttribute("placeholder", _placeholder);
        }
        if (_pattern != 0) {
            EmbAJAXBase::_driver->printAttribute("pattern", _pattern);
        }
        EmbAJAXBase::_driver->printContent(" onInput=\"doRequest(this.id, this.value); this.checkValidity();\"/>");
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
