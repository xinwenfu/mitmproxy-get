# mitmproxy-get

This project requires the installation of adafruit/DHT sensor library within PlatformIO and apache web server at Ubuntu VM. The firmware sends DHT11 data to a web server, whose IP address is hard-coded into the code. Therefore, an apache web server shall be installed at Ubuntu VM.

Once downloaded to Ubuntu VM, start VS code and use *File*->*Open Folder...* to load the project.
## MITM against HTTP

## MITM against HTTPS
1. Enable https on Apache web server of Ubuntu. Please refer to [How To Enable HTTPS Protocol with Apache 2 on Ubuntu 20.04](https://www.rosehosting.com/blog/how-to-enable-https-protocol-with-apache-2-on-ubuntu-20-04/). The following video shows an example.
   - Here is an example https configuration file [my-server.conf](web/my-server.conf)

[![Demo Video](https://img.youtube.com/vi/YTuX5_tq2s8/0.jpg)](https://youtu.be/YTuX5_tq2s8)

2. Copy [test_get.php](web/test_get.php) to /var/www/html at Ubuntu.

3. Make sure the code has the following macro defintion. By defining the directive *FU_HTTPS*, 
```
#define FU_HTTPS
```

