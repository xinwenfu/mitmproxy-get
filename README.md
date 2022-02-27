# mitmproxy-get

This project requires the installation of adafruit/DHT sensor library within PlatformIO and apache web server at Ubuntu VM. The firmware sends DHT11 data to a web server, whose IP address is hard-coded into the code. Therefore, an apache web server shall be installed at Ubuntu VM.

Once downloaded to Ubuntu VM, start VS code and use *File*->*Open Folder...* to load the project.

## MITM against HTTPS
1. Enable https on Apache web server of Ubuntu. Please refer to [How To Enable HTTPS Protocol with Apache 2 on Ubuntu 20.04](https://www.rosehosting.com/blog/how-to-enable-https-protocol-with-apache-2-on-ubuntu-20-04/)
2. Make sure the code has the following macro defintion
```
#define FU_HTTPS
```

