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
class ArduJAXContainerBase;

/** @brief Abstract base class for anything shown on an ArduJAXPage
 *
 *  Anything that can be displayed on an ArduJAXPage will have to inherit from this class
 *  (or be wrapped in something inherited from this class @see ArduJAXStatic). */
class ArduJAXBase {
public:
    virtual void print() const = 0;
    /** Set the driver. You do _not_ have to call this, except if you actually want to switch
     *  between several drivers at runtime. */
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
    virtual bool sendUpdates(uint16_t since, bool first) {
        return false;
    }
    /** Cast this object to ArduJAXElement if it is a controllable element.
     *  @return 0, if this is not a controllable element. */
    virtual ArduJAXElement* toElement() {
        return 0;
    }
    /** Set visibility of this element. Note not all ArduJAXBase-objects support this. Importantly,
     *  ArduJAXStatic does not. Provided in the base class for efficiency. */
    void setVisible(bool visible) {
        setBasicProperty(Visibility, visible);
    }
    /** Set enabledness state of this element. Note not all ArduJAXBase-objects support this. Importantly,
     *  ArduJAXStatic does not. Provided in the base class for efficiency. */
    void setEnabled(bool enabled) {
        setBasicProperty(Enabledness, enabled);
    }
    enum Property {
        Visibility=0,
        Enabledness=1,
        Value=2,
        FirstElementSpecificProperty=3,
        HTMLAllowed=7
    };
    /** Find child element of this one, with the given id. Returns 0, if this is not a container, or
     *  does not have such a child. @see ArduJAXContainer, and @see ArduJAXHideableContainer. */
    virtual ArduJAXElement* findChild(const char*id) const {
        return 0;
    }
protected:
template<size_t NUM> friend class ArduJAXContainer;
    virtual void setBasicProperty(uint8_t num, bool status) {};

    static ArduJAXOutputDriverBase *_driver;
    static char itoa_buf[8];

    /** Filthy trick to keep (template) implementation out of the header. See ArduJAXContainer::printChildren() */
    void printChildren(ArduJAXBase** children, uint num) const;
    /** Filthy trick to keep (template) implementation out of the header. See ArduJAXContainer::sendUpdates() */
    bool sendUpdates(ArduJAXBase** children, uint num, uint16_t since, bool first);
    /** Filthy trick to keep (template) implementation out of the header. See ArduJAXContainer::findChild() */
    ArduJAXElement* findChild(ArduJAXBase** children, uint num, const char*id) const;
    /** Filthy trick to keep (template) implementation out of the header. See ArduJAXPage::print() */
    void printPage(ArduJAXBase** children, uint num, const char* _title, const char* _header) const;
    /** Filthy trick to keep (template) implementation out of the header. See ArduJAXPage::handleRequest() */
    void handleRequest(ArduJAXBase** children, uint num, void (*change_callback)());
};

/** @brief Abstract base class for output drivers/server implementations
 *
 *  Output driver as an abstraction over the server read/write commands.
 *  You will have to instantiate exactly one object of exactly one implementation,
 *  before using any ArduJAX classes.
 *
 *  Providing your own driver is very easy. All you have to do it to wrap the
 *  basic functions for writing to the server and retrieving (POST) arguments:
 *  printHeader(), printContent(), and getArg().
 */
class ArduJAXOutputDriverBase {
public:
    ArduJAXOutputDriverBase() {
        _revision = 1;
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
    /** Print the given value filtered according to the parameters:
     *
     *  @param quoted If true, add double-quotes around the string, and esacpe any double
     *                quotes within the string.
     *  @param HTMLescaped If true, escape any "<" and "&" in the input as "\&lt;" and "\&amp;"
     *                     such that it will appear as plain text if rendered as HTML
     *                     (safe for untrusted user input). */
    void printFiltered(const char* value, bool quoted, bool HTMLescaped);
    /** Shorthand for printFiltered(value, true, false); */
    inline void printQuoted (const char* value) { printFiltered (value, true, false); }
private:
    uint16_t _revision;
    uint16_t next_revision;
};

#if defined (ESP8266)  // TODO: Move this to extra header
#include <ESP8266WebServer.h>
/**  @brief Output driver implementation for ESP8266WebServer */
class ArduJAXOutputDriverESP8266 : public ArduJAXOutputDriverBase {
public:
    /** To register an ESP8266WebServer with ArduJAX, simply create a (globaL) instance of this class. */
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
        if (content[0] != '\0') _server->sendContent(content);  // NOTE: There seems to be a bug in the server when sending empty string.
    }
    const char* getArg(const char* name, char* buf, int buflen) override {
        _server->arg(name).toCharArray (buf, buflen);
        return buf;
    };
private:
    ESP8266WebServer *_server;
};
#endif

/** Convenience macro to set up an ArduJAXPage, without counting the number of elements for the template. See ArduJAXPage::ArduJAXPage()
 *  @param name Variable name of the page instance
 *  @param title HTML Title
 *  @param header_add a custom string to add to the HTML header section, e.g. a CSS definition. */
#define MAKE_ArduJAXPage(name, title, header_add, ...) \
    ArduJAXBase* name_elements[] = {__VA_ARGS__}; \
    ArduJAXPage<sizeof(name_elements)/sizeof(ArduJAXBase*)> name(name_elements, title, header_add);

/** @brief A static chunk of HTML
 *
 * This class represents a chunk of static HTML that will not be changed / cannot be interacted with. Neither from the client, nor from the server.
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

/** @brief connection status indicator
 *
 *  This passive element can be inserted into a page to indicate the connection status: If more than 5 client requests go unanswered, in a row,
 *  the connection to the server is assumed to be broken.
 *
 *  @note While this is a "dynamic" display, the entire logic is implemented on the client, for obvious reasons. From the point of view of the
 *        server, this is a static element. */
class ArduJAXConnectionIndicator : public ArduJAXBase {
public:
    /** c'tor. If you don't like the default status indications, you can pass the HTML to be shown for "ok" and "fail" states.
     *
     *  @param content_ok Value to show for OK state. May contain HTML markup. Leave as 0 for default.
     *  @param content_ok Value to show for broken state. May contain HTML markup. Leave as 0 for default. */
    ArduJAXConnectionIndicator(const char* content_ok = 0, const char* content_fail = 0) {
        _content_ok = content_ok;
        _content_fail = content_fail;
    }
    void print() const override;
private:
    const char* _content_ok;
    const char* _content_fail;
};

/** @brief Abstract base class for modifiable elements.
 *
 *  Abstract Base class for objects that can be changed, either from the server, or from both the client and the server.
 *  To create a derived class, you will need to provide appropriate implementations of print(), value(), and valueProperty().
 *  Further, you will most likely want to add a function like setValue() for control from the server. If the element is to
 *  receive updates from the client side, you will a) have to include an appropriate onChange-call in print(), and b) provide
 *  a non-empty implementation of updateFromDriverArg().
 *
 *  Best look at a simple example such as ArduJAXMutableSpan or ArduJAXSlider for details.
 */
class ArduJAXElement : public ArduJAXBase {
public:
    /** @param id: The id for the element. Note that the string is not copied. Do not use a temporary string in this place. Also, do keep it short. */
    ArduJAXElement(const char* id);

    const char* id() const {
        return _id;
    }
    bool sendUpdates(uint16_t since, bool first) override;

    /** const char representation of the current server side value. Must be implemented in derived class.
     *  This base class handles visibility and enabledness, only. Do call the base implementation for
     *  any "which" that is _not_ handled in your derived class. */
    virtual const char* value(uint8_t which = ArduJAXBase::Value) const {
        if (which == ArduJAXBase::Visibility) return (basicProperty (ArduJAXBase::Visibility) ? "initial" : "none");
        if (which == ArduJAXBase::Enabledness) return (basicProperty (ArduJAXBase::Enabledness) ? "" : "disabled");
        return 0;
    }

    /** Returns true, if the value may contain HTML, and needs HTML escaping when passed to the client.
     *  Base implementation simply returns false. */
    virtual bool valueNeedsEscaping(uint8_t which = ArduJAXBase::Value) const {
        return false;
    }

     /** The JS property that will have to be set on the client. Must be implemented in derived class.
      *  This base class handles visibility and enabledness, only. Do call the base implementation for
      *  any "which" that is _not_ handled in your derived class. */
    virtual const char* valueProperty(uint8_t which = ArduJAXBase::Value) const {
        if (which == ArduJAXBase::Visibility) return ("style.display");
        if (which == ArduJAXBase::Enabledness) return ("disabled");
        return 0;
    }

    /** override this in your derived class to allow updates to be propagated from client to server (if wanted).
     *  The implementation need not call setChanged(). */
    virtual void updateFromDriverArg(const char* argname) {
        return;
    }

    ArduJAXElement *toElement() override final {
        return this;
    }
protected:
    void setBasicProperty(uint8_t num, bool status) override;
    bool basicProperty(uint8_t num) const {
        return (_flags & (1 << num));
    }
template<size_t NUM> friend class ArduJAXPage;
friend class ArduJAXBase;
    const char* _id;
    void setChanged();
    bool changed(uint16_t since);
    /** Filthy trick to keep (template) implementation out of the header. See ArduJAXTextInput::print() */
    void printTextInput(uint size, const char* value) const;
private:
    byte _flags;
    uint16_t revision;
};

/** @brief An HTML span element with content that can be updated from the server (not the client) */
class ArduJAXMutableSpan : public ArduJAXElement {
public:
    ArduJAXMutableSpan(const char* id) : ArduJAXElement(id) {
        _value = 0;
    }
    void print() const override;
    const char* value(uint8_t which = ArduJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = ArduJAXBase::Value) const override;
    /** Set the <span>s content to the given value.
     *
     *  @param value: Note: The string is not copied, so don't make this a temporary.
     *  @param allowHTML: if true, you can set the content to any valid HTML string, which
     *                    allows for much flexibility, but is @em not safe when value is
     *                    untrusted user input.
     *                    if false (the default), any "<" and "&" in value will be escaped,
     *                    before rendering on the client, making the string plain but safe. */
    void setValue(const char* value, bool allowHTML = false);
    bool valueNeedsEscaping(uint8_t which=ArduJAXBase::Value) const override;
private:
    const char* _value;
};

/** @brief A text input field.
 *
 *  A text input field. The template parameter specifies the size (i.e. maximum number of chars)
 *  of the input field.
 *
 *  @note To limit the rate, and avoid conflicting update-conditions, when typing into the text field in the client,
 *        changes are sent to the server one second after the last key was pressed. This worked for me, best. */
template<size_t SIZE> class ArduJAXTextInput : public ArduJAXElement {
public:
    ArduJAXTextInput(const char* id) : ArduJAXElement(id) {
        _value[0] = '\0';
    }
    void print() const override {
        ArduJAXElement::printTextInput(SIZE, _value);
    }
    const char* value(uint8_t which = ArduJAXBase::Value) const override {
        if (which == ArduJAXBase::Value) return _value;
        return ArduJAXElement::value(which);
    }
    const char* valueProperty(uint8_t which = ArduJAXBase::Value) const override {
        if (which == ArduJAXBase::Value) return "value";
        return ArduJAXElement::valueProperty(which);
    }
    /** Set the text inputs content to the given value. Note: In this particular case, the value passed _is_ copied,
     *  you can safely pass a temporary string. */
    void setValue(const char* value) {
        strncpy(_value, value, SIZE);
    }
    void updateFromDriverArg(const char* argname) override {
        _driver->getArg(argname, _value, SIZE);
    }
private:
    char _value[SIZE];
};

/** @brief An HTML span element with content that can be updated from the server (not the client) */
class ArduJAXSlider : public ArduJAXElement {
public:
    ArduJAXSlider(const char* id, int16_t min, int16_t max, int16_t initial);
    void print() const override;
    const char* value(uint8_t which = ArduJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = ArduJAXBase::Value) const override;
    void setValue(int16_t value);
    int16_t intValue() const {
        return _value;
    }
    void updateFromDriverArg(const char* argname) override;
private:
    int16_t _min, _max, _value;
};

/** @brief A push-button.
 *
 *  When clicked a custom callback function will be called on the server. */
class ArduJAXPushButton : public ArduJAXElement {
public:
    /** @param text: @see setText(). HTML is allowed, here.
     *  @param callback Called when the button was clicked in the UI (with a pointer to the button as parameter) */
    ArduJAXPushButton(const char* id, const char* label, void (*callback)(ArduJAXPushButton*));
    void print() const override;
    /** Change the button text
     *
     *  @param value: Note: The string is not copied, so don't make this a temporary.
     *  @param allowHTML: if true, you can set the text to any valid HTML string, which
     *                    allows for much flexibility, but is @em not safe when value is
     *                    untrusted user input.
     *                    if false (the default), any "<" and "&" in value will be escaped,
     *                    before rendering on the client, making the string plain but safe. */
    void setText(const char* label, bool allowHTML = false);
    const char* value(uint8_t which = ArduJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = ArduJAXBase::Value) const override;
    bool valueNeedsEscaping(uint8_t which=ArduJAXBase::Value) const override;
    void updateFromDriverArg(const char* argname) override;
private:
    void (*_callback)(ArduJAXPushButton*);
    const char* _label;
};

class ArduJAXRadioGroupBase;

/** @brief A checkable (option) button.
 *
 *  A checkable button / box (NOTE: _Internally_ this is also used for radio buttons, however
 *  please do not rely on this implementation detail. */
class ArduJAXCheckButton : public ArduJAXElement {
public:
    ArduJAXCheckButton(const char* id, const char* label, bool checked=false);
    void print() const override;
    const char* value(uint8_t which = ArduJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = ArduJAXBase::Value) const override;
    void setChecked(bool checked);
    bool isChecked() const {
        return _checked;
    }
    void updateFromDriverArg(const char* argname) override;
private:
    bool _checked;
    const char* _label;
template<size_t NUM> friend class ArduJAXRadioGroup;
    ArduJAXCheckButton() : ArduJAXElement("") {};
    ArduJAXRadioGroupBase* radiogroup;
};

/** @brief abstract base for ArduJAXRadioGroup, needed for internal reasons. */
class ArduJAXRadioGroupBase {
protected:
    ArduJAXRadioGroupBase() {};
friend class ArduJAXCheckButton;
    virtual void selectOption(ArduJAXCheckButton* which) = 0;
    const char* _name;
};

/** @brief Base class for groups of objects */
template<size_t NUM> class ArduJAXContainer : public ArduJAXBase {
public:
    ArduJAXContainer(ArduJAXBase *children[NUM]) : ArduJAXBase() {
        _children = children;
    }
    void print() const override {
        ArduJAXBase::printChildren(_children, NUM);
    }
    bool sendUpdates(uint16_t since, bool first) override {
        return ArduJAXBase::sendUpdates(_children, NUM, since, first);
    }
    /** Recursively look for a child (hopefully, there is only one) of the given id, and return a pointer to it. */
    ArduJAXElement* findChild(const char*id) const override final {
        return ArduJAXBase::findChild(_children, NUM, id);
    }
protected:
    void setBasicProperty(uint8_t num, bool status) override {
        for (int i = 0; i < NUM; ++i) {
            _children[i]->setBasicProperty(num, status);
        }
    }
template<size_t> friend class ArduJAXHideableContainer;
    ArduJAXContainer() {};
    ArduJAXBase** _children;
};

/** @brief A list of objects that can be hidden, completely
 *
 *  This is _essentially_ an ArduJAXContainer with an id. The one advantage that this
 *  class has other ArduJAXContainer, is that it can be hidden _completely_, including
 *  any ArduJAXStatic objects inside it. On the client, the children of this element
 *  are encapsulated in a \<div> element. Other than this, it should behave identical
 *  to ArduJAXContainer.
 *
 *  You do _not_ need this class to hide an ArduJAXContainer that contains only ArduJAXElement
 *  derived objects, or standalone ArduJAXElement objects.
 *
 *  @note This is _not_ a derived class of ArduJAXContainer, to avoid adding virtual
 *        inheritance just for this. */
template<size_t NUM> class ArduJAXHideableContainer : public ArduJAXElement {
public:
    ArduJAXHideableContainer(const char* id, ArduJAXBase *children[NUM]) : ArduJAXElement(id) {
        _childlist = ArduJAXContainer<NUM>(children);
    }
    void print() const override {
        _driver->printContent("<div id=");
        _driver->printQuoted(_id);
        _driver->printContent(">");
        _childlist.print();
        _driver->printContent("</div>");
    }
    ArduJAXElement* findChild(const char* id) const override {
        return _childlist.findChild(id);
    }
    bool sendUpdates(uint16_t since, bool first) override {
        bool sent = ArduJAXElement::sendUpdates(since, first);
        bool sent2 = _childlist.sendUpdates(since, first && !sent);
        return sent || sent2;
    }
protected:
    void setBasicProperty(uint8_t num, bool status) override {
        ArduJAXElement::setBasicProperty(num, status);
        _childlist.setBasicProperty(num, status);
    }
    ArduJAXContainer<NUM> _childlist;
};

/** @brief A set of radio buttons (mutally exclusive buttons), e.g. for on/off, or low/mid/high, etc.
 *
 *  You can insert either the whole group into an ArudJAXPage at once, or - for more flexbile
 *  layouting - retrieve the individual buttons using() button, and insert them into the page
 *  as independent elements. */
template<size_t NUM> class ArduJAXRadioGroup : public ArduJAXContainer<NUM>, public ArduJAXRadioGroupBase {
public:
    /** ctor.
     *  @param id_base the "base" id. Internally, radio buttons with id_s id_base0, id_base1, etc. will be created.
     *  @param options labels for the options. Note: The @em array of options may be a temporary, but the option-strings themselves will have to be persistent!
     *  @param selected_option index of the default option. 0 by default, for the first option, may be > NUM, for
     *                         no option selected by default. */
    ArduJAXRadioGroup(const char* id_base, const char* options[NUM], uint8_t selected_option = 0) : ArduJAXContainer<NUM>(), ArduJAXRadioGroupBase() {
        for (uint8_t i = 0; i < NUM; ++i) {
            char* childid = childids[i];
            strncpy(childid, id_base, ARDUJAX_MAX_ID_LEN-4);
            itoa(i, &(childid[strlen(childid)]), 10);
            buttons[i] = ArduJAXCheckButton(childid, options[i], i == selected_option);
            buttons[i].radiogroup = this;
            buttonpointers[i] = &buttons[i];
        }
        _current_option = selected_option;
         ArduJAXContainer<NUM>::_children = buttonpointers;  // Hm, why do I need to specify ArduJAXContainer<NUM>::, explicitly?
        _name = id_base;
    }
    /** Select / check the option at the given index. All other options in this radio group will become deselected. */
    void selectOption(uint8_t num) {
        for (uint8_t i = 0; i < NUM; ++i) {
            buttons[i].setChecked(i == num);
        }
        _current_option = num;  // NOTE: might be outside of range, but that's ok, signifies "none selected"
    }
    /** @returns the index of the currently selected option. May be > NUM, if no option is selected. */
    uint8_t selectedOption() const {
        return _current_option;
    }
    /** @returns a representation of an individual option element. You can use this to insert the individual buttons
     *           at arbitrary positions in the page layout. */
    ArduJAXBase* button(uint8_t num) {
        if (num < NUM) return (&buttons[num]);
        return 0;
    }
private:
    ArduJAXCheckButton buttons[NUM]; /** NOTE: Internally, the radio groups allocates individual check buttons. This is the storage space for those. */
    ArduJAXBase* buttonpointers[NUM];
    char childids[NUM][ARDUJAX_MAX_ID_LEN];
    int8_t _current_option;
    void selectOption(ArduJAXCheckButton* which) override {
        _current_option = -1;
        for (uint8_t i = 0; i < NUM; ++i) {
            if (which == buttonpointers[i]) {
                _current_option = i;
            } else {
                buttons[i].setChecked(false);
            }
        }
    }
};

/** @brief Abstract base class for ArduJAXOptionSelect. */
class ArduJAXOptionSelectBase : public ArduJAXElement {
public:
    /** Select the option specified by index. */
    void selectOption(uint8_t num);
    /** @return the index of the currently selected option */
    uint8_t selectedOption() const;
    const char* value(uint8_t which = ArduJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = ArduJAXBase::Value) const override;
    void updateFromDriverArg(const char* argname) override;
protected:
    ArduJAXOptionSelectBase(const char*id, uint8_t current_option) : ArduJAXElement(id) {
        _current_option = current_option;
    };
    void print(const char* const* _labels, uint8_t NUM) const;
    uint8_t _current_option;
};

/** @brief Drop-down list of selectable options
 *
 *  Drop-down list of selectable options. Most functions of interest are implemented in the base class ArduJAXOptionSelectBase,
 *  you'll only use this class for the constructor. */
template<size_t NUM> class ArduJAXOptionSelect : public ArduJAXOptionSelectBase {
public:
    /** ctor.
     *  @param id id for the element
     *  @param labels labels for the options. Note: The @em array of options may be a temporary, but the option-strings themselves will have to be persistent!
     *  @param selected_option index of the default option. 0 by default, for the first option, may be > NUM, for no option selected by default. */
    ArduJAXOptionSelect(const char* id, const char* labels[NUM], uint8_t selected_option = 0) : ArduJAXOptionSelectBase(id, selected_option) {
        for (uint8_t i = 0; i < NUM; ++i) {
            _labels[i] = labels[i];
        }
    }
    void print() const override {
        ArduJAXOptionSelectBase::print(_labels, NUM);
    }
private:
    const char* _labels[NUM];
};

/** @brief The main interface class
 *
 *  This is the main interface class. Create a web-page with a list of elements on it, and arrange for
 *  print() (for page loads) adn handleRequest() (for AJAX calls) to be called on requests. By default,
 *  both page loads, and AJAX are handled on the same URL, but the first via GET, and the second
 *  via POST. */
template<size_t NUM> class ArduJAXPage : public ArduJAXContainer<NUM> {
public:
    /** Create a web page.
     *  @param children list of elements on the page
     *  @param title title (may be 0). This string is not copied, please do not use a temporary string.
     *  @param header_add literal text (may be 0) to be added to the header, e.g. CSS (linked or in-line). This string is not copied, please do not use a temporary string). */
    ArduJAXPage(ArduJAXBase* children[NUM], const char* title, const char* header_add = 0) : ArduJAXContainer<NUM>(children) {
        _title = title;
        _header_add = 0;
    }
    /** Serve the page including headers and all child elements. You should arrange for this function to be called, whenever
     *  there is a GET request to the desired URL. */
    void print() const override {
        ArduJAXBase::printPage(ArduJAXContainer<NUM>::_children, NUM, _title, _header_add);
    }
    /** Handle AJAX client request. You should arrange for this function to be called, whenever there is a POST request
     *  to whichever URL you served the page itself, from.
     *
     *  @param change_callback If some value has changed in the client, this function will be called. While it is optional
     *                         to specify this, if there are any changes that may need to be sent back to the client in
     *                         response to the change, you should specify this function, and handle the change inside it.
     *                         This way, an update can be sent back to the client, immediately, for a smooth UI experience.
     *                         (Otherwise the client will be updated on the next poll). */
    void handleRequest(void (*change_callback)()=0) {
        ArduJAXBase::handleRequest(ArduJAXContainer<NUM>::_children, NUM, change_callback);
    }
protected:
    const char* _title;
    const char* _header_add;
};

#endif
