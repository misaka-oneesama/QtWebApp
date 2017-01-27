# QtWebApp

Fork of [**QtWebApp**](http://stefanfrings.de/qtwebapp/index-en.html) by [Stefan Frings](http://stefanfrings.de) with C++14 modernization and improvements.

<br>

## Feature Overview

 - Multithreaded: can handle multiple requests and responses at once. your app should be thread-safe to avoid failure.
 - Simple to include into your project. (`qmake`)
 - SSL support
 - HTML templatizer
 - Supports Cookies (req/res)

## How to use

Only `qmake` is supported right now. There are no plans to add CMake support yet, but that may change in the future :)

There are 4 components

 - `httpserver` ─ the actual server and tcp socket listener
 - `templateengine` ─ HTML templatizer (shares some similarities with Handlebars?)
 - `logging` ─ file logger (may be removed from the fork as there is no use case for it, there are better loggers out there)
 - `qtservice` ─ UNIX daemon and Windows Service wrapper to send app in the background

To include a component to your project add this to your `.pro` file.
```
include(QtWebApp/{component}/{component}.pri)
```
Include paths are setup accordingly and you can start developing instantly. Simple huh?

#### Notice regarding [御坂ーお姉さま](https://github.com/misaka-oneesama/misaka-oneesama)

Report bugs against this repository only if you are sure it's QtWepApp's fault.
