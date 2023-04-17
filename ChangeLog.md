-- Changes in version 0.2.0 -- UNRELEASED
* EmbAJAXSlider and EmbAJAXColorPicker now send "live" updates (while dragging).
* Page-global minimum delay between any two updates sent to the server.
  Requests from the same element, that get generated within this minimum delay, are merged.
* Parameters active_timeout and idle_timeout in EmbAJAXJoystick have been removed.
  The former is obsolete by the global limit, the latter had never been implemented.
* EmbAJAXTextInputs are more responsive, and no longer prone to "swallowing" key presses.
X TODO: Testing, sp. joystick

-- Changes before version 0.1.1 have not been recorded --
