![IpAddressExpress logo](docs/logo.png?raw=true "IpAddressExpress")

[<img src="https://img.shields.io/github/issues/D9ping/IpAddressExpress.svg?style=flat-square" alt="Open issues" />](https://github.com/D9ping/IpAddressExpress/issues) 
[![Build Status](https://travis-ci.org/D9ping/IpAddressExpress.svg?branch=master)](https://travis-ci.org/D9ping/IpAddressExpress)
### Why use IpAddressExpress?
Compared to shell scripts for figuring out the public IPv4 address IpAddressExpress will  
securely and more reliable detect the current public IPv4 address than using a single public ip service.
It's more secure because it does not require to trust a single server and uses HTTPS for tamper protection.
What makes IpAddressExpress more secure is, that it requires consensus between two randomly selected public IPv4 address web services.
Because of this it's not possible for a single public IPv4 address service to fool the program
 incorrectly thinking the public ip address has been changed.
And by using multiple public IPv4 address services the current public IPv4 can still be discovered after some time
 even when one or more public IPv4 services are down.
IpAddressExpress is written in C so it executes fast and efficiently.
 
### How IpAddressExpress detects public IPv4 address change?
IpAddressExpress randomly selects a HTTPS public IPv4 address service from a list 
of known HTTPS public IPv4 address services for figuring out the public IPv4 address. 
It then compares the result with the previous result to detect if the public IPv4 address has changed. 
If change is detected from previous request, the change is confirmed with another randomly
 selected public IPv4 address HTTPS service so if one public IPv4 HTTPS service is lying, the lie is detected.
If the public IPv4 address is valid and is different from last run the posthook command is executed
 with the new IPv4 address as a commandline argument.

### Flowchart ###
A flowchart to explain how IpAddressExpress works:
![flowchart IpAddressExpress](https://raw.githubusercontent.com/D9ping/IpAddressExpress/master/docs/IpAddressExpress_flowchart_v3.png?raw=true)


### Compiling IpAddressExpress from source
Install the required packages.
```
sudo apt-get install build-essential libcurl4-openssl-dev libsqlite3-dev git-core
```
Get the IpAddressExpress sourcecode in the current folder with:
```
git clone https://github.com/D9ping/IpAddressExpress.git
```
Compile the program with: 
```
cd IpAddressExpress/
make
```

### Use of IpAddressExpress
To use IpAddressExpress for check public IPv4 address change of a server and update the
dynamic DNS entries with a bash script. Do the following:

1. Run ```crontab -e``` to edit your crontab. 
2. Add the following line to run IpAddressExpress every 10 minutes as current user.
```
*/10 * * * * ~/IpAddressExpress/ipaddressexpress --posthook "/bin/sh ~/IpAddressExpress/update_ip_dns.sh"
``` 
Edit the update_ip_dns.sh example bash script with your scripting for updating your dynamic DNS entries.


### Questions and Answers

###### Why not just trust one good public IP service for updating the (Dynamic) DNS?
You could do that but you are putting a lot of trust on one public IP address service 
 that could technically hijack your domain record that you are automatically updating.
 By using IpAddressExpress you don't have to fully trust one public IP address service as lies
 or an errors from one public IP address service are spotted by using consensus with
 an other randomly selected public IP address service. This makes for example fooling your
 server on a dynamic public IPv4 address into thinking incorrectly it needs to update 
 dynamic DNS records almost impossible.

###### Why not ask my router for my public IP address?
Good question. Well technically you could get the public IPv4 address from your router.
But sadly every router manufacturer has a different webinterface for providing this information via their webinterface.
And often the information is secured by a login screen. IpAddressExpress does not rely on the router and there fautures to provide the public IP address and because of this it just works for everyone where there is internet access.

###### What is the maximum downtime for my server could have if my public IP address changes?
It depends on how often you run the ipaddressexpress program for detecting your public ip change.
Also note that if a public ip address service lies to you it will take an extra publicipchangedetector run longer.
And if you are using IpAddressExpress for DDNS then it also depends on how long the DNS entries that needs to change are cached by the DNS servers for the old DNS records to be removed from cache.

###### Should i run IpAddressExpress as often as possible?
No, please be conservative on how often you run ipaddressexpress.
Several public ip services are already serving a lot of requests. 
They don't like it you use too much bandwidth and they will ban/drop you or may send you a http error itstead of your public ip address if you are making too many requests too quickly.

###### Does IpAddressExpress makes two request on execution? 
No, it only make 1 http request if the previous public ip address from last run is known.

###### What about IPv6?
There is no need to support IPv6 as a IPv6 is often a internet routable address.

###### How do i get the current public IPv4 address from commandline?
Run ipaddressexpress with the -showip argument to always print the current public ip address.

###### What command-line arguments can i use?
Run ipaddressexpress with the -h command-line argument for help on all the possible command-line arguments to use.

###### Does IpAddressExpress need root rights?
No, IpAddressExpress can just run fine as a lower privilege user as long as it can read and write to the sqlite database file. The sqlite database file stores all ipservice IpAddressExpress can use and the information of the the last used ipservice. etc.

###### Are all the pubic ip service's hardcoded in the application?
All the services that are used are stored in a SQLite database file. If the SQLite database file exists you can remove, disable or add any public ip services with standard SQL. If the SQLite database file does not exist then the SQLite database file is created and about 20 public ip services are added to the database file by default to use.
