# PublicIpChangeDetector
This C program efficiently requests randomly a HTTP(s) public IPv4 API. 
Then it compares the result with the previous result to detect if the public IPv4 address has changed. 
The change is confirmed with another randomly selected public https/http IPv4 API so if one public ip API lies, the lie is detected. If the public IPv4 address is valid and is different from last run the bash script: 
update_ip_dns.sh is called with the new IPv4 address.
