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
#ifndef EMBAJAX_H
#define EMBAJAX_H

#include <Arduino.h>

#define ARDUJAX_MAX_ID_LEN 16

class EmbAJAXOutputDriverBase;
class EmbAJAXElement;
class EmbAJAXContainerBase;
class EmbAJAXPageBase;

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
     *  does not have such a child. @see EmbAJAXContainer, and @see EmbAJAXHideableContainer. */
    virtual EmbAJAXElement* findChild(const char*id) const {
        return 0;
    }
protected:
template<size_t NUM> friend class EmbAJAXContainer;
    virtual void setBasicProperty(uint8_t num, bool status) {};

    static EmbAJAXOutputDriverBase *_driver;
    static char itoa_buf[8];

    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXContainer::printChildren() */
    void printChildren(EmbAJAXBase** children, uint num) const;
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXContainer::sendUpdates() */
    bool sendUpdates(EmbAJAXBase** children, uint num, uint16_t since, bool first);
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXContainer::findChild() */
    EmbAJAXElement* findChild(EmbAJAXBase** children, uint num, const char*id) const;
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXPage::print() */
    void printPage(EmbAJAXBase** children, uint num, const char* _title, const char* _header) const;
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXPage::handleRequest() */
    void handleRequest(EmbAJAXBase** children, uint num, void (*change_callback)());
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
    virtual void installPage(EmbAJAXPageBase *page, const char *path, void (*change_callback)()=0) = 0;
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
     *  @param quoted If true, add double-quotes around the string, and esacpe any double
     *                quotes within the string as &quot;.
     *  @param HTMLescaped If true, escape any "<" and "&" in the input as "\&lt;" and "\&amp;"
     *                     such that it will appear as plain text if rendered as HTML
     *                     (safe for untrusted user input). */
    void printFiltered(const char* value, QuoteMode quoted, bool HTMLescaped);
    /** Shorthand for printFiltered(value, JSQuoted, false); */
    inline void printJSQuoted (const char* value) { printFiltered (value, JSQuoted, false); }
    /** Shorthand for printFiltered(value, HTMLQuoted, false); */
    inline void printHTMLQuoted (const char* value) { printFiltered (value, HTMLQuoted, false); }
    /** Convenience function to print an attribute inside an HTML tag.
     *  This function adds a space _in front of_ the printed attribute.
     *
     *  @param name name of the attribute
     *  @param value value of the attribute. Will be quoted. */
    void printAttribute(const char* name, const char* value);
    /** Convenience function to print an integert attribute inside an HTML tag.
     *  This function adds a space _in front of_ the printed attribute.
     *
     *  @param name name of the attribute
     *  @param value value of the attribute. */
    void printAttribute(const char* name, const int32_t value);
private:
    uint16_t _revision;
    uint16_t next_revision;
};

/** Convenience macro to set up an EmbAJAXPage, without counting the number of elements for the template. See EmbAJAXPage::EmbAJAXPage()
 *  @param name Variable name of the page instance
 *  @param title HTML Title
 *  @param header_add a custom string to add to the HTML header section, e.g. a CSS definition. */
#define MAKE_EmbAJAXPage(name, title, header_add, ...) \
    EmbAJAXBase* name##_elements[] = {__VA_ARGS__}; \
    EmbAJAXPage<sizeof(name##_elements)/sizeof(EmbAJAXBase*)> name(name##_elements, title, header_add);

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
 *  This passive element can be inserted into a page to indicate the connection status: If more than 5 client requests go unanswered, in a row,
 *  the connection to the server is assumed to be broken.
 *
 *  @note While this is a "dynamic" display, the entire logic is implemented on the client, for obvious reasons. From the point of view of the
 *        server, this is a static element. */
class EmbAJAXConnectionIndicator : public EmbAJAXBase {
public:
    /** c'tor. If you don't like the default status indications, you can pass the HTML to be shown for "ok" and "fail" states.
     *
     *  @param content_ok Value to show for OK state. May contain HTML markup. Leave as 0 for default.
     *  @param content_ok Value to show for broken state. May contain HTML markup. Leave as 0 for default. */
    EmbAJAXConnectionIndicator(const char* content_ok = 0, const char* content_fail = 0) {
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
template<size_t NUM> friend class EmbAJAXPage;
friend class EmbAJAXBase;
    const char* _id;
    void setChanged();
    bool changed(uint16_t since);
    /** Filthy trick to keep (template) implementation out of the header. See EmbAJAXTextInput::print() */
    void printTextInput(uint size, const char* value) const;
private:
    byte _flags;
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
    virtual void selectOption(EmbAJAXCheckButton* which) = 0;
    const char* _name;
};

/** @brief Base class for groups of objects */
template<size_t NUM> class EmbAJAXContainer : public EmbAJAXBase {
public:
    EmbAJAXContainer(EmbAJAXBase *children[NUM]) : EmbAJAXBase() {
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
        for (int i = 0; i < NUM; ++i) {
            _children[i]->setBasicProperty(num, status);
        }
    }
template<size_t> friend class EmbAJAXHideableContainer;
    EmbAJAXContainer() {};
    EmbAJAXBase** _children;
};

/** @brief A list of objects that can be hidden, completely
 *
 *  This is _essentially_ an EmbAJAXContainer with an id. The one advantage that this
 *  class has other EmbAJAXContainer, is that it can be hidden _completely_, including
 *  any EmbAJAXStatic objects inside it. On the client, the children of this element
 *  are encapsulated in a \<div> element. Other than this, it should behave identical
 *  to EmbAJAXContainer.
 *
 *  You do _not_ need this class to hide an EmbAJAXContainer that contains only EmbAJAXElement
 *  derived objects, or standalone EmbAJAXElement objects.
 *
 *  @note This is _not_ a derived class of EmbAJAXContainer, to avoid adding virtual
 *        inheritance just for this. */
template<size_t NUM> class EmbAJAXHideableContainer : public EmbAJAXElement {
public:
    EmbAJAXHideableContainer(const char* id, EmbAJAXBase *children[NUM]) : EmbAJAXElement(id) {
        _childlist = EmbAJAXContainer<NUM>(children);
    }
    void print() const override {
        _driver->printContent("<div");
        _driver->printAttribute("id", _id);
        _driver->printContent(">");
        _childlist.print();
        _driver->printContent("</div>");
    }
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
    EmbAJAXContainer<NUM> _childlist;
};

/** @brief A set of radio buttons (mutally exclusive buttons), e.g. for on/off, or low/mid/high, etc.
 *
 *  You can insert either the whole group into an EmbAJAXPage at once, or - for more flexbile
 *  layouting - retrieve the individual buttons using() button, and insert them into the page
 *  as independent elements. */
template<size_t NUM> class EmbAJAXRadioGroup : public EmbAJAXContainer<NUM>, public EmbAJAXRadioGroupBase {
public:
    /** ctor.
     *  @param id_base the "base" id. Internally, radio buttons with id_s id_base0, id_base1, etc. will be created.
     *  @param options labels for the options. Note: The @em array of options may be a temporary, but the option-strings themselves will have to be persistent!
     *  @param selected_option index of the default option. 0 by default, for the first option, may be > NUM, for
     *                         no option selected by default. */
    EmbAJAXRadioGroup(const char* id_base, const char* options[NUM], uint8_t selected_option = 0) : EmbAJAXContainer<NUM>(), EmbAJAXRadioGroupBase() {
        for (uint8_t i = 0; i < NUM; ++i) {
            char* childid = childids[i];
            strncpy(childid, id_base, ARDUJAX_MAX_ID_LEN-4);
            itoa(i, &(childid[strlen(childid)]), 10);
            buttons[i] = EmbAJAXCheckButton(childid, options[i], i == selected_option);
            buttons[i].radiogroup = this;
            buttonpointers[i] = &buttons[i];
        }
        _current_option = selected_option;
         EmbAJAXContainer<NUM>::_children = buttonpointers;  // Hm, why do I need to specify EmbAJAXContainer<NUM>::, explicitly?
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
    EmbAJAXCheckButton buttons[NUM]; /** NOTE: Internally, the radio groups allocates individual check buttons. This is the storage space for those. */
    EmbAJAXBase* buttonpointers[NUM];
    char childids[NUM][ARDUJAX_MAX_ID_LEN];
    int8_t _current_option;
    void selectOption(EmbAJAXCheckButton* which) override {
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

/** @brief Absrract internal helper class
 *
 * Needed for internal reasons. Refer to EmbAJAXPage, instead. */
class EmbAJAXPageBase {
public:
    virtual void handleRequest(void (*change_callback)()=0) = 0;
    virtual void printPage() = 0;
};

/** @brief The main interface class
 *
 *  This is the main interface class. Create a web-page with a list of elements on it, and arrange for
 *  print() (for page loads) adn handleRequest() (for AJAX calls) to be called on requests. By default,
 *  both page loads, and AJAX are handled on the same URL, but the first via GET, and the second
 *  via POST. */
template<size_t NUM> class EmbAJAXPage : public EmbAJAXContainer<NUM>, public EmbAJAXPageBase {
public:
    /** Create a web page.
     *  @param children list of elements on the page
     *  @param title title (may be 0). This string is not copied, please do not use a temporary string.
     *  @param header_add literal text (may be 0) to be added to the header, e.g. CSS (linked or in-line). This string is not copied, please do not use a temporary string). */
    EmbAJAXPage(EmbAJAXBase* children[NUM], const char* title, const char* header_add = 0) : EmbAJAXContainer<NUM>(children) {
        _title = title;
        _header_add = header_add;
    }
    /** Duplication of print(), needed for internal reasons. Use print(), instead! */
    void printPage() override {
        print();
    }
    /** Serve the page including headers and all child elements. You should arrange for this function to be called, whenever
     *  there is a GET request to the desired URL. */
    void print() const override {
        EmbAJAXBase::printPage(EmbAJAXContainer<NUM>::_children, NUM, _title, _header_add);
    }
    /** Handle AJAX client request. You should arrange for this function to be called, whenever there is a POST request
     *  to whichever URL you served the page itself, from.
     *
     *  @param change_callback If some value has changed in the client, this function will be called. While it is optional
     *                         to specify this, if there are any changes that may need to be sent back to the client in
     *                         response to the change, you should specify this function, and handle the change inside it.
     *                         This way, an update can be sent back to the client, immediately, for a smooth UI experience.
     *                         (Otherwise the client will be updated on the next poll). */
    void handleRequest(void (*change_callback)()=0) override {
        EmbAJAXBase::handleRequest(EmbAJAXContainer<NUM>::_children, NUM, change_callback);
    }
protected:
    const char* _title;
    const char* _header_add;
};

// If the user has not #includ'ed a specific output driver implementation, make a good guess, here
#if not defined (EMBAJAX_OUTUPUTDRIVER_IMPLEMENTATION)
#if defined (ESP8266)
#include <EmbAJAXOutputDriverESP8266.h>
#elif defined (ESP32)
#include <EmbAJAXOutputDriverESP32.h>
#else
#error No output driver available for this hardware (yet). Please implement your own (it is easy!) and submit a patch.
#endif
#endif

#endif
