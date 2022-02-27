# mitmproxy-get

This project requires the installation of adafruit/DHT sensor library within PlatformIO and apache web server at Ubuntu VM. The firmware sends DHT11/DHT22 data to a web server, whose IP address is hard-coded into the code. Therefore, an apache web server shall be installed at Ubuntu VM.

Once downloaded to Ubuntu VM, start VS code and use *File*->*Open Folder...* to load the project.

The code supports both http and https conenctions through a macro definition in the code. By default, https is used. 

## MITM against HTTP

## MITM against HTTPS
The hard part is to enable https with Apache at Ubuntu. Otherwise, the setup looks straightforward.

1. Enable https on Apache web server of Ubuntu. Please refer to [How To Enable HTTPS Protocol with Apache 2 on Ubuntu 20.04](https://www.rosehosting.com/blog/how-to-enable-https-protocol-with-apache-2-on-ubuntu-20-04/). The following video shows an example.
   - Here is an example https configuration file [my-server.conf](web/my-server.conf)

[![Demo Video](https://img.youtube.com/vi/4PwXGR39zpg/0.jpg)](https://youtu.be/4PwXGR39zpg)

2. Copy [test_get.php](web/test_get.php) to /var/www/html at Ubuntu.

3. Make sure the code has the following macro defintion of *FU_HTTPS* to connects to the https server.
```
#define FU_HTTPS
```

