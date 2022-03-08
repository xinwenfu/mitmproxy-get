# MITM attacks against http and https through mitmproxy

In this project, we will learn using mitmproxy to monitor, and modify HTTP/HTTPS traffic manually, and how to use mitmdump with python script to modify HTTP/HTTPS query automatically. Those tools can be used for real attacks and for the purpose of understanding the communication protocol.

Mitmproxy is a set of SSL/TLS-capable proxy tools that can intercept HTTP/HTTPS, Websocket, and generic TCP flows. Mitmproxy tools contain three front-end tools, which work with the same core proxy. 
- *mitmproxy*: an interactive, SSL/TLS-capable intercepting proxy with a console interface.
- *mitmweb*: a web-based interface for mitmproxy.
- *mitmdump*: the command-line version of mitmproxy.

The fi
<img src="imgs/ESP32.png">

# 1. Hardware setup
This project requires the ES32 board, the installation of adafruit/DHT sensor library within PlatformIO and apache web server at Ubuntu VM. The firmware of the ESP32 sends DHT11/DHT22 data to a web server at a Ubuntu VM. The IP address of the web server is hard-coded into the formware code. 

<img src="imgs/mitm-labsetup.jpg">

# 2. Software setup

## Install Apache2

Type the following commands in a terminal at Ubuntu VM.
```
sudo apt update
sudo apt install apache2
```
You can test the installed server by typing in the IP address of the host VM in a web browser.
![image](https://user-images.githubusercontent.com/69218457/156863561-96d0e26f-c1bf-4c27-aa16-26b0ba8c8a1a.png)


## Enable https on Apache web server 

The hard part is to enable https with Apache at Ubuntu. Please refer to [How To Enable HTTPS Protocol with Apache 2 on Ubuntu 20.04](https://www.rosehosting.com/blog/how-to-enable-https-protocol-with-apache-2-on-ubuntu-20-04/). 
The following video shows an example.

[![Demo Video](https://img.youtube.com/vi/4PwXGR39zpg/0.jpg)](https://youtu.be/4PwXGR39zpg)

- The following command creates the https web server's private key (/etc/ssl/private/my-server.key) and self-signed SSL certificate (/etc/ssl/certs/my-server.crt). We do NOT protect the private key of the web server with a password since the web server will not be able to start without user interaction inputting the password. While running this command, the *common name* of the web server must be the IP address of the Ubuntu VM what hosts the web server.
```
sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout /etc/ssl/private/my-server.key -out /etc/ssl/certs/my-server.crt
```

- Edit the configuration file. Here is an example https configuration file [my-server.conf](web/my-server.conf). Copy the example my-server.conf to /etc/apache2/sites-available/ using the sudo command if needed. 

- Enable the https server and restart apache2 to start both http and https servers. It appears we shall not use the full path of my-server.conf with a2ensite. The following command is just fine.
```
sudo a2ensite my-server.conf
sudo systemctl restart apache2
```

## Install PHP
Install PHP and Apache PHP module by running the following commands
```
sudo apt install php libapache2-mod-php
```
Restart Apache2 server
```
sudo systemctl restart apache2.service
```

## Copy PHP script to web folder
Copy [test_get.php](web/test_get.php) to /var/www/html at Ubuntu. The php script returns the temerature and humidity data sent from the ESP32 back to the ESP32 for the purpose of acknowledgement. 
You can test the server by visiting the following link in a browser: *https://Ubuntu-VM-IP/test_get.php?Temperature=21&Humidity=20*


## Install mitmproxy
Download mitmproxy
```
sudo apt install mitmproxy
```

## Clone this project
Download this project to Ubuntu VM, start VS code and use *File*->*Open Folder...* to load the project.
```
cd ~/Documents
git clone https://github.com/xinwenfu/mitmproxy-get.git
```

The code supports both http and https conenctions through a macro definition in the code. 
Make sure the code has the correct macro defintion 

Enable *FU_HTTP* definition as follows to connect to the http server.
```
#define FU_HTTP
```

Enable *FU_HTTPS* definition as follows to connect to the https server.
```
#define FU_HTTPS
```

   
# 3. Set up iptables intercepting http traffic
mitmproxy listens on port 8080 by default. To monitor HTTP and HTTPS flows, we need to redirect traffic to ports 80 and 443 using the tool [iptables](https://linux.die.net/man/8/iptables) in Linux to the port that mitmproxy listens on. Note: These chages will be recovered after next restart.

Enable IP forwarding
```
sudo sysctl -w net.ipv4.ip_forward=1
```

Create an iptables rule set that redirects the desired traffic to mitmproxy
```
sudo iptables -t nat -A PREROUTING -p tcp --dport 443 -j REDIRECT --to-port 8080
sudo iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-port 8080
```

To check added rules using the following command
```
sudo iptables -t nat -L
```

# 4. MITM against HTTP

## Start up mitmproxy
To start up mitmproxy with the console interface, open a command terminal in the directory of mitmproxy and type in
```
mitmproxy
```

## Monitor HTTP flows
Let’s test if mitmproxy can monitor HTTP flows.
Run the http version of the firmware on ESP32 and observe the http requests in mitmproxy.

![image](https://user-images.githubusercontent.com/69218457/156868428-a9f0f869-2e35-40e3-afdb-652006ec86c6.png)    
    
## Intercept and modify HTTP traffic (script) 

Stop mitmproxy by by pressing *Ctrl+c*, then press *y*.

We now use *mitmdump* with python script to modify HTTP traffic sent from ESP32 automatically.
Setup ESP32 and make sure that you can see responses from the server in the VS code console.

Now let’s create the [python script](https://docs.mitmproxy.org/stable/addons-examples/) in the VM. 
Create a .py file ([http-query.py](mitmproxy/http-query.py) in the example). Copy the following code to this file and save it. Remember to replace <host_ip> with your VM’s ip.
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
mitmdump -s /home/iot/Documents/http-query.py
```   
You will see the responses from the server are modified.

![image](https://user-images.githubusercontent.com/69218457/156868594-cce94736-2fcf-4a31-9082-d281a861fd01.png)
    
# 5. MITM against HTTPS

mitmproxy is able to [decrypt encrypted traffic on the fly](https://docs.mitmproxy.org/stable/concepts-howmitmproxyworks/). There are two methods to enable such function and we use the second method as a demo.
- Install mitmproxy’s CA on the client device.
- Use the self-signed server certificate as mitmproxy’s CA.  

We first generate the required PEM format file by running command:
```
cd ~/Documents
sudo cat /etc/ssl/private/my-server.key /etc/ssl/certs/my-server.crt > mitmCA.pem
```
    
Now we can run mitmproxy or mitmdump with two options: 
- --certs *=/home/iot/mitmproxy/mitmCA.pem: configure the self-signed cert
- --ssl-insecure: do not verify server certs

Run mitmproxy to observe decrypted https requests
```
mitmproxy --certs *=/home/iot/Documents/mitmCA.pem --ssl-insecure
```
![image](https://user-images.githubusercontent.com/69218457/156868802-d30c23a1-228e-4bcb-acf5-cf2bb56e9474.png)


Run mitmdump to change the https requests
```
mitmdump --certs *=/home/iot/Documents/mitmCA.pem --ssl-insecure -s ./http-query.py
```
![image](https://user-images.githubusercontent.com/69218457/156868252-acadefc5-4201-490c-b736-82787fb9cb4c.png)


# Reset iptables

After the tasks are done, iptables shall be reset. Otherwise, normal web browsing may be messed up.

Reset iptables by restarting the VM, or by running commands. 
```
sudo iptables -t nat -F
sudo sysctl -w net.ipv4.ip_forward=0
```

# Notes

## Disable apache2 virtual host entry

The following [commands](https://itorn.net/disable-remove-virtual-host-website-entry-in-apache/) disable an apache2 virtual host (server) and restart apache2.
```
sudo a2dissite my-server
sudo service apache2 reload
```

## Intercept and modify HTTP traffic (manually)

To intercept requests with specific URLs, you need to enable the interception function and using specific filter expressions.

To enable the interception function, press **i**. You will see shown at the bottom of the console.
```
set intercept ''
```

<img alt="image" src="https://user-images.githubusercontent.com/69218457/156813305-2802e23d-1f0c-44ea-9ad0-75ab46a29d55.png">
    
    
We use filter expressions *~u \<regex>* to only intercept specific URLs and *~q* to only intercept requests. *&* is used to connect two expressions. More filter expressions can be found [here](https://docs.mitmproxy.org/stable/concepts-filters/).

Type in *~u /test_get.php & ~q*, then press **ENTER**.

<img alt="image" src="https://user-images.githubusercontent.com/69218457/156813514-106b113d-94c9-45da-9713-1de7bba0d2b2.png">

Now let’s visit the same url again. This time, you will find your browser keeps loading. This is because the HTTP request is intercepted by mitmproxy. The intercepted requested is indicated with red text in the mitmproxy console.
<img alt="image" src="https://user-images.githubusercontent.com/69218457/156813624-1a20d41f-b619-409c-814a-7238449cb019.png">

To modify the intercepted connection, put the focus (>>) on that flow using arrow keys and then press **ENTER** to open the details view.
Press *e* to edit the flow. Select a query by using arrow keys and press **ENTER**.

<img alt="image" src="https://user-images.githubusercontent.com/69218457/156813678-2b7f172b-4078-414c-9ed1-adc77d2aa1b3.png">
<img alt="image" src="https://user-images.githubusercontent.com/69218457/156813768-9589f8ae-c5a3-4fce-ab3b-536c2d95fbfa.png">    
    
Mitmproxy shows all query key and value pairs for editing.
Select the value that you want to modify, and press ENTER to edit it.
After finishing editing a value, press ESC to confirm it.

<img alt="image" src="https://user-images.githubusercontent.com/69218457/156813838-a72a6a8f-6ef1-4a8b-a803-b967b4b694aa.png">
    
Press *q* to quit editing mode.
Press *a* to resume the intercepted flow. You will find the changed values shown in your browser.

To stop mitmproxy, Press *Ctrl+c*, then press *y*.

