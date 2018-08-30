![PublicIpChangeDetector logo](docs/logo.png?raw=true "PublicIpChangeDetector")

[<img src="https://img.shields.io/github/issues/D9ping/PublicIpChangeDetector.svg?style=flat-square" alt="Open issues" />](https://github.com/D9ping/PublicIpChangeDetector/issues) 
[![Build Status](https://travis-ci.org/D9ping/PublicIpChangeDetector.svg?branch=master)](https://travis-ci.org/D9ping/PublicIpChangeDetector)
### Why use PublicIpChangeDetector?
Compared to shell scripts for figuring out the public IPv4 address PublicIpChangeDetector will more 
securely and more reliable detect the current public IPv4 address.
It's more secure because it does not require to trust a single server and uses https for tamper protection.
What makes PublicIpChangeDetector more secure is, that it requires consensus between two randomly selected public IPv4 address web services.
Because of this it's not possible for a single public IPv4 address web service to fool the computer
 incorrectly thinking the public ip address has been changed.
And by using multiple public IPv4 address web services the current public IPv4 can be discovered after some time
 even when some public IPv4 web services are down.
PublicIpChangeDetector is written in C so it executes fast and efficiently.
 
### How PublicIpChangeDetector detects public IPv4 address change?
PublicIpChangeDetector randomly selects a https public IPv4 address service from a list 
of known https public IPv4 address services for figuring out the public IPv4 address. 
It then compares the result with the previous result to detect if the public IPv4 address has changed. 
If change is detected from previous request, the change is confirmed with another randomly
 selected public IPv4 address https service so if one public IPv4 https service is lying, the lie is detected.
If the public IPv4 address is valid and is different from last run the posthook command is executed
 with the new IPv4 address as first commandline parameter.

### Flowchart of PublicIpChangeDetector operations ###
This flowcharts show is how PublicIpChangeDetector works:
![flowchart](https://raw.githubusercontent.com/D9ping/PublicIpChangeDetector/master/docs/PublicIpChangeDetector_flowchart_v2.png?raw=true)


### Compiling PublicIpChangeDetector from source
Install the required packages.
```
sudo apt-get install build-essential libcurl4-openssl-dev git-core
```
 Get the PublicIpChangeDetector sourcecode in the folder /opt/ with:
```
cd /opt/
git clone https://github.com/D9ping/PublicIpChangeDetector.git
```
Compile the program with: 
```
cd PublicIpChangeDetector/
make
```

### Use of PublicIpChangeDetector
To use PublicIpChangeDetector for check public IPv4 address change of a server and update the
dynamic dns entries with a bash script. Do the following:

1. Run ```crontab -e``` to edit your crontab. 
2. Add the following line to run PublicIpChangeDetector every 10 minutes as current user.
```
*/10 * * * * /opt/PublicIpChangeDetector/publicipchangedetector --posthook "/bin/sh /opt/PublicIpChangeDetector/update_ip_dns.sh"
``` 
Edit the update_ip_dns.sh example bash script with your scripting for updating your dynamic dns entries.


### Questions and Answers

###### Why not ask my router for my public ip address?
Well technically you could get the public IPv4 address from your router.
But sadly every router manufacturer has a different (web)interface for providing this information.
And often the information is secured by a login screen. 
PublicIpChangeDetector does not rely on the router and because of this just works everywhere where there is internet access.

###### Why not just trust one good public ip service for updating the dynamic dns(ddns)?
You could do that but you are putting a lot of trust on one public ip address service 
 that could technically control your domain records you are automatically updating.
 By using publicipchangedetector you don't have to fully trust one public ip address service as lies
 or an errors from one public ip address service are spotted by using consensus with
 an other randomly selected public ip address service.

###### What is the maximum downtime for my server if my dynamic public ip address changes?
It depends on how often you run the publicipchangedetector program for detecting your public ip change.
Also note that if a public ip address service lies to you it will take an extra publicipchangedetector run longer.
And it also depends on how long the dns entries that needs to change are cached by dns servers.

###### Should i run publicipchangedetector as often as possible?
No, please be conservative on how often you run publicipchangedetector. Several public ip services are already serving a lot of requests.
They don't like it you use too much bandwidth and they will ban/drop you or maybe send you 
a http forbidden(403) error if you are making too many requests too quickly.

###### Does publicipchangedetector makes two request on execution? 
No, it only make 1 http request if previous public ip address from last run is known.

###### What about IPv6?
There is no need to support IPv6 as it does not use NAT.

###### How do i get the current public IPv4 address from commandline?
Run publicipchangedetector with the -showip argument to always print the current public ip address.

###### What command-line arguments can i use?
Run publicipchangedetector with the -h argumemt for help on all the possible command-line arguments.

###### Do you have a precompiled binary?
No, PublicIpChangeDetector is still in development. 
Compiling the binary yourself can create a more optimized binary for the processor you are using.

###### What features still needs to be implemented?
TODO's:
 - Added support for a SQLlite database.
 - Allow to temporary disable a public ip service instead of avoiding forever.


## Support 
If you find it useful lease consider buying the founders a drink.
[![Beerpay](https://beerpay.io/D9ping/PublicIpChangeDetector/badge.svg?style=beer-square)](https://beerpay.io/D9ping/PublicIpChangeDetector)  [![Beerpay](https://beerpay.io/D9ping/PublicIpChangeDetector/make-wish.svg?style=flat-square)](https://beerpay.io/D9ping/PublicIpChangeDetector?focus=wish)