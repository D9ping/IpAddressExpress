/***************************************************************************
 *    Copyright (C) 2018-2020 D9ping
 *
 * IpAddressExpress is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * IpAddressExpress is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with IpAddressExpress. If not, see <http://www.gnu.org/licenses/>.
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

#define PROGRAMNAME           "IpAddressExpress"
#define PROGRAMVERSION        "1.0.1"
#define PROGRAMWEBSITE        " (+https://github.com/D9ping/IpAddressExpress)"
#define DATABASEFILENAME      "ipaddressexpress.db"
#define CONFIGNAMEPREVIP      "lastrunip"
#define CONFIGNAMELASTRUNDT   "lastrundatetime"
#define SEED_LENGTH           32
#define IPV6_TEXT_LENGTH      34
#define MAXNUMSECDELAY        60
#define MAXSIZEIPADDRDOWNLOAD 20
#define MAXLENPATHPOSTHOOK    1023
#define MAXLENURL             1023
#define MAXSIZEAVOIDURLNRS    4096
#define MAXPRIORITY           9
#define MAXCHOOSERETRIES      8
#define PROTOCOLDNS           0
#define PROTOCOLHTTP          1
#define PROTOCOLHTTPS         2
typedef unsigned int          uint;

struct Settings {
        int secondsdelay;
        int argnposthook;
        int errorwait;
        bool verbosemode;
        bool silentmode;
        bool retryposthook;
        bool showip;
        bool showlastrun;
        bool savelastrun;
        bool unsafehttp;
        bool unsafedns;
        bool tripleconfirm;
};

/**
 * Get the current date and time in ISO8601 format without timezone.
 * @return An character array of 19 bytes(without null terminator) with the current date and time.
 */
char * get_current_time_str(char *iso8601timebuf)
{
        time_t rawtime;
        struct tm * timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(iso8601timebuf, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
        return iso8601timebuf;
}

/**
 * Print time and date with an error message to stderr output.
 * @param char[] errormsg
 */
void print_dt_error(char * errormsg)
{
        char * iso8601timebuf = malloc(20 * sizeof(char));
        iso8601timebuf = get_current_time_str(iso8601timebuf);
        fprintf(stderr, "%s %s", iso8601timebuf, errormsg);
}

/**
 * Print date and time and error message with an url to stderr output.
 * @param char[] errormsgformat The format of the error message.
 * @param char[] url            The url added in the error message.
 */
void print_error_with_url(char * errormsgformat, const char * url)
{
        char errormsg[2304];
        int cw;
        cw = snprintf(errormsg,
                      2304,
                      errormsgformat,
                      url);
        if (cw >= 0 && cw <= 2304) {
                print_dt_error(errormsg);
        }
}

/**
 * Print the different results from public ip address services.
 */
void print_detected_difference(char * ipaddrnow, char * ipaddrconfirm, const char * urlipservice, const char * confirmurl)
{
        print_dt_error("Alert: one of the ip address services could have lied.\n");
        fprintf(stderr, "IPv4: %s from %s.\n", ipaddrnow, urlipservice);
        fprintf(stderr, "IPv4: %s from %s.\n", ipaddrconfirm, confirmurl);
}

/**
 * Verify that a IPv4 address is valid by using inet_pton.
 * @return 1 if character array is valid IPv4 address.
 */
int is_valid_ipv4_addr(char *ipv4addr)
{
        struct sockaddr_in sa;
        int result = inet_pton(AF_INET, ipv4addr, &(sa.sin_addr));
        return result;
}

/**
 * Get a new urlnr that is available to choice.
 * @return A random number between 0 and maxurls.
 */
int get_new_random_urlnr(sqlite3 *db, bool unsafehttp, bool unsafedns, bool verbosemode, bool silentmode)
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
                        print_dt_error("Error: could not open /dev/urandom\n");
                }

                exit(EXIT_FAILURE);
        }

        int allowedprotocoltypes = 2;
        if (unsafehttp) {
                allowedprotocoltypes = 1;
        } else if (unsafedns) {
                allowedprotocoltypes = 0;
        }

        const int numavailableipservices = get_count_available_ipservices(db, allowedprotocoltypes);
        int availableurlnrs[numavailableipservices];
        get_urlnrs_ipservices(db, availableurlnrs, 0, allowedprotocoltypes);
        uint *seed = (uint *) malloc(SEED_LENGTH * sizeof(uint));
        int resreadrnd = fread(seed, sizeof(uint), SEED_LENGTH, rand_fd);
        if (resreadrnd <= 0) {
                if (!silentmode) {
                        print_dt_error("Warning: Could not read from /dev/urandom.\n");
                }
        }

        srand(*seed);
        fclose(rand_fd);
        if (numavailableipservices <= 0) {
                if (!silentmode) {
                        print_dt_error("Error: no more ipservices available.\n");
                }

                exit(EXIT_FAILURE);
        }

        // Choose a new random position in availableurlnrs. Between 0 and numavailableipservices.
        int p = (rand() % numavailableipservices);
        int urlnr = availableurlnrs[p];
        if (numavailableipservices > 0 && !needaddlasturl)
        {
                int tries = 0;
                while (urlnr == lasturlnr && tries < MAXCHOOSERETRIES)
                {
                        p = (rand() % numavailableipservices);
                        urlnr = availableurlnrs[p];
                        ++tries;
                        int priority = get_priority_ipservice(db, urlnr);
                        if (priority > 1 && priority <= MAXPRIORITY) {
                                if (rand() % priority != 0) {
                                        if (verbosemode) {
                                                printf("Choose different number again.\n");
                                        }

                                        p = (rand() % numavailableipservices);
                                        urlnr = availableurlnrs[p];
                                }
                        }
                }

                if (urlnr == lasturlnr && tries == MAXCHOOSERETRIES && !silentmode) {
                        print_dt_error("Maximum number of retries to choice different urlnr has been reached.\n\
 Could not avoid to use same urlnr as in last run.\n");
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
 * Strip everything in the character array from the new line character till the end
 * of the array. So str does not contain any new line character anymore.
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
 * Read a text file with ip address.
 * @return A character array with the ip address without a newline character.
 */
char * read_file_ipaddr(char *filepathip, bool silentmode)
{
        size_t len = 0;
        ssize_t bytesread;
        FILE *fpfileip;
        fpfileip = fopen(filepathip, "r");
        if (fpfileip == NULL) {
                if (!silentmode) {
                        print_dt_error("Error: Could not get public ip address from file.\n");
                }

                exit(EXIT_FAILURE);
        }

        char *ipaddrstr;
        bytesread = getline(&ipaddrstr, &len, fpfileip);
        // A hex text full out written IPv6 address is 34 characters +1 for null character.
        if (bytesread > IPV6_TEXT_LENGTH) {
                fclose(fpfileip);
                if (!silentmode) {
                        print_dt_error("Error: response server too long.\n");
                }

                exit(EXIT_FAILURE);
        }

        fclose(fpfileip);
        strip_on_newlinechar(ipaddrstr, bytesread);
        return ipaddrstr;
}

/**
 * Parse http status code.
 */
void parse_httpcode_status(int httpcode, sqlite3 *db, int urlnr)
{
        char errmsg[128];
        int cw;
        switch (httpcode) {
        case 420L:
        case 429L:
        case 503L:
        case 509L:
                print_dt_error("Warning: rate limiting active.\
 Avoid the current public ip address service for some time.");
                // Temporary disable
                update_disabled_ipsevice(db, urlnr, true);
                exit(EXIT_FAILURE);
        case 408L:
        case 500L:
        case 502L:
        case 504L:
                cw = snprintf(errmsg,
                              128,
                              "Warning: used public ip address service has an error or other issue (http error: %d).\n",
                              httpcode);
                if (cw >= 0 && cw <= 128) {
                        print_dt_error(errmsg);
                }

                // Temporary disable
                update_disabled_ipsevice(db, urlnr, true);
                break;
        case 401L:
        case 403L:
        case 404L:
        case 410L:
                print_dt_error("Warning: the used public ip address service has quit or does not\
 want automatic use.\nNever use this public ip address service again.\n");
                // Disable forever
                update_disabled_ipsevice(db, urlnr, false);
                break;
        case 301L:
        case 302L:
        case 308L:
                cw = snprintf(errmsg,
                              128,
                              "Warning: public IP address service has changed url and is redirecting\
 (http status code: %d).\n",
                              httpcode);
                if (cw >= 0 && cw <= 128) {
                        print_dt_error(errmsg);
                }

                // Disable forever
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
                        print_dt_error("Error: should not setup cUrl session.\n");
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
			char curlErr[1024];
			int cw;
			snprintf(curlErr,
                                 1024,
                                 "Error: %s\n",
				 curl_easy_strerror(res));
                        print_dt_error(curlErr);
                }

                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                switch (res) {
                case CURLE_TOO_MANY_REDIRECTS:
                case CURLE_REMOTE_ACCESS_DENIED:
                case CURLE_URL_MALFORMAT:
                case CURLE_REMOTE_FILE_NOT_FOUND:
                case CURLE_SSL_CACERT:
                        update_disabled_ipsevice(db, urlnr, true);
                        break;
                default:
                        // CURLE_OPERATION_TIMEDOUT, CURLE_GOT_NOTHING, CURLE_READ_ERROR, CURLE_RECV_ERROR,
                        // CURLE_COULDNT_CONNECT, CURLE_COULDNT_RESOLVE_HOST, CURLE_COULDNT_RESOLVE_PROXY,
                        // CURLE_WRITE_ERROR
                        update_disabled_ipsevice(db, urlnr, true);
                        break;
                }

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

        // Cleanup curl.
        curl_easy_cleanup(curlsession);

        // Set HTTP status code.
        *httpcodestatus = httpcode;

        // Check filesize
        if (downloadedfilesize == 0) {
                if (!silentmode) {
			char emptyFileErr[128];
			int cw;
			cw = snprintf(emptyFileErr,
                                      128,
                                      "Error: downloaded file is empty(urlnr = %d).\n",
				      urlnr);
			print_dt_error(emptyFileErr);
                }

                // Temporary disable
                update_disabled_ipsevice(db, urlnr, true);
                exit(EXIT_FAILURE);
        } else if (downloadedfilesize > MAXSIZEIPADDRDOWNLOAD) {
                // Did not return only an ip address.
                if (!silentmode) {
			char responseTooBigErr[128];
			int cw;
			cw = snprintf(responseTooBigErr,
                                      128,
				      "Error: response ip service(urlnr = %d) too big.\n",
				      urlnr);
			print_dt_error(responseTooBigErr);
                }

                // Temporary disable
                update_disabled_ipsevice(db, urlnr, true);
                exit(EXIT_FAILURE);
        }

        ipaddr = read_file_ipaddr(downloadtempfilepath, silentmode);
        return ipaddr;
}


/**
 * Check if argumentval is a integer and return it, if not output error and exit the program.
 */
int read_commandline_argument_int_value(char * argumentval, bool silentmode)
{
        // Check if argv[n] valid is integer
        int lenargumentval = strlen(argumentval);
        for (int i = 0; i < lenargumentval; ++i) {
                if (!isdigit(argumentval[i])) {
                        if (!silentmode) {
                                print_dt_error("Error: invalid argument number value.\n");
                        }

                        exit(EXIT_FAILURE);
                }
        }

        return atoi(argumentval);
}

/**
 * Parse the command line arguments to the settings struct.
 */
struct Settings parse_commandline_args(int argc, char **argv, struct Settings settings)
{
        bool argnumdelaysec = false;
        bool argnumerrorwait = false;
        bool argposthook = false;
        // Parse command-line arguments and set settings struct.
        for (int n = 1; n < argc; ++n) {
                if (argnumdelaysec) {
                        argnumdelaysec = false;
                        settings.secondsdelay = read_commandline_argument_int_value(argv[n], settings.silentmode);
                        continue;
                } else if (argnumerrorwait) {
                        argnumerrorwait = false;
                        settings.errorwait = read_commandline_argument_int_value(argv[n], settings.silentmode);
                        continue;
                } else if (argposthook) {
                        argposthook = false;
                        // Check length posthook
                        if (strlen(argv[n]) > MAXLENPATHPOSTHOOK) {
                                if (!settings.silentmode) {
                                        print_dt_error("Error: posthook command too long.\n");
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
                } else if (strcmp(argv[n], "--errorwait") == 0) {
                        argnumerrorwait = true;
                } else if (strcmp(argv[n], "--retryposthook") == 0) {
                        settings.retryposthook = true;
                } else if (strcmp(argv[n], "--showip") == 0) {
                        settings.showip = true;
                } else if (strcmp(argv[n], "--showlastrun") == 0) {
                        settings.showlastrun = true;
                } else if (strcmp(argv[n], "--nosavelastrun") == 0) {
                        settings.savelastrun = false;
                } else if (strcmp(argv[n], "--unsafedns") == 0) {
                        settings.unsafedns = true;
                } else if (strcmp(argv[n], "--unsafehttp") == 0) {
                        settings.unsafehttp = true;
                } else if (strcmp(argv[n], "--tripleconfirm") == 0) {
                        settings.tripleconfirm = true;
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
                        printf("--showlastrun   Show the last date and time %s has been runnend\
 and directly exit.\n", PROGRAMNAME);
                        printf("--nosavelastrun Don't save the date and time of current run.\n");
                        printf("--unsafehttp    Allow the use of http public ip services, no TLS/SSL.\n");
                        printf("--delay 1-59    Delay the execution of this program with X number of seconds.\n");
                        printf("--errorwait n   The number of seconds that an ipservice that causes a temporary error\n");
                        printf("                has to wait before it being able to be used again.\n");
                        int errorwaithours = settings.errorwait / 3600;
                        printf("                By default %d seconds (%d hours).\n", settings.errorwait, errorwaithours);
                        printf("--failsilent    Fail silently do not print issues to stderr.\n");
                        printf("--tripleconfirm Confirm ip address change with a additional third ip service.\n");
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
        settings.errorwait = 14400;  // 4 hours
        settings.retryposthook = false;
        settings.unsafehttp = false;
        settings.unsafedns = false;
        settings.tripleconfirm = false;
        settings.showip = false;
        settings.silentmode = false;
        settings.verbosemode = false;
        settings.showlastrun = false;
        settings.savelastrun = true;
        settings = parse_commandline_args(argc, argv, settings);

        if (settings.secondsdelay > 0 && settings.secondsdelay < 60) {
                if (settings.verbosemode) {
                        printf("Delay %d seconds.\n", settings.secondsdelay);
                }

                sleep(settings.secondsdelay);
        } else if (settings.secondsdelay >= MAXNUMSECDELAY && !settings.silentmode) {
                char errormsg[128];
                int cw;
                cw = snprintf(errormsg,
                              128,
                              "Ignoring secondsdelay. secondsdelay has to be less than %d seconds.\n",
                              MAXNUMSECDELAY);
                if (cw >= 0 && cw <= 128) {
                        print_dt_error(errormsg);
                }
        }

        bool dbsetup = false;
        if (access(DATABASEFILENAME, F_OK) == -1 ) {
                dbsetup = true;
                if (settings.verbosemode) {
                        printf("%s does not exists. Creating %s.\n", DATABASEFILENAME, DATABASEFILENAME);
                }
        }

        /* Setup database connection */
        sqlite3 *db;
        int rc;
        rc = sqlite3_open(DATABASEFILENAME, &db);
        if (rc) {
                print_dt_error("Can't open database file.\n");
                exit(EXIT_FAILURE);
        } else if (settings.verbosemode) {
                printf("Opened database successfully.\n");
        }

        if (dbsetup) {
                create_table_ipservice(db, settings.verbosemode);
                create_table_config(db, settings.verbosemode);
                add_config_value_int(db, "lasturlnr", -2, settings.verbosemode);
                /* added default ipservice records */
                add_ipservice(db, 0, "https://ipinfo.io/ip", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 1, "https://api.ipify.org/?format=text", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 2, "https://wtfismyip.com/text", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 3, "https://v4.ident.me/", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 4, "https://ipv4.icanhazip.com/", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 5, "https://checkip.amazonaws.com/", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 6, "https://bot.whatismyipaddress.com/", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 7, "https://secure.informaction.com/ipecho/", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 8, "https://l2.io/ip", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 9, "https://www.trackip.net/ip", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 10, "https://ip4.seeip.org/", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 11, "https://locate.now.sh/ip", false, PROTOCOLHTTPS, 2, settings.verbosemode);
                add_ipservice(db, 12, "https://tnx.nl/ip", false, PROTOCOLHTTPS, 3, settings.verbosemode);
                add_ipservice(db, 13, "https://diagnostic.opendns.com/myip", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 14, "https://ip4.seeip.org/", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 15, "http://myip.dnsomatic.com/", false, PROTOCOLHTTP, 1, settings.verbosemode);
                add_ipservice(db, 16, "http://whatismyip.akamai.com/", false, PROTOCOLHTTP, 1, settings.verbosemode);
                add_ipservice(db, 17, "http://myexternalip.com/raw", false, PROTOCOLHTTP, 1, settings.verbosemode);
                add_ipservice(db, 18, "https://ipecho.net/plain", false, PROTOCOLHTTPS, 1, settings.verbosemode);
                add_ipservice(db, 19, "http://plain-text-ip.com/", false, PROTOCOLHTTP, 2, settings.verbosemode);
        }

        if (settings.showlastrun) {
                char *lastrundt;
                lastrundt = get_config_value_str(db, CONFIGNAMELASTRUNDT);
                if (settings.verbosemode) {
                        printf("Last run on: ");
                }

                printf("%s\n", lastrundt);
                exit(EXIT_SUCCESS);
        }

        int  num_all_urls = get_count_all_ipservices(db);
        if (num_all_urls < 2) {
                if (!settings.silentmode) {
                        print_dt_error("No enough ipservices added to the database. Need at least 2 ipservices.\n");
                        exit(EXIT_FAILURE);
                }
        }

        int urlnr = get_new_random_urlnr(db, settings.unsafehttp, settings.unsafedns,
                                         settings.verbosemode, settings.silentmode);
        const char *urlipservice;
        urlipservice = get_url_ipservice(db, urlnr);
        if (settings.verbosemode) {
                printf("Using %s for getting public IPv4 address.\n", urlipservice);
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
                        print_error_with_url("Error: invalid IPv4 address from '%s'.\n",
                                             urlipservice);
                }

                exit(EXIT_FAILURE);
        }

        char *ipaddrconfirm;
        if (is_config_exists(db, CONFIGNAMEPREVIP) == true) {
                ipaddrconfirm = get_config_value_str(db, CONFIGNAMEPREVIP);
        } else {
                if (settings.verbosemode) {
                        printf("First run of %s.\n", PROGRAMNAME);
                }

                const char *confirmurl;
                urlnr = get_new_random_urlnr(db, settings.unsafehttp, settings.unsafedns,
                                             settings.verbosemode, settings.silentmode);
                confirmurl = get_url_ipservice(db, urlnr);
                if (settings.verbosemode) {
                        printf("Using %s to confirm current public IPv4 address with.\n", confirmurl);
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
                        if (!settings.silentmode) {
                                print_error_with_url("Error: invalid IPv4 address for first run confirmation from %s.\n",
                                                     confirmurl);
                        }

                        exit(EXIT_FAILURE);
                }

                // Check for ip address difference between the two requested services on first run.
                if (strcmp(ipaddrnow, ipaddrconfirm) != 0) {
                        if (!settings.silentmode) {
                                print_detected_difference(ipaddrnow, ipaddrconfirm, urlipservice, confirmurl);
                                printf("Try getting current public IPv4 address again on next run.\n");
                        }

                        exit(EXIT_FAILURE);
                }
        }

        if (settings.savelastrun) {
                char * lastrundt = malloc(20 * sizeof(char));
                lastrundt = get_current_time_str(lastrundt);
                if (is_config_exists(db, CONFIGNAMELASTRUNDT)) {
                        update_config_value_str(db, CONFIGNAMELASTRUNDT, lastrundt, settings.verbosemode);
                } else {
                        add_config_value_str(db, CONFIGNAMELASTRUNDT, lastrundt, settings.verbosemode);
                }
        }

        if (strcmp(ipaddrnow, ipaddrconfirm) != 0) {
                // Ip address has changed.
                if (settings.verbosemode) {
                        printf("Public ip change detected, IPv4 address different from last\
 run.\n");
                }

                int additionalconfirmruns = 1;
                if (settings.tripleconfirm) {
                        additionalconfirmruns = 2;
                }

                for (int i = 0; i < additionalconfirmruns; ++i) {
                        urlnr = get_new_random_urlnr(db,
                                                     settings.unsafehttp,
                                                     settings.unsafedns,
                                                     settings.verbosemode,
                                                     settings.silentmode);
                        const char *confirmchangeurl;
                        confirmchangeurl = get_url_ipservice(db, urlnr);
                        if (settings.verbosemode) {
                                printf("Ipservice %s is used to confirm public IPv4 address.\n", confirmchangeurl);
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
                                if (!settings.silentmode) {
                                        print_error_with_url("Error: invalid IP(IPv4) address returned from confirm ipservice: %s\n",
                                                             confirmchangeurl);
                                }

                                if (settings.showip) {
                                        // Show old valid ip address.
                                        printf("%s", ipaddrconfirm);
                                }

                                exit(EXIT_FAILURE);
                        }

                        // Check if new ip address with different service is the same new ip address.
                        if (strcmp(ipaddrnow, ipaddrconfirmchange) != 0) {
                                if (!settings.silentmode) {
                                        print_detected_difference(ipaddrnow,
                                                                  ipaddrconfirmchange,
                                                                  urlipservice,
                                                                  confirmchangeurl);
                                        print_dt_error("It's now unknown if public ip address has actually changed.\n");
                                }

                                if (settings.showip) {
                                        // Show old ip address.
                                        printf("%s", ipaddrconfirm);
                                }

                                exit(EXIT_FAILURE);
                        }
                }

                if (settings.verbosemode) {
                        printf("Execute posthook command with \"%s\" as argument.\n", ipaddrnow);
                }

                if (settings.argnposthook <= 1) {
                        if (!settings.silentmode) {
                                print_dt_error("Error: no posthook provided.\n");
                                if (settings.showip) {
                                        // Show current ip address.
                                        printf("%s", ipaddrnow);
                                }
                        }

                        exit(EXIT_FAILURE);
                }

                char cmdposthook[MAXLENPATHPOSTHOOK + 1];
                if (strchr(ipaddrnow, '"') != NULL) {
                        if (!settings.silentmode) {
                                print_dt_error("Error: quote(s) in ip address.\n");
                        }

                        exit(EXIT_FAILURE);
                } else if (strchr(argv[settings.argnposthook], '"') != NULL) {
                        if (!settings.silentmode) {
                                print_dt_error("Error: quote in posthook command.\n");
                        }

                        exit(EXIT_FAILURE);
                }

                snprintf(cmdposthook, MAXLENPATHPOSTHOOK, "\"%s\" \"%s\"",
                         argv[settings.argnposthook], ipaddrnow);
                /* printf("cmdposthook = [ %s ]\n", cmdposthook); // DEBUG */
                unsigned short exitcode = system(cmdposthook);
                if (exitcode != 0) {
                        if (!settings.silentmode) {
                                char errormsg[64];
                                int cw;
                                cw = snprintf(errormsg,
                                              64,
                                              "Error: script returned error (exitcode %hu).\n",
                                              exitcode);
                                if (cw >= 0 && cw <= 64) {
                                        print_dt_error(errormsg);
                                }
                        }

                        if (settings.retryposthook) {
                                if (settings.showip) {
                                        // Do show new ip address.
                                        printf("%s", ipaddrnow);
                                }

                                // Run posthook next time again because CONFIGNAMEPREVIP is
                                // not updated to new ip in database.
                                exit(EXIT_FAILURE);
                        }
                }

                if (is_config_exists(db, CONFIGNAMEPREVIP) == true) {
                        update_config_value_str(db, CONFIGNAMEPREVIP, ipaddrnow, settings.verbosemode);
                } else {
                        add_config_value_str(db, CONFIGNAMEPREVIP, ipaddrnow, settings.verbosemode);
                }
        } else if (settings.verbosemode) {
                printf("The current public ip is the same as the public ip from last ipservice.\n");
                if (is_config_exists(db, CONFIGNAMEPREVIP) == false) {
                        // Readded missing CONFIGNAMEPREVIP config value.
                        add_config_value_str(db, CONFIGNAMEPREVIP, ipaddrnow, settings.verbosemode);
                }
        }

        if (settings.showip) {
                // Show current ip address.
                printf("%s", ipaddrnow);
        }

        reenable_expired_disabled_ipservices(db, settings.errorwait);
        return EXIT_SUCCESS;
}

