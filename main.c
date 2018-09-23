/***************************************************************************
 *    Copyright (C) 2018 D9ping
 *
 * PublicIpChangeDetector is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PublicIpChangeDetector is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PublicIpChangeDetector. If not, see <http://www.gnu.org/licenses/>.
 *
 ***************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <sqlite3.h>
#include "db.h"

#define PROGRAMNAME           "PublicIpChangeDetector"
#define PROGRAMVERSION        "0.5.0-beta"
#define PROGRAMWEBSITE        " (+https://github.com/D9ping/PublicIpChangeDetector)"
#define SEED_LENGTH           32
#define IPV6_TEXT_LENGTH      34
#define MAXNUMSECDELAY        60
#define MAXSIZEIPADDRDOWNLOAD 20
#define MAXLENPATHPOSTHOOK    1023
#define MAXLENURL             1023
#define MAXSIZEAVOIDURLNRS    4096
typedef unsigned int          uint;


struct Settings {
        int secondsdelay;
        int argnposthook;
        bool verbosemode;
        bool silentmode;
        bool retryposthook;
        bool showip;
        bool unsafehttp;
};


/**
    Verify that a IPv4 address is valid by using inet_pton.
    @return 1 if character array is valid IPv4 address.
*/
int is_valid_ipv4_addr(char *ipv4addr)
{
        struct sockaddr_in sa;
        int result = inet_pton(AF_INET, ipv4addr, &(sa.sin_addr));
        return result;
}


/**
    Get a new urlnr that is available to choice.
    @return A random number between 0 and maxurls.
*/
int get_new_random_urlnr(sqlite3 *db, bool unsafehttp, bool verbosemode, bool silentmode)
{
        if (verbosemode) {
                printf("Choose random urlnr.\n");
        }

        bool needaddlasturl = false;
        int lasturlnr = get_config_value_int(db, "lasturlnr");
        if (lasturlnr == -1) {
                needaddlasturl = true;
        }

        // Initializes a random number generator using a seed from /dev/urandom */
        FILE * rand_fd;
        rand_fd = fopen("/dev/urandom", "r");
        if (rand_fd == NULL) {
                if (!silentmode) {
                        fprintf(stderr, "Error: could not open /dev/urandom\n");
                }

                exit(EXIT_FAILURE);
        }

        int allowedprotocoltypes = 1;
        if (unsafehttp) {
                allowedprotocoltypes = 0;
        }

        const int numavailableipservices = get_count_available_ipservices(db, allowedprotocoltypes);
        int availableurlnrs[numavailableipservices];
        get_urlnrs_ipservices(db, availableurlnrs, 0, allowedprotocoltypes);

        /*
        // For debugging print availableurlnrs array.
        if (verbosemode && !silentmode) {
                for (int i = 0; i < numavailableipservices; ++i) {
                        printf("availableurlnrs[%d] = %d.\n", i, availableurlnrs[i]);
                }
        }
        */

        uint *seed = (uint *) malloc(SEED_LENGTH * sizeof(uint));
        int resreadrnd = fread(seed, sizeof(uint), SEED_LENGTH, rand_fd);
        if (resreadrnd <= 0) {
                if (!silentmode) {
                        printf("Could not read from /dev/urandom.\n");
                }
        }

        srand(*seed);
        fclose(rand_fd);
        // Choose a new random position in availableurlnrs. Between 0 and numavailableipservices.
        int p = (rand() % numavailableipservices);
        int urlnr = availableurlnrs[p];
        if (numavailableipservices > 0 && lasturlnr >= 0)
        {
                int tries = 0;
                int maxtries = 2;
                while (urlnr == lasturlnr && tries < maxtries)
                {
                        p = (rand() % numavailableipservices);
                        urlnr = availableurlnrs[p];
                        ++tries;
                }

                if (urlnr == lasturlnr && tries == maxtries && !silentmode) {
                        printf("Maximum number of retries choicing different urlnr reached.\n\
 Use same urlnr as last run.\n");
                }
        }

        free(seed);
        if (needaddlasturl) {
                add_config_value_int(db, "lasturlnr", urlnr, verbosemode);
        } else {
                update_config_value_int(db, "lasturlnr", urlnr, verbosemode);
        }

        return urlnr;
}


/**
    Strip everything in the character array from the new line character till the end
    of the array. So str does not contain any new line character anymore.
*/
void strip_on_newlinechar(char *str, int strlength)
{
        for (int i = 0; i < strlength; ++i) {
                if (str[i] == '\n') {
                        // Added null character here so it's shorter just before first line feed.
                        str[i] = '\0';
                        return;
                }
        }
}


/**
    Read a text file with ip address.
    @return A character array with the ip address without a newline character.
*/
char * read_file_ipaddr(char *filepathip, bool silentmode)
{
        size_t len = 0;
        ssize_t bytesread;
        FILE *fpfileip;
        fpfileip = fopen(filepathip, "r");
        if (fpfileip == NULL) {
                if (!silentmode) {
                        fprintf(stderr, "Error: Could not get public ip from file.\n");
                }

                exit(EXIT_FAILURE);
        }

        char *ipaddrstr;
        bytesread = getline(&ipaddrstr, &len, fpfileip);
        // A hex text full out written IPv6 address is 34 characters +1 for null character.
        if (bytesread > IPV6_TEXT_LENGTH) {
                fclose(fpfileip);
                if (!silentmode) {
                        fprintf(stderr, "Error: response server too long.\n");
                }

                exit(EXIT_FAILURE);
        }

        fclose(fpfileip);
        strip_on_newlinechar(ipaddrstr, bytesread);
        return ipaddrstr;
}


/**
        Parse http status code.
 */
void parse_httpcode_status(int httpcode, sqlite3 *db, int urlnr)
{
        switch (httpcode) {
        case 420L:
        case 429L:
        case 503L:
        case 509L:
                printf("Warn: rate limiting active.\
 Avoid the current public ip address service for some time.");
                update_disabled_ipsevice(db, urlnr, true);
                exit(EXIT_FAILURE);
        case 408L:
        case 500L:
        case 502L:
        case 504L:
                printf("Warn: used public ip address service has an error or other issue\
 (http error: %d).\n", httpcode);
                // should not be avoid forever, but for some time only.
                update_disabled_ipsevice(db, urlnr, true);
                break;
        case 401L:
        case 403L:
        case 404L:
        case 410L:
                printf("Warn: the used public ip address service has quit or does not\
 want automatic use.\nNever use this public ip service again.\n");
                // do disable forever.
                update_disabled_ipsevice(db, urlnr, false);
                break;
        case 301L:
        case 302L:
        case 308L:
                printf("Warn: public IP address service has changed url and is redirecting\
 (http status code: %d).\n", httpcode);
                // do disable forever.
                update_disabled_ipsevice(db, urlnr, false);
                break;
        }
}


/**
 * Download the ip address from a ipservice.
 * @return ip address
 */
char * download_ipaddr_ipservice(char *ipaddr, const char *urlipservice, sqlite3 *db, int urlnr,
                                 bool unsafehttp, bool silentmode, int * httpcodestatus)
{
        //char * downloadtempfilepath;
        char * downloadtempfilepath = malloc(20 * sizeof(char));
        downloadtempfilepath = "/tmp/tempip.txt";
        FILE *fpdownload;
        // mode w+: truncates the file to zero length if it exists,
        // otherwise creates a file if it does not exist.
        fpdownload = fopen(downloadtempfilepath, "w+");
        curl_global_init(CURL_GLOBAL_DEFAULT);
        CURL * curlsession = curl_easy_init();
        if (!curlsession || curlsession == NULL) {
                if (!silentmode) {
                        fprintf(stderr, "Error: should not setup curl session.\n");
                }

                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                exit(EXIT_FAILURE);
        }

        curl_easy_setopt(curlsession, CURLOPT_URL, urlipservice);
        curl_easy_setopt(curlsession, CURLOPT_HTTPGET, 1L);
        // Write to fpdownload
        curl_easy_setopt(curlsession, CURLOPT_WRITEDATA, fpdownload);
        // Default 300s, changed to max. 20 seconds to connect
        curl_easy_setopt(curlsession, CURLOPT_CONNECTTIMEOUT, 20L);
        // Default timeout is 0/never. changed to 25 seconds
        curl_easy_setopt(curlsession, CURLOPT_TIMEOUT, 25L);
        // Never follow redirects.
        curl_easy_setopt(curlsession, CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curlsession, CURLOPT_MAXREDIRS, 0L);
        // Resolve host name using IPv4-names only
        curl_easy_setopt(curlsession, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        if (!unsafehttp) {
                // Only allow https to be used.
                curl_easy_setopt(curlsession, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
        } else {
                // Allow https and unsafe http.
                curl_easy_setopt(curlsession, CURLOPT_PROTOCOLS,
                                 CURLPROTO_HTTPS | CURLPROTO_HTTP);
        }

        char useragent[128];
        strcpy(useragent, PROGRAMNAME);
        strcat(useragent, "/");
        strcat(useragent, PROGRAMVERSION);
        strcat(useragent, PROGRAMWEBSITE);
        curl_easy_setopt(curlsession, CURLOPT_USERAGENT, useragent);
        // Perform the request, res will get the return code
        CURLcode res;
        res = curl_easy_perform(curlsession);
        // Check for errors
        if (res != CURLE_OK) {
                if (!silentmode) {
                        fprintf(stderr,
                                "Error: %s, url: %s\n",
                                curl_easy_strerror(res),
                                urlipservice);
                }

                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                //write_avoid_urlnr(urlnr, silentmode);  TODO: replace with db.
                exit(EXIT_FAILURE);
        }

        int httpcode = 0;
        curl_easy_getinfo(curlsession, CURLINFO_RESPONSE_CODE, &httpcode);

        // Check the filelength of the downloaded file.
        fclose(fpdownload);
        fpdownload = fopen(downloadtempfilepath, "r");
        fseek(fpdownload, 0L, SEEK_END);
        unsigned long long int downloadedfilesize = ftell(fpdownload);
        fclose(fpdownload);

        // TODO: redirect url, print it?
        //char *followurl = NULL;
        //curl_easy_getinfo(curlsession, CURLINFO_REDIRECT_URL, &followurl);

        // Cleanup curl.
        curl_easy_cleanup(curlsession);

        // Set HTTP status code.
        *httpcodestatus = httpcode;

        // Check filesize
        if (downloadedfilesize == 0) {
                if (!silentmode) {
                        fprintf(stderr, "Error: downloaded file is empty.\n");
                }

                exit(EXIT_FAILURE);
        } else if (downloadedfilesize > MAXSIZEIPADDRDOWNLOAD) {
                if (!silentmode) {
                        fprintf(stderr, "Error: response ip service too big.\n");
                }

                exit(EXIT_FAILURE);
        }

        ipaddr = read_file_ipaddr(downloadtempfilepath, silentmode);

        //return http_code;
        return ipaddr;
}


/**
 * Parse the command line arguments to the settings struct.
 */
struct Settings parse_commandline_args(int argc, char **argv, struct Settings settings)
{
        bool argnumdelaysec = false;
        bool argposthook = false;
        // Parse commandline arguments and set settings struct.
        for (int n = 1; n < argc; ++n) {
                if (argnumdelaysec) {
                        argnumdelaysec = false;
                        // Check if argv[n] valid is integer
                        int lenarg = strlen(argv[n]);
                        for (int i = 0; i < lenarg; ++i) {
                                if (!isdigit(argv[n][i])) {
                                        if (!settings.silentmode) {
                                                fprintf(stderr,
                                                        "Error: invalid number of seconds delay.\n");
                                        }

                                        exit(EXIT_FAILURE);
                                }
                        }

                        settings.secondsdelay = atoi(argv[n]);
                        continue;
                } else if (argposthook) {
                        argposthook = false;
                        // Check length posthook
                        if (strlen(argv[n]) > MAXLENPATHPOSTHOOK) {
                                if (!settings.silentmode) {
                                        fprintf(stderr, "Error: posthook command too long.\n");
                                }

                                exit(EXIT_FAILURE);
                        }

                        // For now, if multiple posthook's are provided
                        // override the command with last posthook command.
                        settings.argnposthook = n;
                        continue;
                }

                if (strcmp(argv[n], "--posthook") == 0) {
                        argposthook = true;
                } else if (strcmp(argv[n], "--delay") == 0) {
                        argnumdelaysec = true;
                } else if (strcmp(argv[n], "--retryposthook") == 0) {
                        settings.retryposthook = true;
                } else if (strcmp(argv[n], "--showip") == 0) {
                        settings.showip = true;
                } else if (strcmp(argv[n], "--unsafehttp") == 0) {
                        settings.unsafehttp = true;
                } else if (strcmp(argv[n], "--failsilent") == 0) {
                        settings.silentmode = true;
                } else if (strcmp(argv[n], "--version") == 0) {
                        printf("%s %s\n", PROGRAMNAME, PROGRAMVERSION);
                        exit(EXIT_SUCCESS);
                } else if (strcmp(argv[n], "-v") == 0 || strcmp(argv[n], "--verbose") == 0) {
                        settings.verbosemode = true;
                } else if (strcmp(argv[n], "-h") == 0 || strcmp(argv[n], "--help") == 0) {
                        printf("--posthook      The script or program to run on public IPv4 address change.\n\
                Put the command between quotes. Spaces in path are currently not supported.\n");
                        printf("--retryposthook Rerun posthook on next run if posthook command \n\
                did not return 0 as exit code.\n");
                        printf("--showip        Always print the currently confirmed public IPv4 address.\n");
                        printf("--unsafehttp    Allow the use of http public ip services, no TLS/SSL.\n");
                        printf("--delay 1-59    Delay the execution of this program with X number of seconds.\n");
                        printf("--failsilent    Don't print issues to stderr.\n");
                        printf("--version       Print the version of this program and exit.\n");
                        printf("-v --verbose    Run in verbose mode, output what this program does.\n");
                        printf("-h --help       Print this help message.\n");
                        exit(EXIT_SUCCESS);
                } else if (!settings.silentmode) {
                        fprintf(stderr, "Ignore unknown argument \"%s\".\n", argv[n]);
                }
        }

        return settings;
}


int main(int argc, char **argv)
{
        struct Settings settings;
        // Set default values:
        settings.secondsdelay = 0;
        settings.argnposthook = 0;
        settings.retryposthook = false;
        settings.unsafehttp = false;
        settings.showip = false;
        settings.silentmode = false;
        settings.verbosemode = false;
        settings = parse_commandline_args(argc, argv, settings);

        /*
        if (settings.verbosemode) {
                // Display settings
                printf(" --- Current program settings --- \n");
                printf("settings.verbosemode = %d\n", settings.verbosemode);
                printf("settings.secondsdelay = %d\n", settings.secondsdelay);
                printf("settings.showip = %d\n", settings.showip);
                printf("settings.silentmode = %d\n", settings.silentmode);
                printf("settings.unsafehttp = %d\n", settings.unsafehttp);
                printf("settings.retryposthook = %d\n", settings.retryposthook);
                printf("settings.argnposthook = %d\n", settings.argnposthook);
                if (settings.argnposthook > 1) {
                        printf("posthook = '%s'\n", argv[settings.argnposthook]);
                }

                printf(" --------------------------------- \n");
        }
        */

        if (settings.secondsdelay > 0 && settings.secondsdelay < 60) {
                if (settings.verbosemode) {
                        printf("Delay %d seconds.\n", settings.secondsdelay);
                }

                sleep(settings.secondsdelay);
        } else if (settings.secondsdelay >= MAXNUMSECDELAY && !settings.silentmode) {
            fprintf(stderr, "Ingoring secondsdelay. secondsdelay has to be less than %d seconds.\n", MAXNUMSECDELAY);
        }

        bool dbsetup = false;
        if (access("data.db", F_OK) == -1 ) {
                dbsetup = true;
                if (settings.verbosemode) {
                        printf("data.db does not exists.\n");
                }
        }

        /* Setup database connection */
        sqlite3 *db;
        int rc;
        rc = sqlite3_open("data.db", &db);
        if (rc) {
                fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                return(0);
        } else if (settings.verbosemode) {
                printf("Opened database successfully.\n");
        }

        if (dbsetup) {
                create_table_ipservice(db, settings.verbosemode);
                create_table_config(db, settings.verbosemode);
                add_config_value_int(db, "lasturlnr", -2, settings.verbosemode);
                /* added default ipservice records */
                add_ipservice(db, 0, "https://ipinfo.io/ip", false, 1, settings.verbosemode);
                add_ipservice(db, 1, "https://api.ipify.org/?format=text", false, 1, settings.verbosemode);
                add_ipservice(db, 2, "https://wtfismyip.com/text", false, 1, settings.verbosemode);
                add_ipservice(db, 3, "https://v4.ident.me/", false, 1, settings.verbosemode);
                add_ipservice(db, 4, "https://ipv4.icanhazip.com/", false, 1, settings.verbosemode);
                add_ipservice(db, 5, "https://checkip.amazonaws.com/", false, 1, settings.verbosemode);
                add_ipservice(db, 6, "https://bot.whatismyipaddress.com/", false, 1, settings.verbosemode);
                add_ipservice(db, 7, "https://secure.informaction.com/ipecho/", false, 1, settings.verbosemode);
                add_ipservice(db, 8, "https://l2.io/ip", false, 1, settings.verbosemode);
                add_ipservice(db, 9, "https://www.trackip.net/ip", false, 1, settings.verbosemode);
                add_ipservice(db, 10, "https://ip4.seeip.org/", false, 1, settings.verbosemode);
                add_ipservice(db, 11, "https://locate.now.sh/ip", false, 1, settings.verbosemode);
                add_ipservice(db, 12, "https://tnx.nl/ip", false, 1, settings.verbosemode);
                add_ipservice(db, 13, "https://diagnostic.opendns.com/myip", false, 1, settings.verbosemode);
                add_ipservice(db, 14, "https://ip4.seeip.org/", false, 1, settings.verbosemode);
                add_ipservice(db, 15, "http://myip.dnsomatic.com/", false, 0, settings.verbosemode);
                add_ipservice(db, 16, "http://whatismyip.akamai.com/", false, 0, settings.verbosemode);
                add_ipservice(db, 17, "http://myexternalip.com/raw", false, 0, settings.verbosemode);
                add_ipservice(db, 18, "https://ipecho.net/plain", false, 1, settings.verbosemode);
                add_ipservice(db, 19, "http://plain-text-ip.com/", false, 0, settings.verbosemode);
        }

        int  num_all_urls = get_count_all_ipservices(db);
        if (num_all_urls < 2) {
                if (!settings.silentmode) {
                        printf("No enough url's added to database need at least 2 services.\n");
                        exit(EXIT_FAILURE);
                }
        }

        int urlnr = get_new_random_urlnr(db, settings.unsafehttp, settings.verbosemode, settings.silentmode);
        const char *urlipservice;
        urlipservice = get_url_ipservice(db, urlnr);
        if (settings.verbosemode) {
                printf("Use: %s for getting public IPv4 address.\n", urlipservice);
        }

        int httpcodestatus = 0;
        char * ipaddrnow;
        ipaddrnow = download_ipaddr_ipservice(ipaddrnow,
                                              urlipservice,
                                              db,
                                              urlnr,
                                              settings.unsafehttp,
                                              settings.silentmode,
                                              &httpcodestatus);
        parse_httpcode_status(httpcodestatus, db, urlnr);
        if (is_valid_ipv4_addr(ipaddrnow) != 1) {
                free(ipaddrnow);
                if (!settings.silentmode) {
                        fprintf(stderr, "Error: got invalid IP(IPv4) address '%s' from '%s'.\n", ipaddrnow, urlipservice);
                }

                exit(EXIT_FAILURE);
        }

        char *ipaddrconfirm;
        if (is_config_exists(db, "ipwas") == true) {
                ipaddrconfirm = get_config_value_str(db, "ipwas");
        } else {
                if (settings.verbosemode) {
                        printf("First run of %s.\n", PROGRAMNAME);
                }

                const char *confirmurl;
                urlnr = get_new_random_urlnr(db, settings.unsafehttp, settings.verbosemode, settings.silentmode);
                confirmurl = get_url_ipservice(db, urlnr);
                if (settings.verbosemode) {
                        printf("Using %s to confirm current public ip address with.\n", confirmurl);
                }

                int httpcodeconfirm = -1;
                ipaddrconfirm = download_ipaddr_ipservice(ipaddrconfirm,
                                                          confirmurl,
                                                          db,
                                                          urlnr,
                                                          settings.unsafehttp,
                                                          settings.silentmode,
                                                          &httpcodeconfirm);
                parse_httpcode_status(httpcodeconfirm, db, urlnr);
                if (is_valid_ipv4_addr(ipaddrconfirm) != 1) {
                        free(ipaddrconfirm);
                        //unlink(filepathipnow);
                        if (!settings.silentmode) {
                                fprintf(stderr, "Error: invalid IP(IPv4) address for first\
 run confirmation from %s.\n", confirmurl);
                        }

                        exit(EXIT_FAILURE);
                }

                // Check for ip address different between the two requuested services on first run.
                if (strcmp(ipaddrnow, ipaddrconfirm) != 0) {
                        if (!settings.silentmode) {
                                fprintf(stderr, "Alert: one of the ip address services\
 could have lied to us.\nTry getting public ip again on next run.\n");
                                fprintf(stderr, "IPv4: %s from %s.\n", ipaddrnow, urlipservice);
                                fprintf(stderr, "IPv4: %s from %s.\n", ipaddrconfirm,
                                        confirmurl);
                        }

                        exit(EXIT_FAILURE);
                }
        }

        if (strcmp(ipaddrnow, ipaddrconfirm) != 0) {
                // Ip address has changed.
                if (settings.verbosemode) {
                        printf("Public ip change detected, ip address different from last\
 run.\n");
                }

                urlnr = get_new_random_urlnr(db, settings.unsafehttp, settings.verbosemode, settings.silentmode);
                const char *confirmchangeurl;
                confirmchangeurl = get_url_ipservice(db, urlnr);
                if (settings.verbosemode) {
                        printf("Public ip service %s is used to confirm ip address\n", confirmchangeurl);
                }

                char *ipaddrconfirmchange;
                int httpcodeconfirmchange = -2;
                ipaddrconfirmchange = download_ipaddr_ipservice(ipaddrconfirmchange,
                                                                confirmchangeurl,
                                                                db,
                                                                urlnr,
                                                                settings.unsafehttp,
                                                                settings.silentmode,
                                                                &httpcodeconfirmchange);
                parse_httpcode_status(httpcodeconfirmchange, db, urlnr);

                if (is_valid_ipv4_addr(ipaddrconfirmchange) != 1) {
                        //free(ipaddrnowconfirm);  is const
                        if (!settings.silentmode) {
                                fprintf(stderr, "Error: invalid IP(IPv4) address returned\
 from %s as confirm public ip service.\n", confirmchangeurl);
                        }

                        if (settings.showip) {
                                // Show old valid ip address.
                                printf("%s", ipaddrconfirm);
                        }

                        exit(EXIT_FAILURE);
                }

                // Check if new ip address with different service is the same new ip address.
                if (strcmp(ipaddrnow, ipaddrconfirmchange) == 0) {
                        if (!settings.silentmode) {
                                fprintf(stderr, "Alert: the ip address service could have\
 lied to us.\nIt's now unknown if public ip address has actually changed.\n");
                        }

                        if (settings.showip) {
                                // Show old ip address.
                                printf("%s", ipaddrconfirm);
                        }

                        exit(EXIT_FAILURE);
                }

                if (settings.verbosemode) {
                        printf("Execute posthook command with \"%s\" as argument.\n", ipaddrnow);
                }

                if (settings.argnposthook <= 1) {
                        //cmdposthook[] = "/bin/sh ./update_ip_dns.sh ";
                        if (!settings.silentmode) {
                                fprintf(stderr, "Error: no posthook provided.\n");
                        }

                        exit(EXIT_FAILURE);
                }

                char cmdposthook[MAXLENPATHPOSTHOOK + 1];
                if (strchr(ipaddrnow, '"') != NULL) {
                        if (!settings.silentmode) {
                                fprintf(stderr, "Error: quote(s) in ip address.\n");
                        }

                        exit(EXIT_FAILURE);
                } else if (strchr(argv[settings.argnposthook], '"') != NULL) {
                        if (!settings.silentmode) {
                                fprintf(stderr, "Error: quote in posthook command.\n");
                        }

                        exit(EXIT_FAILURE);
                }

                snprintf(cmdposthook, MAXLENPATHPOSTHOOK, "\"%s\" \"%s\"", argv[settings.argnposthook], ipaddrnow);
                /* printf("cmdposthook = [ %s ]\n", cmdposthook); // DEBUG */
                unsigned short exitcode = system(cmdposthook);
                if (exitcode != 0) {
                        if (!settings.silentmode) {
                                fprintf(stderr,
                                        "Error: script returned error (exitcode %hu).\n",
                                        exitcode);
                        }

                        if (settings.retryposthook) {
                                if (settings.showip) {
                                        // Do show new ip address.
                                        printf("%s", ipaddrnow);
                                }

                                // Run posthook next time again because ipwas is not updated to new ip in database.
                                exit(EXIT_FAILURE);
                        }
                }

                if (is_config_exists(db, "ipwas") == true) {
                        update_config_value_str(db, "ipwas", ipaddrnow, settings.verbosemode);
                } else {
                        add_config_value_str(db, "ipwas", ipaddrnow, settings.verbosemode);
                }
        } else if (settings.verbosemode) {
                printf("The current public ip is the same as the public ip from last ipservice.\n");
                if (is_config_exists(db, "ipwas") == false) {
                        // Readded missing ipwas config value.
                        add_config_value_str(db, "ipwas", ipaddrnow, settings.verbosemode);
                }
        }

        if (settings.showip) {
                // Show current ip address.
                printf("%s", ipaddrnow);
        }

        return EXIT_SUCCESS;
}

