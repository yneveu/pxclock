PxClock is a textual clock.

It's made of esp8266 and led matrix, 64x32, driven by PxMatrix library.

The time comes from ntp and the matrix can also display notifications, coming from mqtt broker.

You can send basic text on domo/clock mqtt topic messages like 

```
{"type":"info", "text":"Hello World\n!!!!",delay:1000, color:7444}
```
or commands like
```
{"type":"command", "command":"banana"}
```
To let appear a dancing banana animation of course.

With Home Assistant:

```
service: mqtt.publish
data:
  payload: '{"type":"info", "text":"GARAGE\nOUVERT\n!!!!",delay:10000, color:7444}'
  topic: domo/clock/
```

Basic setup
===========

Once compiled and uploaded, a new public wifi should appear. Connect to it and let your operating system open browser on the captive portal.

You should be able to enter your wifi infos.

Mqtt server address is hardcoded for now... You can also specify mqtt user/password in code.

Initially, clock was French only, i just added quick translation to let you get in in English. It's as simple as change include in source code: include switch_hour_en.h or include switch_hour_fr.h. Of course it would be nice to add some languages....


