#!/usr/bin/env sh
# This bash/dash script is called by publicipchangedetector if the public ip address has changed
# with the first argument being the new IPv4 address.
if [ "$1" = "" ];
then
    echo "Missing IPv4 address argument."
    exit 1
fi

readonly IPv4addr="$1"
echo "New IPv4 address is: $IPv4addr "
readonly DDNSUPDATER_USERAGENT="ddnsupdater/0.5 (youremail@yourdomain.test)"

# Update Hurricane Electric DNS.
updatedns_hurricaneelectric() {
    local hostname=$1
    local domain=$2
    local password=$3
    echo "Update DNS HurricaneElectric: $hostname.$domain to $4."
    if [ "$hostname" = "@" ];
    then
        curl -s --proto -all,https --tlsv1.2 --user-agent "$5" "https://$domain:$password@dyn.dns.he.net/nic/update?hostname=$domain&myip=$4" -o /dev/null 
    else
        curl -s --proto -all,https --tlsv1.2 --user-agent "$5" "https://$hostname.$domain:$password@dyn.dns.he.net/nic/update?hostname=$hostname.$domain&myip=$4" -o /dev/null 
    fi
}


# Update NameCheap DNS.
updatedns_namecheap() {
    local hostname=$1
    local domain=$2
    local password=$3
    echo "Update DNS NameCheap: $hostname.$domain to $4."
    curl -s --proto -all,https --tlsv1.2 --user-agent "$5" "https://dynamicdns.park-your-domain.com/update?domain=$domain&password=$password&host=$hostname&ip=$4" --max-filesize 1048576 -o /dev/null 
}


# Put your dynamic dns entries here, use e.g.:
#updatedns_hurricaneelectric 'www' 'yourdomain.test' 'yoursecretapikeyhere' "$IPv4addr" "$DDNSUPDATER_USERAGENT"
#updatedns_hurricaneelectric '@' 'yourdomain.test' 'yoursecretapikeyhere' "$IPv4addr" "$DDNSUPDATER_USERAGENT"
#updatedns_namecheap 'www' 'yourotherdomain.test' 'yoursecretapikey2here' "$IPv4addr" "$DDNSUPDATER_USERAGENT"
#updatedns_namecheap '@' 'yourotherdomain.test' 'yoursecretapikey2here' "$IPv4addr" "$DDNSUPDATER_USERAGENT"
# etc. etc.
#
# Run: chmod o-rwx update_ip_dns.sh to make this file not readable by others.


exit 0
