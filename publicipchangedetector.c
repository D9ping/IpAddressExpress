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
#include <curl/curl.h>
#include <arpa/inet.h>

#define PROGRAMNAME        "PublicIpChangeDetector"
#define PROGRAMVERSION     "0.4.5"
#define PROGRAMWEBSITE     " (+https://github.com/D9ping/PublicIpChangeDetector)"
#define SEED_LENGTH        32
#define IPV6_TEXT_LENGTH   34
#define MAXLENPATHPOSTHOOK 1023
#define MAXSIZEAVOIDURLNRS 4096
typedef unsigned int       uint;

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
    Is an number with a number array.
    @return 1 if it's within the array and 0 if the number not in the array.
            And return 2 if array is filled with all maxurls urlnr's.
*/
int is_num_within_numarr(int numarr[], int maxurls, int needlenum)
{
        for (int i = 0; i < maxurls; ++i) {
                if (numarr[i] == -1) {
                        break;
                } else {
                        if (i == maxurls) {
                                return 2;
                        }
                }

                if (numarr[i] == needlenum) {
                        return 1;
                }
        }

        return 0;
}


/**
    Write urlnr to file to avoid on next run.
*/
void write_avoid_urlnr(int urlnr, bool silentmode)
{
        char filepathavoidurlnrs[] = "/tmp/avoidurlnrs.txt";
        FILE *fpavoidurlnrs;
        bool writecomma = true;
        if (access(filepathavoidurlnrs, F_OK) == -1) {
                // filepathavoidurlnrs does not exist.
                writecomma = false;
        }

        fpavoidurlnrs = fopen(filepathavoidurlnrs, "a+");
        if (fpavoidurlnrs == NULL) {
                if (!silentmode) {
                        fprintf(stderr, "Error: Could not open %s.\n", filepathavoidurlnrs);
                }

                return;
        }

        fseek(fpavoidurlnrs, 0L, SEEK_END);
        unsigned long filesizeavoidurlnrs = ftell(fpavoidurlnrs);
        if (filesizeavoidurlnrs > MAXSIZEAVOIDURLNRS) {
                if (!silentmode) {
                        fprintf(stderr, "Error: %s too big.\n", filepathavoidurlnrs);
                }

                return;
        }

        rewind(fpavoidurlnrs);
        int lastcharfirstline = 0;
        int c;
        do {
                c = getc(fpavoidurlnrs);
                if (c == '\n' || c == '\r') {
                        break;
                }

                lastcharfirstline++;
        } while (c != EOF);

        if (filesizeavoidurlnrs == 0L) {
                writecomma = false;
        }

        if (lastcharfirstline != (int)(filesizeavoidurlnrs++)) {
                fprintf(stderr, "First line is shorter than filesize\
 overwriting after first line.\n");
        }

        if (writecomma == true) {
                fprintf(fpavoidurlnrs, "%s%d", ",", urlnr);
        } else {
                fprintf(fpavoidurlnrs, "%d", urlnr);
        }

        fclose(fpavoidurlnrs);
}


/**
    Get a pointer to the array with urlnr to avoid.
 */
void get_avoid_urlnrs(int avoidurlnrs[], int maxurls, bool verbosemode, bool silentmode)
{
        char filepathreadavoidurlnrs[] = "/tmp/avoidurlnrs.txt";
        if (access(filepathreadavoidurlnrs, F_OK) == -1) {
                if (verbosemode) {
                        printf("%s does not exists.\n", filepathreadavoidurlnrs);
                }

                return;
        } else if (verbosemode) {
                printf("The %s file exists.\n", filepathreadavoidurlnrs);
        }

        FILE * fpreadavoidurlnrs;
        fpreadavoidurlnrs = fopen(filepathreadavoidurlnrs, "r");
        if (fpreadavoidurlnrs == NULL) {
                perror("fopen");
                return;
        }

        const char SEPERATOR = ',';
        ssize_t filelength;
        size_t readlen = 0;
        char *lineavoidurlnrs;
        filelength = getline(&lineavoidurlnrs, &readlen, fpreadavoidurlnrs) - 1;  
        fclose(fpreadavoidurlnrs);
        int numurlnrs = 0;
        int n = 0;
        const char nonumberchars[] = "   ";
        char arrnumber[3] = {' ', ' ', ' '};
        for (uint i = 0; i <= filelength; ++i) {
                if ((lineavoidurlnrs[i] == SEPERATOR) || 
                    (i == filelength && lineavoidurlnrs[i] != SEPERATOR)) {
                        n = 0;
                        if (i >= 1 && strcmp(arrnumber, nonumberchars) != 0) {
                                avoidurlnrs[numurlnrs] = atoi(arrnumber);
                                ++numurlnrs;
                                if (numurlnrs >= maxurls) {
                                        if (!silentmode) {
                                                fprintf(stderr, "Maximum number of urlnr\
 numbers to avoid reached.");
                                        }

                                        break;
                                }
                        }

                        arrnumber[0] = ' ';
                        arrnumber[1] = ' ';
                        arrnumber[2] = ' ';
                        continue;
                }

                if (n > 2) {
                        // Currently two digits is the highest urlnr.
                        if (!silentmode) {
                                fprintf(stderr, "Error: number too big.\n");
                        }

                        arrnumber[0] = ' ';
                        arrnumber[1] = ' ';
                        arrnumber[2] = ' ';
                        continue;
                }

                if (!isdigit(lineavoidurlnrs[i])) {
                        if (!silentmode) {
                                fprintf(stderr,
                                        "Error '%c' character is not a number.\n",
                                        lineavoidurlnrs[i]);
                        }

                        continue;
                }

                arrnumber[n] = lineavoidurlnrs[i];
                ++n;
        }
}


/**
    Generate a secure random number between 0 and maxurls.
    Write the random number to disk and do not use the same value next time.
    @return A random number between 0 and maxurls.
*/
int get_new_random_urlnr(int maxurls, bool verbosemode, bool silentmode)
{
        if (verbosemode) {
                printf("Choice random urlnr.\n");
        }

        if (maxurls < 2) {
                if (!silentmode) {
                        fprintf(stderr, "Error: maxurls needs to be 2 or higher.\n");
                }

                exit(EXIT_FAILURE);
        }

        char filepathlasturlnr[] = "/tmp/lasturlnr.txt";
        char *strlasturlnr;
        FILE * lasturlnr_fd;
        int lasturlnr = -1;
        if (access(filepathlasturlnr, F_OK) != -1) {
                ssize_t bytesread;
                size_t len = 0;
                lasturlnr_fd = fopen(filepathlasturlnr, "r");
                bytesread = getline(&strlasturlnr, &len, lasturlnr_fd);
                fclose(lasturlnr_fd);
                lasturlnr = atoi(strlasturlnr);
                if (verbosemode) {
                        printf("read lasturlnr = %d\n", lasturlnr);
                }
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

        int avoidurlnrs[16] = {-1}; // avoidurlnrs[maxurls]
        if (verbosemode) {
                printf("Get array with urlnumbers to avoid.\n");
        }

        get_avoid_urlnrs(avoidurlnrs, maxurls, verbosemode, silentmode);
        // TODO: construct: availableurlnrs
        uint *seed = (uint *) malloc(SEED_LENGTH * sizeof(uint));
        int resreadrnd = fread(seed, sizeof(uint), SEED_LENGTH, rand_fd);
        srand(*seed);
        fclose(rand_fd);
        // Choose a new random urlnr between 0 and maxurls.
        int urlnr = (rand() % maxurls);
        uint tries = 0;
        uint maxtries = maxurls * 2;

        int res = is_num_within_numarr(avoidurlnrs, maxurls, urlnr);
        if (res == 2) {
                if (!silentmode) {
                        fprintf(stderr, "Error: avoided all possible public ip url's to use.\n\
 There is no url to use.\n");
                }

                exit(EXIT_FAILURE);
        }

        while (urlnr == lasturlnr || res == 1) {
                urlnr = (rand() % maxurls);
                ++tries;
                if (tries > maxtries) {
                        exit(EXIT_FAILURE);
                }

                res = is_num_within_numarr(avoidurlnrs, maxurls, urlnr);
                if (verbosemode) {
                        if (res != 1 && urlnr != lasturlnr) {
                                printf("Found new urlnr = %d.\n", urlnr);
                        }
                }
        }

        // write urlnr
        lasturlnr_fd = fopen(filepathlasturlnr, "w+");
        fprintf(lasturlnr_fd, "%d\n", urlnr);
        fclose(lasturlnr_fd);
        if (verbosemode) {
                printf("new urlnr = %d\n", urlnr);
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
                        fprintf(stderr, "Error: Could not get public ip now. Was not written.\n");
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
    Get a random url of one of the public ip http or https service. 
    But not the same url as the last random url.
    @return An random url to get the public ip address from.
*/
char * get_url_ipservice(const int urlnr, bool silentmode)
{
        /* printf("urlnr = %d\n", urlnr); // DEBUG */
        // Allocate url memory dynamically.
        char *url = NULL;
        url = malloc(40 * sizeof(char)); // strlen("https://secure.informaction.com/ipecho/") => 39
        if (url == NULL) {
                if (!silentmode) {
                        fprintf(stderr, "Error: unable to allocate required memory.\n");
                }

                exit(EXIT_FAILURE);
        }

        switch (urlnr) {
        // https services:
        case 0:
                strcpy(url, "https://ipinfo.io/ip");
                break;
        case 1:
                strcpy(url, "https://api.ipify.org/?format=text");
                break;
        case 2:
                strcpy(url, "https://wtfismyip.com/text");
                break;
        case 3:
                strcpy(url, "https://v4.ident.me/");
                break;
        case 4:
                strcpy(url, "https://ipv4.icanhazip.com/");
                break;
        case 5:
                strcpy(url, "https://checkip.amazonaws.com/");
                break;
        case 6:
                strcpy(url, "https://bot.whatismyipaddress.com/");
                break;
        case 7:
                strcpy(url, "https://secure.informaction.com/ipecho/");
                break;
        case 8:
                strcpy(url, "https://l2.io/ip");
                break;
        case 9:
                strcpy(url, "https://www.trackip.net/ip");
                break;
        case 10:
                strcpy(url, "https://ip4.seeip.org/");
                break;
        // http services:
        case 11:
                strcpy(url, "http://myip.dnsomatic.com/");
                break;
        case 12:
                strcpy(url, "http://whatismyip.akamai.com/");
                break;
        case 13:
                strcpy(url, "http://myexternalip.com/raw");
                break;
        case 14:
                strcpy(url, "http://ipecho.net/plain");
                break;
        case 15:
                strcpy(url, "http://plain-text-ip.com");
                break;
        default:
                if (!silentmode) {
                        fprintf(stderr, "Error: unknown urlnr.\n");
                }

                exit(EXIT_FAILURE);
        }

        return url;
}


/**
    Download all content from a url 
*/
void download_file(char *url, int urlnr, char *filepathnewdownload, bool unsafehttp, bool silentmode)
{
        FILE *fpdownload;
        // mode w+: truncates the file to zero length if it exists,
        // otherwise creates a file if it does not exist.
        fpdownload = fopen(filepathnewdownload, "w+");
        curl_global_init(CURL_GLOBAL_DEFAULT);
        CURL *curlsession;
        curlsession = curl_easy_init();
        if (!curlsession || curlsession == NULL) {
                if (!silentmode) {
                        fprintf(stderr, "Error: should not setup curl session.\n");
                }

                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                exit(EXIT_FAILURE);
        }

        curl_easy_setopt(curlsession, CURLOPT_URL, url);
        /* Use a GET to fetch this */
        curl_easy_setopt(curlsession, CURLOPT_HTTPGET, 1L);
        /* Write to fpdownload */
        curl_easy_setopt(curlsession, CURLOPT_WRITEDATA, fpdownload);
        /* Default 300s, changed to max. 20 seconds to connect */
        curl_easy_setopt(curlsession, CURLOPT_CONNECTTIMEOUT, 20L);
        /* Default timeout is 0/never. changed to 25 seconds */
        curl_easy_setopt(curlsession, CURLOPT_TIMEOUT, 25L);
        /* Never follow redirects. */
        curl_easy_setopt(curlsession, CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curlsession, CURLOPT_MAXREDIRS, 0L);
        /* Resolve host name using IPv4-names only */
        curl_easy_setopt(curlsession, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        if (!unsafehttp) {
                // Only allow https to be used.
                curl_easy_setopt(curlsession, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
        } else {
                // Allow https and unsafe http.
                curl_easy_setopt(curlsession, CURLOPT_PROTOCOLS, 
                                 CURLPROTO_HTTPS | CURLPROTO_HTTP);
        }

        char useragent[100];
        strcpy(useragent, PROGRAMNAME);
        strcat(useragent, "/");
        strcat(useragent, PROGRAMVERSION);
        strcat(useragent, PROGRAMWEBSITE);
        curl_easy_setopt(curlsession, CURLOPT_USERAGENT, useragent);
        /* Perform the request, res will get the return code */
        CURLcode res;
        res = curl_easy_perform(curlsession);
        /* Check for errors */
        if (res != CURLE_OK) {
                if (!silentmode) {
                        fprintf(stderr, "Error: %s, url: %s\n", curl_easy_strerror(res), url);
                }

                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                write_avoid_urlnr(urlnr, silentmode);
                exit(EXIT_FAILURE);
        }

        long http_code = 0;
        curl_easy_getinfo(curlsession, CURLINFO_RESPONSE_CODE, &http_code);

        // Get HTTP status code.
        /* printf("Got HTTP status code: %ld.\n", http_code); // DEBUG */
        switch (http_code) {
        case 420L:
        case 429L:
        case 503L:
        case 509L:
                printf("Warn: rate limiting active.\
 Avoid the current public ip address service for some time.");
                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                // FIXME should not be avoid forever, but for some time only.
                write_avoid_urlnr(urlnr, silentmode);
                exit(EXIT_FAILURE);
        case 408L:
        case 500L:
        case 502L:
        case 504L:
                printf("Warn: used public ip address service has an error or other issue\
 (http error: %ld).\n", http_code);
                // FIXME should not be avoid forever, but for some time only.
                write_avoid_urlnr(urlnr, silentmode);
                break;
        case 401L:
        case 403L:
        case 404L:
        case 410L:
                printf("Warn: the used public ip address service has quit or does not\
 want automatic use.\nNever use this public ip service again.\n");
                write_avoid_urlnr(urlnr, silentmode);
                break;
        case 301L:
        case 302L:
        case 308L:
                printf("Warn: public IP address service has changed url and is redirecting\
 (http status code: %ld).\n", http_code);
                printf("Incorrect url: %s\n", url);
                char *followurl = NULL;
                curl_easy_getinfo(curlsession, CURLINFO_REDIRECT_URL, &followurl);
                if (followurl) {
                        if (strcmp(followurl, url) != 0) {
                                printf("Wants to redirect to: %s\n", followurl);
                        } else {
                                printf("Redirect to same page.\n");
                        }
                }

                write_avoid_urlnr(urlnr, silentmode);
                break;
        }

        // Cleanup, should always happen.
        curl_easy_cleanup(curlsession);
        // Check the length of the downloaded file.
        fseek(fpdownload, 0L, SEEK_END);
        unsigned long long int downloadedfilesize = ftell(fpdownload);
        /* printf("Downloaded file filesize: %llu bytes.\n", downloadedfilesize); // DEBUG */
        fclose(fpdownload);
        if (downloadedfilesize == 0) {
                if (!silentmode) {
                        fprintf(stderr, "Error: downloaded file is empty.\n");
                }

                exit(EXIT_FAILURE);
        }
}


int main(int argc, char **argv)
{
        int argnposthook = -1;
        bool retryposthook = false;
        int secondsdelay = 0;
        bool silentmode = false;
        bool verbosemode = false;
        bool argnumdelaysec = false;
        bool argposthook = false;
        bool showip = false;
        bool unsafehttp = false;
        for (int n = 1; n < argc; ++n) {
                if (argnumdelaysec) {
                        argnumdelaysec = false;
                        // Check if argv[n] valid is integer
                        int lenarg = strlen(argv[n]);
                        for (int i = 0; i < lenarg; ++i) {
                                if (!isdigit(argv[n][i])) {
                                        if (!silentmode) {
                                                fprintf(stderr, 
                                                        "Error: invalid number of seconds delay.\n");
                                        }

                                        exit(EXIT_FAILURE);
                                }
                        }

                        secondsdelay = atoi(argv[n]);
                        continue;
                } else if (argposthook) {
                        argposthook = false;
                        // Check length posthook
                        if (strlen(argv[n]) > MAXLENPATHPOSTHOOK) {
                                if (!silentmode) {
                                        fprintf(stderr, "Error: posthook command too long.\n");
                                }

                                exit(EXIT_FAILURE);
                        }

                        // For now, if multiple posthook's are provided override the command with last posthook command.
                        argnposthook = n;
                        continue;
                }

                if (strcmp(argv[n], "--posthook") == 0) {
                        argposthook = true;
                } else if (strcmp(argv[n], "--retryposthook") == 0) {
                        retryposthook = true;
                } else if (strcmp(argv[n], "--showip") == 0) {
                        showip = true;
                } else if (strcmp(argv[n], "--unsafehttp") == 0) {
                        unsafehttp = true;
                } else if (strcmp(argv[n], "--delay") == 0) {
                        argnumdelaysec = true;
                } else if (strcmp(argv[n], "--failsilent") == 0) {
                        silentmode = true;
                } else if (strcmp(argv[n], "--version") == 0) {
                        printf("%s %s\n", PROGRAMNAME, PROGRAMVERSION);
                        exit(EXIT_SUCCESS);
                } else if (strcmp(argv[n], "-v") == 0 || strcmp(argv[n], "--verbose") == 0) {
                        verbosemode = true;
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
                } else if (!silentmode) {
                        fprintf(stderr, "Ignore unknown argument \"%s\".\n", argv[n]);
                }
        }

        if (secondsdelay > 0 && secondsdelay < 60) {
                if (verbosemode) {
                        printf("Delay %d seconds.\n", secondsdelay);
                }

                sleep(secondsdelay);
        }

        char filepathipnow[] = "/tmp/ipnow.txt";
        char filepathipwas[] = "/tmp/ipwas.txt";
        int maxurls = 11;
        if (unsafehttp) {
                maxurls = 16;
        }

        int urlnr = get_new_random_urlnr(maxurls, verbosemode, silentmode);
        char *url;
        url = get_url_ipservice(urlnr, silentmode);
        if (verbosemode) {
                printf("Use: %s for getting public IPv4 address.\n", url);
        }

        download_file(url, urlnr, filepathipnow, unsafehttp, silentmode);
        if (verbosemode) {
                printf("Download finished.\n");
        }

        char *ipaddrstr;
        ipaddrstr = read_file_ipaddr(filepathipnow, silentmode);
        if (is_valid_ipv4_addr(ipaddrstr) != 1) {
                free(ipaddrstr);
                if (!silentmode) {
                        fprintf(stderr, "Error: got invalid IP(IPv4) address from %s.\n", url);
                }

                unlink(filepathipnow);
                exit(EXIT_FAILURE);
        }

        char *previpaddrstr;
        if (access(filepathipwas, F_OK) == -1 ) {
                // PublicIpChangeDetector has not been runned before.
                char *confirmurl;
                urlnr = get_new_random_urlnr(maxurls, verbosemode, silentmode);
                confirmurl = get_url_ipservice(urlnr, silentmode);
                if (verbosemode) {
                        printf("First run. Confirm current public ip result.\n");
                        printf("Use: %s to confirm public ip address.\n", confirmurl);
                }

                download_file(confirmurl, urlnr, filepathipnow, unsafehttp, silentmode);
                previpaddrstr = read_file_ipaddr(filepathipnow, silentmode);
                if (is_valid_ipv4_addr(previpaddrstr) != 1) {
                        free(previpaddrstr);
                        unlink(filepathipnow);
                        if (!silentmode) {
                                fprintf(stderr, "Error: invalid IP(IPv4) address for first\
 run confirmation from %s.\n", confirmurl);
                        }

                        exit(EXIT_FAILURE);
                }

                if (strcmp(ipaddrstr, previpaddrstr) != 0) {
                        if (!silentmode) {
                                fprintf(stderr, "Alert: one of the ip address services\
 couldhave lied to us.\nTry getting public ip again on next run.\n");
                                fprintf(stderr, "IPv4: %s from %s.\n", ipaddrstr, url);
                                fprintf(stderr, "IPv4: %s from %s.\n", previpaddrstr,
                                        confirmurl);
                        }

                        exit(EXIT_FAILURE);
                }
        } else {
                previpaddrstr = read_file_ipaddr(filepathipwas, silentmode);
        }

        if (strcmp(ipaddrstr, previpaddrstr) != 0) {
                if (verbosemode) {
                        printf("Public ip change detected, ip address different from last\
 run.\n");
                }

                urlnr = get_new_random_urlnr(maxurls, verbosemode, silentmode);
                url = get_url_ipservice(urlnr, silentmode);
                if (verbosemode) {
                        printf("Public ip service %s is used to confirm ip address\n", url);
                }

                download_file(url, urlnr, filepathipnow, unsafehttp, silentmode);
                char *confirmipaddrstr;
                confirmipaddrstr = read_file_ipaddr(filepathipnow, silentmode);
                if (is_valid_ipv4_addr(confirmipaddrstr) != 1) {
                        free(confirmipaddrstr);
                        if (!silentmode) {
                                fprintf(stderr, "Error: invalid IP(IPv4) address returned\
 from %s as confirm public ip service.\n", url);
                        }

                        unlink(filepathipnow);
                        if (showip) {
                                printf("%s", ipaddrstr);
                        }

                        exit(EXIT_FAILURE);
                }

                if (strcmp(confirmipaddrstr, previpaddrstr) == 0) {
                        if (!silentmode) {
                                fprintf(stderr, "Alert: the ip address service could have\
 lied to us.\nIt's now unknown if public ip address has actually changed.\n");
                        }

                        if (showip) {
                                printf("%s", ipaddrstr);
                        }

                        exit(EXIT_FAILURE);
                }

                if (verbosemode) {
                        printf("Execute posthook command with \"%s\" as argument.\n", ipaddrstr);
                }

                if (argnposthook < 1) {
                        //cmdposthook[] = "/bin/sh ./update_ip_dns.sh ";
                        if (!silentmode) {
                                fprintf(stderr, "Error: no posthook provided.\n");
                        }

                        exit(EXIT_FAILURE);
                }

                char cmdposthook[MAXLENPATHPOSTHOOK + 1];
                if (strchr(ipaddrstr, '"') != NULL) {
                        if (!silentmode) {
                                fprintf(stderr, "Error: quote in ip address.\n");
                        }

                        exit(EXIT_FAILURE);
                } else if (strchr(argv[argnposthook], '"') != NULL) {
                        if (!silentmode) {
                                fprintf(stderr, "Error: quote in posthook command.\n");
                        }

                        exit(EXIT_FAILURE);
                }

                snprintf(cmdposthook, MAXLENPATHPOSTHOOK, "\"%s\" \"%s\"", argv[argnposthook], ipaddrstr);
                /* printf("cmdposthook = [ %s ]\n", cmdposthook); // DEBUG */
                unsigned short exitcode = system(cmdposthook);
                if (exitcode != 0) {
                        if (!silentmode) {
                                fprintf(stderr,
                                        "Error: script returned error (exitcode %hu).\n",
                                        exitcode);
                        }

                        if (retryposthook) {
                                if (showip) {
                                        printf("%s", ipaddrstr);
                                }

                                // Run posthook next time again on posthook error
                                exit(EXIT_FAILURE);
                        }
                }
        } else if (verbosemode) {
                printf("Public ip is the same as from last run.\n");
        }

        /* Rename file */
        int ret = rename(filepathipnow, filepathipwas);
        if (ret != 0) {
                fprintf(stderr, "Error: unable to rename %s to %s.\n", filepathipnow, filepathipwas);
                if (showip) {
                        printf("%s", ipaddrstr);
                }

                exit(EXIT_FAILURE);
        }

        if (showip) {
                printf("%s", ipaddrstr);
        }

        return EXIT_SUCCESS;
}

