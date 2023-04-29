# Technical background / tweaking performance and resource usage

## Latency vs. network traffic vs. performance

In general you will want user input to arrive at the server, quickly, and changed values on the server to be displayed at the client, quickly.
At the same time there are practical limits to how much the MCU can process, and how much traffic you can - or want to - pass through the network.

For this reason, the following logic is used:

### Client to server

- Events (e.g. a button press) are sent to the server immediately, however.
- While waiting for the answer to an event message, no second event will be transmitted (they will be queued, instead). This simple measure prevents
  the MCU from getting overloaded. (Note: It does not protect against deliberate DOS attacks. That is currently out-of-scope.)
- Even if an answer has already arrived, a rate limit is imposed on event messages. This helps keep traffic limited. The default value is
  one message per 100 ms. You can tweak this setting using the ```min_interval``` parameter to ```EmbAJAXPAge()```, but note that a certainly
  latency is unavoidable at the network level, too.
- While events are waiting to be sent for one of the two reasons, above, they will be "merged", whereever that makes sense. E.g. while typing in
  a text input, quickly, not each indiviual key stroke will be sent, but only the latest full text. In contrast, for push buttons, every single
  click event will be relayed to the server (such that it could count clicks, for example).

### Server to client

EmbAJAX, deliberately, does not use a persistent connection (see implementation notes, below). On current MCUs the result would generally be that connections
would be limited to a single client at a time (or very few clients, anway). Therefore, if using a permanent connection, all further access would be blocked.
Even separate page loads from the same browser.

Instead however, changes happening on the server need to be "polled" by the client. Polling happens:
- Once per seconds (currently hard-coded)
- Implicitly, whenever the client sends an event itself

The latter can be leveraged by registering an ```updateUI()```-function with ```installPage()```, as shown in the basic usage example: Any change
performed, there, will be relayed back to the client, immediately. In most use cases, therefore, the client will still refresh very quickly (again,
an unavoidable latency will result from transmission over the network, anyway, and of course the time needed for processing, itself).

To avoid sending all states of all controls on each request from each client, the framework keeps track of the latest "revision number"
sent to any client. The client pings back its current revision number on each request, so only real changes have to be forwarded. This is particularly
important where several clients are accessing the same page, and need to be kept in sync.

## RAM vs. Flash

On some MCU-architectures, RAM and FLASH reside in two logically distinct address spaces. This implies that regular const char* strings
need to be copied into RAM address space, even if they are fully static. EmbAJAX needs many string constants, and is therefore quite affected
by this problem.

The Arduino F() macro helps to work around this (further reading, there), but does incur a small performance (and code size) penalty, which can
be avoided if a) RAM usage is not an issue, or b) the MCU uses a unified address space.

By default, EmbAJAX tries to detect case b), and will disable use of F() strings, then. The USE_PROGMEM_STRINGS define near the top of EmbAJAX.h
allows you to tweak this for special needs.

Note that at the time of this writing, there is no distinct support for keeping ```EmbAJAXStatic``` blocks in PROGMEM. Pull requests are welcome.

## Some further implementation notes

Concurrent access by an arbitrary number of separate clients is the main reason behind going with AJAX, instead of WebSockets, even if the
latter are often described as more "modern". Note that the purpoted drawback to AJAX - latency - can easily be circumventented for most use
cases, as desribed, above. Still, it would be relatively easy to generalize the framework to also allow a WebSocket-connection. I'm not
doing this, ATM, for fear of adding unneccessary complexity for little or no practical gain.

You may have noted that the framework avoids the use of the String class, even though that would make some things easier. The reason
for this design choice is that the overhead of using char*, here, in a sketch that may be using String, already, is low. However, if this
framework were to rely on String, while nothing else in the sketch uses String, that would incur a significant overhead. Further, it should
be noted, that the risk of memory-fragmentation is relatively real in the present use-case, as arbitrary strings are regularly coming in
"from the outside". Nonetheless, using Strings would relieve the use from having to worry about the lifetime of strings passed in to the
framework, and thus, in the future, it may make sense to support String *optionally*.

Another thing you will notice is that the framework avoids any sort of dynamic list. Instead, template classes with a size parameter are
used to keep lists of elements (such as the EmbAJAXPage\<SIZE>). The reason is again, memory efficiency, and fear of fragmentation. Also,
the vast majority of use cases should be perfectly fine with a statically defined setup of elements. However, should the need arise, it
would be very easily possible to create a dynamically allocated analogon to EmbAJAXContainer<SIZE>. An instance of that could simply be
inserted into a page, and serve as a straight-forward wrapper around elements that are created dynamically. (A different question is how to
keep this in sync with the client, of course, if that is also a requirement...)

Connection error handling, despite asynchronous requests: Both server and client keep track of the "revision" number of their state. This is
used for keeping several clients in sync, but also for error handling: If the server detects that it has a lower revision than the client, it
will know that it has rebooted (while the client has not), and will re-send all current states. If the client tries to send a UI change, but
the network request fails, it will discard its revision, and thereby ask the server to also re-send all states. Thus, the latest user input
may get lost on a network error, but the state of the controls shown in the client will remain in sync with the state as known to the server.
