# PublicIpChangeDetector
This C program efficiently request a randomly selected https public IPv4 address service 
for figuring out the public IPv4 address of a server. Then it compares the result with the 
previous result to detect if the public IPv4 address has changed. 
If change is detected from previous request, the change is confirmed with another randomly
 selected public IPv4 address service so if one public IPv4 service is lying, the lie is detected.
By using multiple public IPv4 address services the current public IPv4 can be discovered
 even when some public IPv4 services are down.
If the public IPv4 address is valid and is different from last run the bash script: 
update_ip_dns.sh is called with the new IPv4 address as parameter.


### Flowchart of PublicIpChangeDetector operations ###
This is how PublicIpChangeDetector can securely use a public IPv4 address service.
![flowchart](https://raw.githubusercontent.com/D9ping/PublicIpChangeDetector/master/docs/PublicIpChangeDetector_flowchart_v1.png?raw=true)


### Compiling the PublicIpChangeDetector program
1. Install the required packages.
```
sudo apt-get install libcurl4-openssl-dev git-core
```
2. Get the PublicIpChangeDetector sourcecode in the current folder with:
```
cd /opt/
git clone https://github.com/D9ping/PublicIpChangeDetector.git
```
3. make the program with: 
```
cd /opt/PublicIpChangeDetector/
make
```
4. Edit the update_ip_dns.sh script with your bash code for updating dynamic dns entries.

5. Run ```crontab -e``` to edit (not root!) crontab and add the following line to run it every 12 minutes.
```
*/12 * * * * /opt/PublicIpChangeDetector/publicipchangedetector
```
