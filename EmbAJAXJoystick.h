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

#ifndef EMBAJAXJOYSTICK_H
#define EMBAJAXJOYSTICK_H

#include <EmbAJAX.h>

const char EmbAJAXJoystick_SNAP_BACK[] = "if (!pressed) { x = width / 2; y = height / 2; }\n";
const char EmbAJAXJoystick_NO_SNAP_BACK[] = "";
const char EmbAJAXJoystick_FREE_POSITION[] = "";
const char EmbAJAXJoystick_POSITION_9_DIRECTIONS[] = "if (pressed) {\n"
                                                      "  if (x < width/3) x = width / 6;\n"
                                                      "  else if (x > (2*width/3)) x = 5*width / 6\n"
                                                      "  else x = width / 2;\n\n"
                                                      "  if (y < height/3) y = height / 6;\n
                                                      "  else if (y > (2*height/3)) y = 5*height / 6\n"
                                                      "  else y = height / 2;\n"
                                                      "}\n";

/** This class provides a basic joystick for directional control. WORK IN PROGRESS, THIS IS NOT EXPECTED TO WORK, YET. */
class EmbAJAXJoystick : public EmbAJAXJoystick {
public:
    /** C'tor.
     * @param width: Element width
     * @param height: Element height
     * @param rate_limit: Minimum timeout between two position change notifications (in milliseconds)
     * @param timeout: Timeout after which the position will be regarded as uncertain (e.g. due to unreliable network connection)
     * @param position_adjust: Custom javascript that will be applied to "correct" the user supplied position, e.g. snapping it to certain allowed positions. @See e.g. EmbAJAXJoystick_POSITION_9_DIRECTIONS
     * @param snap_back: Custom javascript that will be applied to snap back the position on mouse release. @See EmbAJAXJoystick_SNAP_BACK */
    EmbAJAXJoystick(const char* id, int width, int height, int rate_limit, int timeout, const char* position_adjust=EmbAJAXJoystick_FREE_POSITION, const char* snap_back=EmbAJAXJoystick_SNAP_BACK) : EmbAJAXJoystick(id) {
        _width = width;
        _height = height;
        _position_adjust = position_adjust;
        _snap_back = snap_back;
    }
    void print() const override {
        EmbAJAXBase::_driver->printContent("<canvas");
        EmbAJAXBase::_driver->printAttribute("id", EmbAJAXTextInput<SIZE>::_id);
        EmbAJAXBase::_driver->printAttribute("width", _width);
        EmbAJAXBase::_driver->printAttribute("height", _height);
        EmbAJAXBase::_driver->printContent("style=\"cursor: all-scroll\"/>");
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
private:
    int _width;
    int _height;
    int _active_timeout;
    int _idle_timeout;
    const char* _snap_back;
    const char* _position_adjust;
};

/* <canvas id="joystick" height="300" width="300" style="border-radius:50%; background-color:grey; cursor: all-scroll"></canvas>
<script>
var elem = document.getElementById("joystick");
var width = elem.width;
var height = elem.height;

elem.applypos = function(x, y, pressed) {
    if (!pressed) {
    	x = width / 2;
      y = height / 2;
    }
  	this.posx = x;
    this.posy = y;
}

elem.update = function(x, y) {
  var oldx = this.posx;
  var oldy = this.posy;
  this.applypos(x, y, this.pressed);
  if (this.posx != oldx || this.posy != oldy) {
	  var ctx = this.getContext("2d");
    ctx.clearRect(0, 0, this.width, this.height);
    this.drawKnob(ctx, this.posx, this.posy);
  }
}

elem.drawKnob = function(ctx, x, y) {
	ctx.beginPath();
	ctx.arc(x, y, 15, 0, 2 * Math.PI);
	ctx.stroke();
	ctx.fill();
}

elem.press = function(x, y) {
  this.pressed = 1;
  this.update(x, y);
}

elem.move = function(x, y) {
  this.update(x, y);
}

elem.release = function(x, y) {
  this.pressed = 0;
  this.update();
}

elem.addEventListener("mousedown", function(event) { elem.press(event.offsetX, event.offsetY); }, false);
elem.addEventListener("mousemove", function(event) { elem.move(event.offsetX, event.offsetY); }, false);
elem.addEventListener("mouseup", function(event) { elem.release(event.offsetX, event.offsetY); }, false);
elem.addEventListener("mouseleave", function(event) { elem.release(event.offsetX, event.offsetY); }, false);
elem.addEventListener("touchstart", function(event) { elem.press(event.touches[0].offsetX, event.touches[0].offsetY); }, false);
elem.addEventListener("touchmove", function(event) { elem.move(event.touches[0].offsetX, event.touches[0].offsetY); }, false);
elem.addEventListener("touchend", function(event) { elem.release(event.touches[0].offsetX, event.touches[0].offsetY); }, false);
</script>

void EmbAJAXMomentaryButton::print() const {
    _driver->printContent("<button type=\"button\"");
    _driver->printAttribute("id", _id);
    _driver->printContent(" onMouseDown=\"this.pinger=setInterval(function() {doRequest(this.id, 'p');}.bind(this),");
    _driver->printContent(itoa(_timeout / 1.5, itoa_buf, 10));
    _driver->printContent("); doRequest(this.id, 'p');\" onMouseUp=\"clearInterval(this.pinger); doRequest(this.id, 'r');\" onMouseLeave=\"clearInterval(this.pinger); doRequest(this.id, 'r');\">");
    _driver->printFiltered(_label, EmbAJAXOutputDriverBase::NotQuoted, valueNeedsEscaping());
    _driver->printContent("</button>");
}

*/


#endif
