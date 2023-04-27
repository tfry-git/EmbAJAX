/*
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
*/

#ifndef EMBAJAXSCRIPTEDSPAN_H
#define EMBAJAXSCRIPTEDSPAN_H

#include "EmbAJAX.h"

/** @brief A span element containing a custom javascript script, meant to creating custom dispays
 * 
 * This class creates a <span> element containing a custom script. The script to
 * use is passed in the constructor. The span element itself is accessible using
 * "this", inside the script.
 * 
 * The script may contain "this.receiveValue = function(value) { [...] };". This
 * function will then be called, when the value is changed from the server.
 * 
 * If the scripted object allows the user to make changes, the script should call
 * "this.sendValue(value);", when the value has changed. You will also have to set a
 * large enough receive buffer in the constructor in this case!
 * 
 * @warning This class is new, and its API may not be quite stable at the time of this
 *          writing. Feedback welcome.
 */
class EmbAJAXScriptedSpan : public EmbAJAXElement {
public:
    /** Constructor. See the class description for some detail on how to provide
     * a script.
     *
     * @param id unique id of the span
     * @param script the script code. See the class description for details.
     * @param rec_buffer if the script supplies values to the server (via @code this.sendValue(x); @endcode in the script), a receive buffer is needed hold that value. Pass a pointer to a suitably sized buffer.
     * @param rec_buffer_size size of the rec_buffer.
     */
    EmbAJAXScriptedSpan(const char* id, const char* script, char* rec_buffer = 0, size_t rec_buffer_size=0) : EmbAJAXElement(id) {
        _value = EmbAJAXBase::null_string;
        _script = script;
        _rec_buffer = rec_buffer;
        _rec_buffer_size = rec_buffer_size;
    }
    void print() const override {
        _driver->printFormatted("<span id=", HTML_QUOTED_STRING(_id), "><script>{\n"
                              "let spn=document.getElementById(", JS_QUOTED_STRING(_id), ");\n"
                              "Object.defineProperty(spn, 'EmbAJAXValue', {\n"
                              "  set: function(value) {\n"
                              "    if (this.receiveValue) this.receiveValue(value);\n"
                              "  }\n"
                              "})\n"
                              "spn.sendValue = function(value) {\n"
                              "  doRequest(this.id, value);\n"
                              "}\n"
                              "spn.init=function() {\n",
                              PLAIN_STRING(_script),
                              "\n};\n"
                              "spn.init();\n"
                              "spn.EmbAJAXValue=", JS_QUOTED_STRING(_value), ";\n"
                              "}</script></span>\n");
    }
    const char* value(uint8_t which = EmbAJAXBase::Value) const override {
        if (which == EmbAJAXBase::Value) return _value;
        return EmbAJAXElement::value(which);
    }
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override {
        if (which == EmbAJAXBase::Value) return "EmbAJAXValue";
        return EmbAJAXElement::valueProperty(which);
    }
    /** Send the given value to the client side script. Note that if you call this very
     *  often, the client will probably not see every value. It will only get to see
     *  the latest value that was set on each poll.
     * 
     *  Further note that as of this writing, EmbAJAX does not check whether the value
     *  is actually changed, when you call this. You can avoid network overhead by
     *  making sure to call setValue(), only when something has actually changed. This
     *  may be changed in a future version of EmbAJAX.
     * 
     *  For safety, the value string is always quoted when sending it to the client. This
     *  is not a problem as long as you are sending strings or plain numbers. To send more
     *  complex objects (such as an array, or even a function), your receiveValue() function
     *  should contain a call to eval().
     *
     *  @param value: Note: The string is not copied, so don't make this a temporary. */
    void setValue(const char* value) {
        // TODO: Check whether the value has actually changed
        _value = value;
        setChanged();
    };
    
    void updateFromDriverArg(const char* argname) override {
        _driver->getArg(argname, _rec_buffer, _rec_buffer_size);
        _value = _rec_buffer;
    }
private:
    const char* _value;
    const char* _script;
    char* _rec_buffer;
    size_t _rec_buffer_size;
};

#endif
