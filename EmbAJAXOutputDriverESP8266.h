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

#ifndef EMBAJAXOUTPUTDRIVERESP8266_H
#define EMBAJAXOUTPUTDRIVERESP8266_H

#include <ESP8266WebServer.h>
#define EmbAJAXOutputDriverWebServerClass ESP8266WebServer
#include <ESP8266WiFi.h>  // Makes the examples work cross-platform; not strictly needed

// the actual implementation
#include "EmbAJAXOutputDriverGeneric.h"

// for compatibility with early code examples
#define EmbAJAXOutputDriverESP8266 EmbAJAXOutputDriver

#endif
