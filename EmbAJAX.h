/*
 * 
 * EmbAJAX - Simplistic framework for creating and handling displays and controls on a WebPage served by an Arduino (or other small device).
 * 
 * Copyright (C) 2018-2023 Thomas Friedrichsmeier
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
#ifndef EMBAJAX_H
#define EMBAJAX_H

#include <Arduino.h>

/** Maximum length to assume for id strings. Reducing this could help to reduce RAM usage, a little. */
#define EMBAJAX_MAX_ID_LEN 16

/** \def EMBAJAX_DEBUG
 * Set to a value above 0 for diagnostics on Serial and browser console (for troubleshooting, only, as it increase flash, RAM, and processing requirements,
 * considerably. */
// #define EMBAJAX_DEBUG 3

/**V@file EmbAJAX.h
 *
 * Main include file.
 */

/** \def USE_PROGMEM_STRINGS
 * Control storage of string constants
 *
 * On some MCU-architectures, RAM and FLASH reside in two logically distinct address spaces. This implies that regular const char* strings
 * need to be copied into RAM address space, even if they are fully static. EmbAJAX needs many string constants, and is therefore quite affected
 * by this problem.
 *
 * The Arduino F() macro helps to work around this (further reading, there), but does incur a small performance penalty, which can be avoided if
 * a) RAM usage is not an issue, or b) the MCU uses a unified address space.
 *
 * This define controls whether (most) static strings in EmbAJAX will be wrapped into the Arduino F() macro:
 *   - 1 - always
 *   - 0 - never
 *   - undefined - based on auto-detected CPU arch (this is the default) */
//#define USE_PROGMEM_STRINGS 0

#if !defined USE_PROGMEM_STRINGS
 #if defined(memcpy_P) && (memcpy_P == memcpy)
  #define USE_PROGMEM_STRINGS 0
 #else
  #define USE_PROGMEM_STRINGS 1
 #endif
#endif

#include "macro_definitions.h"

class EmbAJAXOutputDriverBase;
class EmbAJAXElement;
class EmbAJAXPage;

/** @brief Abstract base class for anything shown on an EmbAJAXPage
 *
 *  Anything that can be displayed on an EmbAJAXPage will have to inherit from this class
 *  (or be wrapped in something inherited from this class @see EmbAJAXStatic). */
class EmbAJAXBase {
public:
    virtual void print() const = 0;
    /** Set the driver. You do _not_ have to call this, except if you actually want to switch
     *  between several drivers at runtime. */
    static void setDriver(EmbAJAXOutputDriverBase *driver) {
        _driver = driver;
    }
    /** serialize pending changes for the client. Virtual so you could customize it, completely, but
     *  instead you probably want to override EmbAJAXElement::valueProperty(), only, instead.
     *
     *  @param since revision number last sent to the server. Send only changes that occured since this revision.
     *  @param first if false, @em and this object writes any update, it should write a ',', first.
     *  @returns true if anything has been written, false otherwise.
     */
    virtual bool sendUpdates(uint16_t since, bool first) {
        return false;
    }
    /** Cast this object to EmbAJAXElement if it is a controllable element.
     *  @return 0, if this is not a controllable element. */
    virtual EmbAJAXElement* toElement() {
        return 0;
    }
    /** Set visibility of this element. Note not all EmbAJAXBase-objects support this. Importantly,
     *  EmbAJAXStatic does not. Provided in the base class for efficiency. */
    void setVisible(bool visible) {
        setBasicProperty(Visibility, visible);
    }
    /** Set enabledness state of this element. Note not all EmbAJAXBase-objects support this. Importantly,
     *  EmbAJAXStatic does not. Provided in the base class for efficiency. */
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
     *  does not have such a child. @see EmbAJAXElementList, and @see EmbAJAXHideableContainer. */
    virtual EmbAJAXElement* findChild(const char*id) const {
        return 0;
    }
protected:
template<size_t NUM> friend class EmbAJAXContainer;
friend class EmbAJAXElementList;
    virtual void setBasicProperty(uint8_t num, bool status) {};

    static EmbAJAXOutputDriverBase *_driver;
    static char itoa_buf[8];
    constexpr static const char null_string[1] = "";

    // Note: The following can be moved into EmbAJAXElementList once EmbAJAXContainer is removed for good
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXElementList::printChildren() */
    void printChildren(EmbAJAXBase* const* children, size_t num) const;
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXElementList::sendUpdates() */
    bool sendUpdates(EmbAJAXBase* const* children, size_t num, uint16_t since, bool first);
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXElementList::findChild() */
    EmbAJAXElement* findChild(EmbAJAXBase* const* children, size_t num, const char*id) const;
};

/** @brief Abstract base class for output drivers/server implementations
 *
 *  Output driver as an abstraction over the server read/write commands.
 *  You will have to instantiate exactly one object of exactly one implementation,
 *  before using any EmbAJAX classes.
 *
 *  Providing your own driver is very easy. All you have to do it to wrap the
 *  basic functions for writing to the server and retrieving (POST) arguments:
 *  printHeader(), printContent(), and getArg().
 */
class EmbAJAXOutputDriverBase {
public:
    EmbAJAXOutputDriverBase() {
        _revision = 1;
        next_revision = _revision;
    }

    virtual void printHeader(bool html) = 0;
    virtual void printContent(const char *content) = 0;
    virtual const char* getArg(const char* name, char* buf, int buflen) = 0;
    /** Set up the given page to be served on the given path.
     *
     *  @param change_callback See EmbAJAXPage::handleRequest() for details.
     */
    virtual void installPage(EmbAJAXPage *page, const char *path, void (*change_callback)()=0) = 0;
    /** Insert this hook into loop(). Takes care of the appropriate server calls, if needed. */
    virtual void loopHook() = 0;

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
    /** Quotation modes. Used in printFiltered() */
    enum QuoteMode {
        NotQuoted,  ///< Will not be quoted
        JSQuoted,   ///< Will be quoted suitable for JavaScript
        HTMLQuoted  ///< Will be quoted suitable for HTML attributes
    };
    /** Print the given value filtered according to the parameters:
     *
     *  @note It is genarally recommended to use the #printFormatted(...) macro, wherever possible, instead of this.
     *
     *  @param quoted If true, add double-quotes around the string, and esacpe any double
     *                quotes within the string as &quot;.
     *  @param HTMLescaped If true, escape any "<" and "&" in the input as "\&lt;" and "\&amp;"
     *                     such that it will appear as plain text if rendered as HTML
     *                     (safe for untrusted user input). */
    void printFiltered(const char* value, QuoteMode quoted, bool HTMLescaped) {
        _printFiltered(value, quoted, HTMLescaped);
        commitBuffer();
    }
    /** Shorthand for printFiltered(value, JSQuoted, false); */
    inline void printJSQuoted (const char* value) { printFiltered (value, JSQuoted, false); }
    /** Shorthand for printFiltered(value, HTMLQuoted, false); */
    inline void printHTMLQuoted (const char* value) { printFiltered (value, HTMLQuoted, false); }
    /** Convenience function to print an attribute inside an HTML tag.
     *  This function adds a space _in front of_ the printed attribute.
     *
     *  @note It is genarally recommended to use the #printFormatted(...) macro, wherever possible, instead of this.
     *
     *  @param name name of the attribute
     *  @param value value of the attribute. Will be quoted. */
    void printAttribute(const char* name, const char* value);
    /** Convenience function to print an integer attribute inside an HTML tag.
     *  This function adds a space _in front of_ the printed attribute.
     *
     *  @note It is genarally recommended to use the #printFormatted(...) macro, wherever possible, instead of this.
     *
     *  @param name name of the attribute
     *  @param value value of the attribute. */
    void printAttribute(const char* name, const int32_t value);
    /** Print a static string with parameters replaced, somewhat similar to printf
     *
     *  @note It is genarally recommended to use the #printFormatted(...) macro, wherever possible, instead of this.
     *
     *  See there for further info. This function is really just the internal implementation, public for technical reasons.
    */
    void _printContentF(const char* fmt, ...);
#if USE_PROGMEM_STRINGS
    void _printContentF(const __FlashStringHelper*, ...);
#endif
private:
    void _printFiltered(const char* value, QuoteMode quoted, bool HTMLescaped);
    void _printContent(const char* content);
    void _printChar(const char content);
    void commitBuffer();
    const int _bufsize = 64;
    char _buf[64];
    int _bufpos = 0;
    uint16_t _revision;
    uint16_t next_revision;
};

EMBAJAX_DEPRECATED(2023_03_12, "Use EmbAJAXPage constructor, direclty") inline int MAKE_EmbAJAXPageDeprecated() { return 0; };
/** DEPRECATED: Convenience macro to set up an EmbAJAXPage, without counting the number of elements for the template. See EmbAJAXPage::EmbAJAXPage().
 *  @param name Variable name of the page instance
 *  @param title HTML Title
 *  @param header_add a custom string to add to the HTML header section, e.g. a CSS definition. */
#define MAKE_EmbAJAXPage(name, title, header_add, ...) \
    EmbAJAXBase* name##_elements[] = {__VA_ARGS__}; \
    EmbAJAXPage name(name##_elements, title, header_add, 100); \
    int name##_warning = MAKE_EmbAJAXPageDeprecated();

/** @brief A static chunk of HTML
 *
 * This class represents a chunk of static HTML that will not be changed / cannot be interacted with. Neither from the client, nor from the server.
 *  This does not have to correspond to a complete HTML element, it can be any fragment. */
class EmbAJAXStatic : public EmbAJAXBase {
public:
    /** ctor. Note: Content string is not copied. Don't make this a temporary. */
    EmbAJAXStatic(const char* content) {
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
 *  This passive element can be inserted into a page to indicate the connection status: If there is no reply from the server for 5 seconds,
 *  the connection to the server is assumed to be broken.
 *
 *  @note While this is a "dynamic" display, the entire logic is implemented on the client, for obvious reasons. From the point of view of the
 *        server, this is a static element. */
class EmbAJAXConnectionIndicator : public EmbAJAXBase {
public:
    /** c'tor. If you don't like the default status indications, you can pass the HTML to be shown for "ok" and "fail" states.
     *
     *  @param content_ok Value to show for OK state. May contain HTML markup. Default is "OK" on a green background.
     *  @param content_ok Value to show for broken state. May contain HTML markup. Default is "FAIL" on a green background. */
    EmbAJAXConnectionIndicator(const char* content_ok = default_ok, const char* content_fail = default_fail) {
        _content_ok = content_ok;
        _content_fail = content_fail;
    }
    void print() const override;
    static constexpr const char* default_ok = {"<span style=\"background-color:green;\">OK</span>"};
    static constexpr const char* default_fail = {"<span style=\"background-color:red;\">FAIL</span>"};
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
 *  Best look at a simple example such as EmbAJAXMutableSpan or EmbAJAXSlider for details.
 */
class EmbAJAXElement : public EmbAJAXBase {
public:
    /** @param id: The id for the element. Note that the string is not copied. Do not use a temporary string in this place. Also, do keep it short. */
    EmbAJAXElement(const char* id);

    const char* id() const {
        return _id;
    }
    bool sendUpdates(uint16_t since, bool first) override;

    /** const char representation of the current server side value. Must be implemented in derived class.
     *  This base class handles visibility and enabledness, only. Do call the base implementation for
     *  any "which" that is _not_ handled in your derived class. */
    virtual const char* value(uint8_t which = EmbAJAXBase::Value) const {
        if (which == EmbAJAXBase::Visibility) return (basicProperty (EmbAJAXBase::Visibility) ? "" : "none");
        if (which == EmbAJAXBase::Enabledness) return (basicProperty (EmbAJAXBase::Enabledness) ? "" : "disabled");
        return 0;
    }

    /** Returns true, if the value may contain HTML, and needs HTML escaping when passed to the client.
     *  Base implementation simply returns false. */
    virtual bool valueNeedsEscaping(uint8_t which = EmbAJAXBase::Value) const {
        return false;
    }

     /** The JS property that will have to be set on the client. Must be implemented in derived class.
      *  This base class handles visibility and enabledness, only. Do call the base implementation for
      *  any "which" that is _not_ handled in your derived class. */
    virtual const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const {
        if (which == EmbAJAXBase::Visibility) return ("style.display");
        if (which == EmbAJAXBase::Enabledness) return ("disabled");
        return 0;
    }

    /** override this in your derived class to allow updates to be propagated from client to server (if wanted).
     *  The implementation need not call setChanged(). */
    virtual void updateFromDriverArg(const char* argname) {
        return;
    }

    EmbAJAXElement *toElement() override final {
        return this;
    }
protected:
    void setBasicProperty(uint8_t num, bool status) override;
    bool basicProperty(uint8_t num) const {
        return (_flags & (1 << num));
    }
friend class EmbAJAXPage;
friend class EmbAJAXBase;
    byte _flags;
    const char* _id;
    void setChanged();
    bool changed(uint16_t since);
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXTextInput::print() */
    void printTextInput(size_t size, const char* value) const;
private:
    uint16_t revision;
};

/** @brief An HTML span element with content that can be updated from the server (not the client) */
class EmbAJAXMutableSpan : public EmbAJAXElement {
public:
    EmbAJAXMutableSpan(const char* id) : EmbAJAXElement(id) {
        _value = 0;
    }
    void print() const override;
    const char* value(uint8_t which = EmbAJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override;
    /** Set the <span>s content to the given value.
     *
     *  @param value: Note: The string is not copied, so don't make this a temporary.
     *  @param allowHTML: if true, you can set the content to any valid HTML string, which
     *                    allows for much flexibility, but is @em not safe when value is
     *                    untrusted user input.
     *                    if false (the default), any "<" and "&" in value will be escaped,
     *                    before rendering on the client, making the string plain but safe. */
    void setValue(const char* value, bool allowHTML = false);
    bool valueNeedsEscaping(uint8_t which=EmbAJAXBase::Value) const override;
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
template<size_t SIZE> class EmbAJAXTextInput : public EmbAJAXElement {
public:
    EmbAJAXTextInput(const char* id) : EmbAJAXElement(id) {
        _value[0] = '\0';
    }
    void print() const override {
        EmbAJAXElement::printTextInput(SIZE, _value);
    }
    const char* value(uint8_t which = EmbAJAXBase::Value) const override {
        if (which == EmbAJAXBase::Value) return _value;
        return EmbAJAXElement::value(which);
    }
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override {
        if (which == EmbAJAXBase::Value) return "value";
        return EmbAJAXElement::valueProperty(which);
    }
    /** Set the text inputs content to the given value. Note: In this particular case, the value passed _is_ copied,
     *  you can safely pass a temporary string. */
    void setValue(const char* value) {
        strncpy(_value, value, SIZE);
        setChanged();
    }
    void updateFromDriverArg(const char* argname) override {
        _driver->getArg(argname, _value, SIZE);
    }
protected:
    char _value[SIZE];
};

/** @brief An HTML span element with content that can be updated from the server (not the client) */
class EmbAJAXSlider : public EmbAJAXElement {
public:
    EmbAJAXSlider(const char* id, int16_t min, int16_t max, int16_t initial);
    void print() const override;
    const char* value(uint8_t which = EmbAJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override;
    void setValue(int16_t value);
    int16_t intValue() const {
        return _value;
    }
    void updateFromDriverArg(const char* argname) override;
private:
    int16_t _min, _max, _value;
};

/** @brief A color picker element (\<input type="color">) */
class EmbAJAXColorPicker : public EmbAJAXElement {
public:
    /** c'tor.
     *  @param r Initial value for red
     *  @param g Initial value for green
     *  @param b Initial value for blue */
    EmbAJAXColorPicker(const char* id, uint8_t r, uint8_t g, uint8_t b);
    void print() const override;
    const char* value(uint8_t which = EmbAJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override;
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    uint8_t red() const;
    uint8_t green() const;
    uint8_t blue() const;
    void updateFromDriverArg(const char* argname) override;
private:
    uint8_t _r, _g, _b;
};

/** @brief A push-button.
 *
 *  When clicked a custom callback function will be called on the server. */
class EmbAJAXPushButton : public EmbAJAXElement {
public:
    /** @param label: @see setText(). HTML is allowed, here.
     *  @param callback Called when the button was clicked in the UI (with a pointer to the button as parameter) */
    EmbAJAXPushButton(const char* id, const char* label, void (*callback)(EmbAJAXPushButton*));
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
    const char* value(uint8_t which = EmbAJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override;
    bool valueNeedsEscaping(uint8_t which=EmbAJAXBase::Value) const override;
    void updateFromDriverArg(const char* argname) override;
protected:
    void (*_callback)(EmbAJAXPushButton*);
    const char* _label;
};

/** @brief A momentary "press-and-hold" button.
 *
 *  This button will be return status()==Pressed while the button is actively held pressed in a client.
 *  While the button is pressed, the client will send continuous "pings" to the server. If a button
 *  press has been registered, but the latest ping has timed out, the status() will return MayBePressed,
 *  instead, so your code can take select an appropriate fallback behavior in case of unreliable connections
 *  (e.g. stopping an RC car).
 */
class EmbAJAXMomentaryButton : public EmbAJAXPushButton {
public:
    /** @param text: @see setText(). HTML is allowed, here.
     *  @param ping_interval: While the button is pressed, the client will send "ping" messages every timeout/1.5 milliseconds.
     *                        If no ping has been received within timeout, status() will return MaybePressed.
     *  @param callback Called when the button was clicked or relased in the UI (with a pointer to the button as parameter)
     *                  @em Not called, when a ping has timed out. */
    EmbAJAXMomentaryButton(const char* id, const char* label, uint16_t timeout=600, void (*callback)(EmbAJAXPushButton*)=0);
    void print() const override;
    enum Status {
        Pressed,
        MaybePressed,
        Released
    };
    Status status() const;
    void updateFromDriverArg(const char* argname) override;
private:
    uint32_t latest_ping;
    uint16_t _timeout;
};

class EmbAJAXRadioGroupBase;

/** @brief A checkable (option) button.
 *
 *  A checkable button / box (NOTE: _Internally_ this is also used for radio buttons, however
 *  please do not rely on this implementation detail. */
class EmbAJAXCheckButton : public EmbAJAXElement {
public:
    EmbAJAXCheckButton(const char* id, const char* label, bool checked=false);
    void print() const override;
    const char* value(uint8_t which = EmbAJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override;
    void setChecked(bool checked);
    bool isChecked() const {
        return _checked;
    }
    void updateFromDriverArg(const char* argname) override;
private:
    bool _checked;
    const char* _label;
template<size_t NUM> friend class EmbAJAXRadioGroup;
    EmbAJAXCheckButton() : EmbAJAXElement("") {};
    EmbAJAXRadioGroupBase* radiogroup;
};

/** @brief abstract base for EmbAJAXRadioGroup, needed for internal reasons. */
class EmbAJAXRadioGroupBase {
protected:
    EmbAJAXRadioGroupBase() {};
friend class EmbAJAXCheckButton;
    virtual void selectButton(EmbAJAXCheckButton* which) = 0;
    const char* _name;
};

/** @brief Base class for groups of objects. Deprecated. Use EmbAJAXElementList, instead. */
template<size_t NUM> class EMBAJAX_DEPRECATED(2023_03_12, "Use EmbAJAXElementList, instead") EmbAJAXContainer : public EmbAJAXBase {
public:
    EMBAJAX_DEPRECATED(2023_03_12, "Use EmbAJAXElementList, instead") EmbAJAXContainer(EmbAJAXBase *children[NUM]) : EmbAJAXBase() {
        _children = children;
    }
    void print() const override {
        EmbAJAXBase::printChildren(_children, NUM);
    }
    bool sendUpdates(uint16_t since, bool first) override {
        return EmbAJAXBase::sendUpdates(_children, NUM, since, first);
    }
    /** Recursively look for a child (hopefully, there is only one) of the given id, and return a pointer to it. */
    EmbAJAXElement* findChild(const char*id) const override final {
        return EmbAJAXBase::findChild(_children, NUM, id);
    }
protected:
    void setBasicProperty(uint8_t num, bool status) override {
        for (uint8_t i = 0; i < NUM; ++i) {
            _children[i]->setBasicProperty(num, status);
        }
    }

    EmbAJAXContainer() {};
    EmbAJAXBase** _children;
};

/** @brief A container for a group of elements
 *
 * This is a modernized alternative to EmbAJAXContainer, without requiring a template parameter.
 */
class EmbAJAXElementList : public EmbAJAXBase {
public:
    /** constructor taking a static array of elements */
    template<size_t N> constexpr EmbAJAXElementList(EmbAJAXBase* (&children)[N]) : _children(children), NUM(N) {};
    /** constructor taking an array of elements with a size that cannot be determined at compile time. In this case, you'll have to specify the size, as the first parameter */
    constexpr EmbAJAXElementList(size_t childcount, EmbAJAXBase* const* children) :
        EmbAJAXBase(),
        _children(children),
        NUM(childcount) {}
    /** constructor taking list of pointers to elements */
    // Note: "first" forces all args to be EmbAJAXBase
    template<class... T> constexpr EmbAJAXElementList(EmbAJAXBase* first, T*... elements) :
        EmbAJAXBase(),
        _children(new EmbAJAXBase*[sizeof...(elements) + 1] {first, elements...}),
        NUM(sizeof...(elements) + 1) {}
#warning proper d'tor! Perhaps const init arrays should not be accepted at all? EmbAJAXPage c'tor should be the only API entry point where we need backwards compatibility
    void print() const override {
        EmbAJAXBase::printChildren(_children, NUM);
    }
    bool sendUpdates(uint16_t since, bool first) override {
        return EmbAJAXBase::sendUpdates(_children, NUM, since, first);
    }
    /** Recursively look for a child (hopefully, there is only one) of the given id, and return a pointer to it. */
    EmbAJAXElement* findChild(const char*id) const override final {
        return EmbAJAXBase::findChild(_children, NUM, id);
    }
    size_t size() const {
        return NUM;
    }
    EmbAJAXBase* getChild(const size_t index) const {
        return _children[index];
    }
protected:
friend class EmbAJAXHideableContainer;
    constexpr EmbAJAXElementList(size_t N) : _children(nullptr), NUM(N) {};
    void setBasicProperty(uint8_t num, bool status) override {
        for (size_t i = 0; i < NUM; ++i) {
            _children[i]->setBasicProperty(num, status);
        }
    }
    EmbAJAXBase* const* _children;
    size_t NUM; // TODO: make me const, again
};

/** @brief A list of objects that can be hidden, completely
 *
 *  This is _essentially_ an EmbAJAXElementList with an id. The one advantage that this
 *  class has other EmbAJAXElementList, is that it can be hidden _completely_, including
 *  any EmbAJAXStatic objects inside it. On the client, the children of this element
 *  are encapsulated in a \<div> element. Other than this, it should behave identical
 *  to EmbAJAXElementList.
 *
 *  You do _not_ need this class to hide an EmbAJAXElementList that contains only EmbAJAXElement
 *  derived objects, or standalone EmbAJAXElement objects.
 *
 *  @note This is _not_ a derived class of EmbAJAXElementList, to avoid adding virtual
 *        inheritance just for this. */
class EmbAJAXHideableContainer : public EmbAJAXElement {
public:
    template<int NUM> EmbAJAXHideableContainer(const char* id, EmbAJAXBase *(&children)[NUM]) : EmbAJAXElement(id), _childlist(children) {}
    void print() const override {
        _driver->printFormatted("<div id=", HTML_QUOTED_STRING(_id), ">");
        _childlist.print();
        _driver->printContent("</div>");
    }
    /** constructor taking an array of elements with a size that cannot be determined at compile time. In this case, you'll have to specify the size, as the first parameter */
    EmbAJAXHideableContainer(const char* id, size_t childcount, EmbAJAXBase* const* children) : EmbAJAXElement(id), _childlist(childcount, children) {}
    /** constructor taking list of pointers to elements */
    template<class... T> EmbAJAXHideableContainer(const char* id, EmbAJAXBase* first, T*... elements) : EmbAJAXElement(id), _childlist(first, elements...) {}

    EmbAJAXElement* findChild(const char* id) const override {
        return _childlist.findChild(id);
    }
    bool sendUpdates(uint16_t since, bool first) override {
        bool sent = EmbAJAXElement::sendUpdates(since, first);
        bool sent2 = _childlist.sendUpdates(since, first && !sent);
        return sent || sent2;
    }
protected:
    void setBasicProperty(uint8_t num, bool status) override {
        EmbAJAXElement::setBasicProperty(num, status);
        _childlist.setBasicProperty(num, status);
    }
    EmbAJAXElementList _childlist;
};

/** @brief A set of radio buttons (mutally exclusive buttons), e.g. for on/off, or low/mid/high, etc.
 *
 *  You can insert either the whole group into an EmbAJAXPage at once, or - for more flexbile
 *  layouting - retrieve the individual buttons using() button, and insert them into the page
 *  as independent elements. */
template<size_t N> class EmbAJAXRadioGroup : public EmbAJAXElementList, public EmbAJAXRadioGroupBase {
public:
    /** ctor.
     *  @param id_base the "base" id. Internally, radio buttons with id_s id_base0, id_base1, etc. will be created.
     *  @param options labels for the options. Note: The @em array of options may be a temporary, but the option-strings themselves will have to be persistent!
     *  @param selected_option index of the default option. 0 by default, for the first option, may be > NUM, for
     *                         no option selected by default. */
    EmbAJAXRadioGroup(const char* id_base, const char* options[N], uint8_t selected_option = 0) : EmbAJAXElementList(N), EmbAJAXRadioGroupBase() {
#warning port to element list properly
        for (uint8_t i = 0; i < N; ++i) {
            char* childid = childids[i];
            strncpy(childid, id_base, EMBAJAX_MAX_ID_LEN-4);
            itoa(i, &(childid[strlen(childid)]), 10);
            buttons[i] = EmbAJAXCheckButton(childid, options[i], i == selected_option);
            buttons[i].radiogroup = this;
            buttonpointers[i] = &buttons[i];
        }
        _current_option = selected_option;
        _children = buttonpointers;
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
    EmbAJAXBase* button(uint8_t num) {
        if (num < NUM) return (&buttons[num]);
        return 0;
    }
private:
    EmbAJAXCheckButton buttons[N]; /** NOTE: Internally, the radio groups allocates individual check buttons. This is the storage space for those. */
    EmbAJAXBase* buttonpointers[N]; /** NOTE: ... and, unfortunately, we need a separate persistent array of pointers... */
    char childids[N][EMBAJAX_MAX_ID_LEN]; /** NOTE: Child ids are not copied by EmbAJAXElement. This is the storage space for them */  // TODO: Can we remove those, by using a recursive lookup scheme, instead? (groupid.buttonid)
    int8_t _current_option;
    void selectButton(EmbAJAXCheckButton* which) override {
        _current_option = -1;
        for (uint8_t i = 0; i < NUM; ++i) {
            if (which == _children[i]) {
                _current_option = i;
            } else {
                buttons[i].setChecked(false);
            }
        }
    }
};

/** @brief Abstract base class for EmbAJAXOptionSelect. */
class EmbAJAXOptionSelectBase : public EmbAJAXElement {
public:
    /** Select the option specified by index. */
    void selectOption(uint8_t num);
    /** @return the index of the currently selected option */
    uint8_t selectedOption() const;
    const char* value(uint8_t which = EmbAJAXBase::Value) const override;
    const char* valueProperty(uint8_t which = EmbAJAXBase::Value) const override;
    void updateFromDriverArg(const char* argname) override;
protected:
    EmbAJAXOptionSelectBase(const char*id, uint8_t current_option) : EmbAJAXElement(id) {
        _current_option = current_option;
    };
    void print(const char* const* _labels, uint8_t NUM) const;
    uint8_t _current_option;
};

/** @brief Drop-down list of selectable options
 *
 *  Drop-down list of selectable options. Most functions of interest are implemented in the base class EmbAJAXOptionSelectBase,
 *  you'll only use this class for the constructor. */
template<size_t NUM> class EmbAJAXOptionSelect : public EmbAJAXOptionSelectBase {
public:
    /** ctor.
     *  @param id id for the element
     *  @param labels labels for the options. Note: The @em array of options may be a temporary, but the option-strings themselves will have to be persistent!
     *  @param selected_option index of the default option. 0 by default, for the first option, may be > NUM, for no option selected by default. */
    EmbAJAXOptionSelect(const char* id, const char* labels[NUM], uint8_t selected_option = 0) : EmbAJAXOptionSelectBase(id, selected_option) {
        for (uint8_t i = 0; i < NUM; ++i) {
            _labels[i] = labels[i];
        }
    }
    void print() const override {
        EmbAJAXOptionSelectBase::print(_labels, NUM);
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
class EmbAJAXPage : public EmbAJAXElementList {
public:
    /** Create a web page.
     *  @param children list of elements on the page
     *  @param title title (may be 0). This string is not copied, please do not use a temporary string.
     *  @param header_add literal text (may be 0) to be added to the header, e.g. CSS (linked or in-line). This string is not copied, please do not use a temporary string).
     *  @param min_interval minimum interval (ms) between two requests sent by a single client. A lower value may reduce latency at the cost of traffic/CPU. */
    template<size_t NUM> constexpr EmbAJAXPage(EmbAJAXBase* (&children)[NUM], const char* title, const char* header_add = 0, uint16_t min_interval=100) :
        EmbAJAXElementList(children), _title(title), _header_add(header_add), _min_interval(min_interval) {}
    /** constructor taking an array of elements with a size that cannot be determined at compile time. In this case, you'll have to specify the size, as the first parameter */
    constexpr EmbAJAXPage(size_t childcount, EmbAJAXBase* const* children, const char* title, const char* header_add = 0, uint16_t min_interval=100) :
        EmbAJAXElementList(childcount, children), _title(title), _header_add(header_add), _min_interval(min_interval) {}
    /** constructor taking list of pointers to elements */
    template<class... T> constexpr EmbAJAXPage(const char* title, const char* header_add, uint16_t min_interval, EmbAJAXBase* first, T*... elements) :
        EmbAJAXElementList(first, elements...), _title(title), _header_add(header_add), _min_interval(min_interval) {}

    /** Duplication of print(), historically needed for internal reasons. Use print(), instead! */
    EMBAJAX_DEPRECATED(2023_03_12, "Use print(), instead") void printPage() const {
        print();
    };
    /** Serve the page including headers and all child elements. You should arrange for this function to be called, whenever
     *  there is a GET request to the desired URL. */
    void print() const override;
    /** Handle AJAX client request. You should arrange for this function to be called, whenever there is a POST request
     *  to whichever URL you served the page itself, from.
     *
     *  @param change_callback If some value has changed in the client, this function will be called. While it is optional
     *                         to specify this, if there are any changes that may need to be sent back to the client in
     *                         response to the change, you should specify this function, and handle the change inside it.
     *                         This way, an update can be sent back to the client, immediately, for a smooth UI experience.
     *                         (Otherwise the client will be updated on the next poll). */
    void handleRequest(void (*change_callback)()=0);
    /** Returns true if a client seems to be connected (connected clients should send a ping at least once per second; by default this
     *  function returns whether a ping has been seen within the last 5000 ms.
     *  @param latency_ms Number of milliseconds to consider as maximum silence period for an active connection */
    bool hasActiveClient(uint64_t latency_ms=5000) const {
        return(_latest_ping && (_latest_ping + latency_ms > millis()));
    }
protected:
    const char* _title;
    const char* _header_add;
    uint16_t _min_interval;
    uint64_t _latest_ping = 0;
};

// If the user has not #includ'ed a specific output driver implementation, make a good guess, here
#if not defined (EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION)
#if defined (ESP8266)
#include "EmbAJAXOutputDriverESP8266.h"
#elif defined (ESP32)
#include "EmbAJAXOutputDriverESP32.h"
#elif defined (ARDUINO_ARCH_RP2040)
#include "EmbAJAXOutputDriverRP2040.h"
#else
#include <WebServer.h>
#define EmbAJAXOutputDriverWebServerClass WebServer
#include <WiFi.h>
#include "EmbAJAXOutputDriverGeneric.h"
#warning No output driver available for this hardware (yet). We try using the generic driver, but success is not guaranteed.
#warning In most cases, implementing a driver is as easy as picking the correct header file to include. Please consider submitting a patch.
#endif
#endif

#endif
