# CPU usage tracker
## Disclaimer
It's just a test assignment. For real life case it will be better to use
``top`` tool etc.
## Description
This is the simple console application in C to track CPU percentage usage.
The program consists of three threads. The first thread _(Reader)_ reads
``/proc/stat`` and parses it. The second thread _(Analyzer)_ produces CPU usage
in percentages for each CPU core. Then Analyzer sends CPU usage to the third
thread _(Printer)_. Printer prints to the console average CPU usage each second.
Also a SIGTERM handler is implemented to close the application gently (close
descriptors, finish threads, free memory). The application has to work on every
Linux distribution.
## Building
### GCC ###
```
$ make
```
### Clang ###
```
$ make CC=clang
```
## Author
#### __Mark Sungurov__ <mark.sungurov@gmail.com>
## License
This project is licensed under the __GPLv3__.
