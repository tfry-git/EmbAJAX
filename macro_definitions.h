/**
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

/** @file macro_definitions.h
 *
 * Macro definitions. Please note that many of these are internal. Please stick to the documented ones.
 */

/* //////////// String formatting tokens ////////////////  */

#define JS_QUOTED_STRING_ARG "\10"
#define HTML_QUOTED_STRING_ARG "\11"
//#define JS_ESCAPED_STRING_ARG '\12'
#define HTML_ESCAPED_STRING_ARG "\13"
#define PLAIN_STRING_ARG "\14"
#define INTEGER_VALUE_ARG "\15"

/* //////////// String formatting macros //////////////// 
 *
 * The user-facing macro here is printFormatted(). This takes static strings and args in a
 * and re-aggranges them so that static strings are merged into one, and the variable args args
 * are appended at the end (suitable for EmbAJAXOutputDriverBase::printContentF(). */

// See https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
#define GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,NAME,...) NAME
#define printF_(...) GET_MACRO(__VA_ARGS__, printF_26, printF_25, printF_24, printF_23, printF_22, printF_21, printF_20, printF_19, printF_18, printF_17, printF_16, printF_15, printF_14, printF_13, printF_12, printF_11, printF_10, printF_9, printF_8, printF_7, printF_6, printF_5, printF_4, printF_3, printF_2, printF_1)(__VA_ARGS__)

/** For use in #printFormatted(): Insert an argument such that is suitable as a string literal in JavaScript. Surrounding quotes are added, automatically. */
#define JS_QUOTED_STRING(X) JS_QUOTED_STRING_ARG, (const char*) X
/** For use in #printFormatted(): Insert an argument such that is suitable as a string attribute value in HTML. Surrounding quotes are added, automatically. */
#define HTML_QUOTED_STRING(X) HTML_QUOTED_STRING_ARG, (const char*) X
//#define JS_QUOTED_STRING(X) JS_ESCAPED_STRING_ARG, (const char*) X
/** For use in #printFormatted(): Insert a string replacing HTML entities that could break parsing: "<& . */
#define HTML_ESCAPED_STRING(X) HTML_ESCAPED_STRING_ARG, (const char*) X
/** For use in #printFormatted(): Insert an plain string argument, as is. */
#define PLAIN_STRING(X) PLAIN_STRING_ARG, (const char*) X
/** For use in #printFormatted(): Insert an integer value as a string (base 10). */
#define INTEGER_VALUE(X) INTEGER_VALUE_ARG, (int) X

    /** \def printFormatted()
     *  @brief Print a static string with parameters replaced, roughly similar to printf
     *
     *  This is the primary function for elements to "print" to the client, i.e. to send the HTML/JS code needed to make
     *  the element work. The usage may best be explained by example (actual code of EmbAJAXSlider:
     *  @code{.cpp}
     *  _driver->printFormatted("<input type=\"range\" id=", HTML_QUOTED_STRING(_id),
     *                          " min=", INTEGER_VALUE(_min), " max=", INTEGER_VALUE(_max),
     *                          " value=", INTEGER_VALUE(_value),
     *                          " oninput=\"doRequest(this.id, this.value);\" onchange=\"oninput();\"/>");
     *  @endcode
     *
     *  First thing to note is that - although this function is technically implemented as a macro - it behaves like a
     *  public member function of EmbAJAXOutputDriverBase. Actually the macro relays to appropriate helper functions in that class.
     *
     *  Arguments can be either string literals, or values. These two kinds of argument have to be used alternatingly (which is usually
     *  needed, anyway), i.e. "string", value, "string", value... Values have to be wrapped by one of #HTML_QUOTED_STRING(), #INTEGER_VALUE(),
     *  #HTML_ESCAPED_STRING, JS_QUOTED_STRING, PLAIN_STRING, which will control just how the value is inserted (with of without quotes, with
     *  HTML entities escaped, etc.).
     *
     *  Internally, all static portions of the output will be concatenated to a single string, which - on architectures where it matters -
     *  will automatically be wrapped inside an F() macro, for storage in FLASH memory, thus helping a lot to reduce RAM usage (not yet implemented,
     *  to come soon).
     *
     *  For efficiency reasons, you should try to merge as many bits of output in a single printFormatted(), as possible. I.e. instead of
     *  @code{.cpp}
     *  _driver->printFormatted("id=", HTML_QUOTED_STRING(_id));
     *  _driver->printFormatted(" value=", INTEGER_VALUE(_value));
     *  @endcode
     *  always use:
     *  @code{.cpp}
     *  _driver->printFormatted("id=", HTML_QUOTED_STRING(_id), _driver->printFormatted(" value=", INTEGER_VALUE(_value));
     *  @endcode
     * */
#define printFormatted(...) printF_(__VA_ARGS__)

#define printF_3(F1, A1a, A1b) printF_proxy((F1 A1a), A1b)
#define printF_4(F1, A1a, A1b, F2) printF_proxy((F1 A1a F2), A1b)
//#define printF_5(...) // Not validly possible, as we always follow fmt, arg, fmt, arg...
#define printF_6(F1, A1a, A1b, F2, A2a, A2b) printF_proxy((F1 A1a F2 A2a), A1b, A2b)
#define printF_7(F1, A1a, A1b, F2, A2a, A2b, F3) printF_proxy((F1 A1a F2 A2a F3), A1b, A2b)
//#define printF_8(...)
#define printF_9(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b) printF_proxy((F1 A1a F2 A2a F3 A3a), A1b, A2b, A3b)
#define printF_10(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4) printF_proxy((F1 A1a F2 A2a F3 A3a F4), A1b, A2b, A3b)
//#define printF_11(...)
#define printF_12(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a), A1b, A2b, A3b, A4b)
#define printF_13(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5), A1b, A2b, A3b, A4b)
//#define printF_14(...)
#define printF_15(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5, A5a, A5b) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5 A5a), A1b, A2b, A3b, A4b, A5b)
#define printF_16(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5, A5a, A5b, F6) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5 A5a F6), A1b, A2b, A3b, A4b, A5b)
//#define printF_17(...)
#define printF_18(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5, A5a, A5b, F6, A6a, A6b) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5 A5a F6 A6a), A1b, A2b, A3b, A4b, A5b, A6b)
#define printF_19(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5, A5a, A5b, F6, A6a, A6b, F7) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5 A5a F6 A6a F7), A1b, A2b, A3b, A4b, A5b, A6b)
//#define printF_20(...)
#define printF_21(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5, A5a, A5b, F6, A6a, A6b, F7, A7a, A7b) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5 A5a F6 A6a F7 A7a), A1b, A2b, A3b, A4b, A5b, A6b, A7b)
#define printF_22(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5, A5a, A5b, F6, A6a, A6b, F7, A7a, A7b, F8) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5 A5a F6 A6a F7 A7a F8), A1b, A2b, A3b, A4b, A5b, A6b, A7b)
//#define printF_23(...)
#define printF_24(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5, A5a, A5b, F6, A6a, A6b, F7, A7a, A7b, F8, A8a, A8b) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5 A5a F6 A6a F7 A7a F8 A8a), A1b, A2b, A3b, A4b, A5b, A6b, A7b, A8b)
#define printF_25(F1, A1a, A1b, F2, A2a, A2b, F3, A3a, A3b, F4, A4a, A4b, F5, A5a, A5b, F6, A6a, A6b, F7, A7a, A7b, F8, A8a, A8b, F9) printF_proxy((F1 A1a F2 A2a F3 A3a F4 A4a F5 A5a F6 A6a F7 A7a F8 A8a F9), A1b, A2b, A3b, A4b, A5b, A6b, A7b, A8b)

#define printF_proxy(X, ...) _printContentF(X, __VA_ARGS__);

