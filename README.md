# MITM attacks against http and https through mitmproxy

mitmproxy is a set of SSL/TLS-capable proxy tools that can intercept HTTP, Websocket, and generic TCP flows. mitmproxy tools contain three front-end tools which work with the same core proxy. 
- mitmproxy: an interactive, SSL/TLS-capable intercepting proxy with a console interface.
- mitmweb: a web-based interface for mitmproxy.
- mitmdump: the command-line version of mitmproxy.

We will learn using mitmproxy to modify HTTP query manually, and how to using mitmdump with python script  to modify HTTP query automatically.

# Software setup
## Apache2 server
Install Apache2 by running:
```
sudo apt update
sudo apt install apache2c
```
You can test the installed server by typing in the IP address of the host VM in a web browser.

## PHP
Install PHP and Apache PHP module by running the following commands
```
sudo apt install php libapache2-mod-php
```
Restart Apache2 server
```
sudo systemctl restart apache2.service
```
Put your PHP script (e.g., test_get.php) under path /var/www/html. You can test the server by visiting the following link in a browser: http://<local IP>/test_get.php

## mitmproxy

   
This project requires the installation of adafruit/DHT sensor library within PlatformIO and apache web server at Ubuntu VM. The firmware sends DHT11/DHT22 data to a web server, whose IP address is hard-coded into the code. Therefore, an apache web server shall be installed at Ubuntu VM.

Once downloaded to Ubuntu VM, start VS code and use *File*->*Open Folder...* to load the project.

The code supports both http and https conenctions through a macro definition in the code. By default, https is used. 

## MITM against HTTP

## MITM against HTTPS
The hard part is to enable https with Apache at Ubuntu. Otherwise, the setup looks straightforward.

1. Enable https on Apache web server of Ubuntu. Please refer to [How To Enable HTTPS Protocol with Apache 2 on Ubuntu 20.04](https://www.rosehosting.com/blog/how-to-enable-https-protocol-with-apache-2-on-ubuntu-20-04/). The following video shows an example.
   - Here is an example https configuration file [my-server.conf](web/my-server.conf)
   - Do NOT protect the private key of the web server with a password since the web server will not be able to start with user interaction

[![Demo Video](https://img.youtube.com/vi/4PwXGR39zpg/0.jpg)](https://youtu.be/4PwXGR39zpg)

2. Copy [test_get.php](web/test_get.php) to /var/www/html at Ubuntu.
   - The php script returns the temerature and humidity data sent from the ESP32 back to the ESP32 for the purpose of acknowledgement. 

3. Make sure the code has the following macro defintion of *FU_HTTPS* to connects to the https server.
```
#define FU_HTTPS
```

