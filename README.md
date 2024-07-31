# FloWWeaver
Streaming gateway designed to perform multi-: 
- source
- destination
- protocol
- data-type

streaming. 

## Supported protocols
Nowadays FloWWeaver supports pipelines:
1. RTMP-HLS
2. TCP-HLS
3. SpaceWire-HLS

## Design
The main idea of FloWWeaver is to separate data processing from data streaming and data source. FloWWeaver uses a shared memory ring queue to provide layer separation.

To run any pipeline, you need to link FloWWeaver to your application. Here is the command to execute your own logic in the pipeline:
```
flowweave --exec_mode=http --streamer=./video_streamer --handler=./video_handler --video_repository=./video_repository
```
This will execute 3 separated processes:
1. streamer
2. handler
3. main server

## Easy to integrate

To include custom logic you have to provide all three components. Good news: there are some templates, which you could
use within your application.
Check `src/wc_daemon.h` / `src/wc_daemon.cpp` to verify the usage of FloWWeaver's default `HandleStreamDaemon` interface
and the boilerplate "main"s of streamer and handler: `webcam_streamer.cpp` / `webcam_handler.cpp`

## High performance
Due to interprocess communication, many applications face performance issues, even if they use a powerful and performant
tool like C++. 
FloWWeaver uses a fixed-size shared memory pool to prevent OOM and performance leaks when using the kernel.

## Future of this project:
- Separated Host-Server template is in *development*.
- FlyMyAI (https://flymy.ai) integration
- Nginx-Unit integration
- Pingora integration
- nvidia/Triton-Inference-Server integration
- RabbitMQ integration
- ZeroMQ integration
- Opensource SpaceWire extension

# Contribute and interact
If you consider this project as part of your own product or want to become a contributor  - feel free to DM me on:
- Telegram: @lucshe_tebia
- EMail: lyerhd@gmail.com
I'll help you integrate it with your applications or I'll direct your efforts in the right direction. 
