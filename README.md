# MITM attacks against http and https through mitmproxy

mitmproxy is a set of SSL/TLS-capable proxy tools that can intercept HTTP, Websocket, and generic TCP flows. mitmproxy tools contain three front-end tools which work with the same core proxy. 
- mitmproxy: an interactive, SSL/TLS-capable intercepting proxy with a console interface.
- mitmweb: a web-based interface for mitmproxy.
- mitmdump: the command-line version of mitmproxy.

We will learn using mitmproxy to modify HTTP query manually, and how to using mitmdump with python script  to modify HTTP query automatically.

This project requires the installation of adafruit/DHT sensor library within PlatformIO and apache web server at Ubuntu VM. The firmware sends DHT11/DHT22 data to a web server, whose IP address is hard-coded into the code. Therefore, an apache web server shall be installed at Ubuntu VM.

Once downloaded to Ubuntu VM, start VS code and use *File*->*Open Folder...* to load the project.

The code supports both http and https conenctions through a macro definition in the code. By default, https is used. 

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
Download mitmproxy
```
sudo apt install mitmproxy
```

# Redirecting traffic for interception
mitmproxy listens on port 8080 by default. To monitor HTTP and HTTPS flows, their ports 80 and 443 should be redirected to port the port mitmproxy listens on. (these chages will be recovered after next restart)

Enable IP forwarding
```
sudo sysctl -w net.ipv4.ip_forward=1
```

Create an iptables ruleset that redirects the desired traffic to mitmproxy
```
sudo iptables -t nat -A PREROUTING -p tcp --dport 443 -j REDIRECT --to-port 8080
sudo iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-port 8080
```

# Start up mitmproxy
To start up mitmproxy with console interface, open a command terminal in the directory of mitmproxy and type in
```
mitmproxy
```
mitmproxy can be stopped by press Ctrl+c, then press y.

# MITM against HTTP

## Monitor HTTP flows
Let’s test if mitmproxy can monitor HTTP flows.
Open a web browser from a separate machine (e.g., your host computer). Don’t use the VM which runs mitmproxy because the [PREROUTING](https://serverfault.com/a/977515) table only redirects traffic from outside. 
Visit http://<host ip>/test_get.php?Temperature=21&Humidity=20 and you will see the traffic in the mitmproxy console.

## Intercept and modify HTTP traffic (manually)

To intercept request with specific URLs, you need to enable the interception function and using specific filter expressions.

To enable the interception function, press i. You will see shown at the bottom of the console.
```
set intercept ``
```

We use filter expressions “~u <regex>” to only intercept specific URLs and “~q” to only intercept requests. “&” is used to connect two expressions. More filter expressions can be found [here](https://docs.mitmproxy.org/stable/concepts-filters/).

Type in *~u /test_get.php & ~q*, then press **ENTER**.

Now let’s visit the same url again. This time, you will find your browser keeps loading. This is because the HTTP request is intercepted by mitmproxy. The intercepted requested is indicated with red text in the mitmproxy console.

To modify the intercepted connection, put the focus (>>) on that flow using arrow keys and then press **ENTER** to open the details view.
Press *e* to edit the flow. Select query by using arrow keys and press **ENTER**.

Mitmproxy shows all query key and value pairs for editing.
Select the value that you want to modify, and press ENTER to edit it.
After finishing editing a value, press ESC to confirm it.

Press q to quit editing mode.
Press a to resume the intercepted flow. You will find the changed values shown in your browser.

To stop mitmproxy, Press Ctrl+c, then press y.
![image](https://user-images.githubusercontent.com/69218457/156810975-6cf47f20-35af-452d-ba51-59a5024d09e4.png)

## Intercept and modify HTTP traffic (script) 

We use mitmdump with python script to modify HTTP traffic sent from ESP32 automatically.
Setup ESP32 and make sure that you can see responses from the server in the VS code console.

Remember to recover redirected ports before you test ESP32 without mitmproxy involved, otherwise ESP32 will failed to connect the server. The recovery can be done by restarting the VM, or by running commands: 
```
sudo iptables -t nat -F
sudo sysctl -w net.ipv4.ip_forward=0
```

Now let’s create the python script in the VM. Some example scripts can be found [here](https://docs.mitmproxy.org/stable/addons-examples/).
Create a .py file (http-query.py in the example). Copy the following code to this file and save it. Remember to replace <host_ip> with your VM’s ip.
```
"""Modify HTTP query parameters."""
from mitmproxy import http

Servername = "http://<host_ip>/test_get.php"

def request(flow: http.HTTPFlow) -> None:
    if Servername in flow.request.pretty_url:
        flow.request.query["Temperature"] = "10000"
        flow.request.query["Humidity"] = "10000"
```

Run the script using command:
```
mitmdump –s <script_path>
```   
You will see the responses from the server are modified.

   
# MITM against HTTPS
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

mitmproxy is able to decrypt encrypted traffics on the fly (more information can be found here). There are two methods to enable such function :
- Install mitmproxy’s CA on the client device.
- Use the self-signed server certificate as mitmproxy’s CA.  
Since our client ESP32 does not have functions for installing trusted CA, we use the second method.
   
We first generate the required PEM format file by running command:
```
sudo cat [self-signed key] [self-signed crt] > mitmCA
```

Now we can run mitmproxy or mitmdump with two options: 
- *--certs *=/home/iot/mitmproxy/mitmCA.pem*: configure the self-signed cert
- *--ssl-insecure*: do not verify server certs
   
