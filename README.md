# PublicIpChangeDetector

### Why use PublicIpChangeDetector?
Compared to shell scripts for figuring out the public IPv4 address PublicIpChangeDetector will more 
securely and more reliable detect the current IPv4 address of a computer on an internal network.
It's more secure because it does not require to trust a single server and uses https for tamper protection.
PublicIpChangeDetector requires consensus between two randomly selected public IPv4 address HTTPS services.
PublicIpChangeDetector is written in C so it executes fast and efficiently.
By using multiple public IPv4 address https services the current public IPv4 can be discovered after some time
 even when some public IPv4 https services are down.
 
### How PublicIpChangeDetector detects public IPv4 address change?
PublicIpChangeDetector randomly selects a https public IPv4 address service from a list 
of known https public IPv4 address services for figuring out the public IPv4 address. 
It then compares the result with the previous result to detect if the public IPv4 address has changed. 
If change is detected from previous request, the change is confirmed with another randomly
 selected public IPv4 address https service so if one public IPv4 https service is lying, the lie is detected.
If the public IPv4 address is valid and is different from last run the posthook command execute
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

###### Do you have a precompiled binary?
No, it's still in heavy development. 
Besides compiling the binary yourself can create a more optimized binary for the processor you are using.

###### What is the maximum downtime for my server if my dynamic public ip address changes?
It depends on how often you run the publicipchangedetector program for detecting your public ip change.
Also note that if a public ip address service lies to you it will take an extra publicipchangedetector run longer.
And it also depends on how long the dns entries that needs to change are cached by dns servers.

###### Should i run publicipchangedetector as often as possible?
No, please be conservative on how often you run publicipchangedetector. Several public ip services are already serving a lot of requests.
They don't like it you use too much bandwidth and they will ban/drop you or send you 403 error if you are make too many requests too quickly.

###### What command-line arguments has publicipchangedetector?
Run: /opt/PublicIpChangeDetector/publicipchangedetector -h  
for help on the possible command-line arguments and there use.

