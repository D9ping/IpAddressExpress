# PublicIpChangeDetector
This C program efficiently requests randomly a HTTP(s) public IPv4 API. 
Then it compares the result with the previous result to detect if the public IPv4 address has changed. 
The change is confirmed with another randomly selected public https/http IPv4 API so if one public ip API lies, the lie is detected. If the public IPv4 address is valid and is different from last run the bash script: 
update_ip_dns.sh is called with the new IPv4 address.

### Compiling PublicIpChangeDetector
1. Install the required packages.
```
sudo apt-get install libcurl4-openssl-dev git-core
```
2. Get the PublicIpChangeDetector sourcecode ìn the current folder with:
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

5. Run ```crontab -e``` to edit your crontab to run publicipchangedetector every 10 minutes.
```
*/10 * * * * /opt/PublicIpChangeDetector/publicipchangedetector
```
