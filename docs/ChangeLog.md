-- Changes in version 0.3.0 -- UNRELEASED
X TODO: Fix EmbAJAXValidatingTextInput (did it ever work?)

-- Changes in version 0.2.0 -- 2023-04-29
* On Harvard-architecture MCUs, keep most static strings in flash memory, only. This can achieve
  massive RAM savings on some MCUs (none one others), at the cost of small performance hit
  (behavior can be customized using the USE_PROGMEM_STRINGS #define).
* Internal: Add printContentF function to allow more flash-efficient generation of HTML/JS code
* EmbAJAXSlider and EmbAJAXColorPicker now send "live" updates (while dragging).
* Page-global minimum delay between any two updates sent to the server.
  Requests from the same element, that get generated within this minimum delay, are merged.
* Parameters active_timeout and idle_timeout in EmbAJAXJoystick have been removed.
  The former is obsolete by the global limit, the latter had never been implemented.
* EmbAJAXTextInputs are more responsive, and no longer prone to "swallowing" key presses.
* Add demo on keeping track of active clients

-- Changes before version 0.1.1 have not been recorded --
