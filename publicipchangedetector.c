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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <arpa/inet.h>


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
    Generate a secure random number between 0 and maxurls.
    Write the random number to disk and do not use the same value next time.
    @return A random number between 0 and maxurls.
*/
int get_new_random_urlnr(int maxurls)
{
        if (maxurls < 2) {
                fprintf(stderr, "Error: maxurls needs to be 2 or higher.\n");
                exit(EXIT_FAILURE);
        }

        char filepathlasturlnr[] = "/tmp/lasturlnr.txt";
        char *strlasturlnr;
        FILE * lasturlnr_fd;
        int lasturlnr = -1;
        if (access(filepathlasturlnr, F_OK) != -1) {
            // exists
            ssize_t bytesread;
            size_t len = 0;
            lasturlnr_fd = fopen(filepathlasturlnr, "r");
            bytesread = getline(&strlasturlnr, &len, lasturlnr_fd);
            fclose(lasturlnr_fd);
            lasturlnr = atoi(strlasturlnr);
            /* printf("lasturlnr = %d\n", lasturlnr); // DEBUG */
        }

        /* Initializes a random number generator using a seed from /dev/urandom */
        FILE * rand_fd;
        rand_fd = fopen("/dev/urandom", "r");
        if (rand_fd == 0) {
                fprintf(stderr, "Error: could not open /dev/urandom\n");
                fclose(rand_fd);
                exit(EXIT_FAILURE);
        }

        uint *seed = (uint *) malloc(32 * sizeof(uint));
        int resreadrnd = fread(seed, sizeof(uint), 32, rand_fd);
        srand(*seed);
        fclose(rand_fd);
        /* Choose a new random urlnr between 0 and maxurls */
        int urlnr = (rand() % maxurls);
        uint tries = 0;
        const uint MAXTRIESRNDNUM = 2048;
        while (urlnr == lasturlnr) {
            int urlnr = (rand() % maxurls);
            ++tries;
            if (tries > MAXTRIESRNDNUM) {
                exit(EXIT_FAILURE);
            }
        }

        /* write urlnr */
        lasturlnr_fd = fopen(filepathlasturlnr, "w+");
        fprintf(lasturlnr_fd, "%d\n", urlnr);
        fclose(lasturlnr_fd);
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
            /* Added null character here so it's shorter just before first line feed. */
            str[i] = '\0';
            return;
        }
    }
}


/**
    Read a text file with ip address.
    @return A character array with the ip address without a newline character.
*/
char * read_file_ipaddr(char *filepathip)
{
        size_t len = 0;
        ssize_t bytesread;
        FILE *fpfileip;
        fpfileip = fopen(filepathip, "r");
        if (fpfileip == NULL) {
        fclose(fpfileip);
                fprintf(stderr, "Error: Could not get public ip now. Was not written.\n");
                exit(EXIT_FAILURE);
        }

        char *ipaddrstr;
        bytesread = getline(&ipaddrstr, &len, fpfileip);
        // A hex text full out written IPv6 address is 34 characters +1 for null character.
        if (bytesread > 35) {
        fclose(fpfileip);
                fprintf(stderr, "Error: response server too long.\n");
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
char * get_url_ipservice()
{
        int urlnr = get_new_random_urlnr(16);
        /* printf("urlnr = %d\n", urlnr); // DEBUG */

        /* allocate memory dynamically */
        char *url = NULL;
        url = malloc(40 * sizeof(char)); // strlen("https://secure.informaction.com/ipecho/") => 39
        if (url == NULL) {
                fprintf(stderr, "Error: unable to allocate required memory.\n");
                exit(EXIT_FAILURE);
        }

        switch (urlnr) {
        /* https services */
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
                strcpy(url, "https://v4.ifconfig.co/ip");
                break;
        case 4:
                strcpy(url, "https://v4.ident.me/");
                break;
        case 5:
                strcpy(url, "https://ipv4.icanhazip.com/");
                break;
        case 6:
                strcpy(url, "https://checkip.amazonaws.com/");
                break;
        case 7:
                strcpy(url, "https://bot.whatismyipaddress.com/");
                break;
        case 8:
                strcpy(url, "https://secure.informaction.com/ipecho/");
                break;
        case 9:
                strcpy(url, "https://l2.io/ip");
                break;
        case 10:
                strcpy(url, "https://www.trackip.net/ip");
                break;
        /* http services */
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
                strcpy(url, "http://ifconfig.me/ip");
                break;
        default:
                fprintf(stderr, "Error: unknown urlnr.\n");
                exit(EXIT_FAILURE);
                break;
        }

        return url;
}


/**
    Download all content from a url 
*/
void download_file(char *url, char *filepathnewdownload)
{
        FILE *fpdownload;
        // mode w+: truncates the file to zero length if it exists,
        // otherwise creates a file if it does not exist.
        fpdownload = fopen(filepathnewdownload, "w+");
        curl_global_init(CURL_GLOBAL_DEFAULT);
        CURL *curlsession;
        curlsession = curl_easy_init();
        if (!curlsession || curlsession == NULL) {
                fprintf(stderr, "Error: should not setup curl session.\n");
                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                exit(EXIT_FAILURE);
        }

        curl_easy_setopt(curlsession, CURLOPT_URL, url);
        /* use a GET to fetch this */
        curl_easy_setopt(curlsession, CURLOPT_HTTPGET, 1L);
        /* Write to ipfp */
        curl_easy_setopt(curlsession, CURLOPT_WRITEDATA, fpdownload);
        /* default 300s, max. 25 seconds to connect */
        curl_easy_setopt(curlsession, CURLOPT_CONNECTTIMEOUT, 25L);
        /* default timeout is 0/never. change to 25 seconds */
        curl_easy_setopt(curlsession, CURLOPT_TIMEOUT, 25L);
        /* Never follow redirects. */
        curl_easy_setopt(curlsession, CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curlsession, CURLOPT_MAXREDIRS, 0L);
        /* resolve host name using IPv4-names only */
        curl_easy_setopt(curlsession, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        /* Only allow http and https to be used. (default: CURLPROTO_ALL) */
        curl_easy_setopt(curlsession, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
        curl_easy_setopt(curlsession, CURLOPT_USERAGENT, "PublicIpChangeDetector/0.3.1");
        /* Perform the request, res will get the return code */
        CURLcode res;
        res = curl_easy_perform(curlsession);
        /* Check for errors */
        if (res != CURLE_OK) {
                fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                exit(EXIT_FAILURE);
        }

        long http_code = 0;
        curl_easy_getinfo(curlsession, CURLINFO_RESPONSE_CODE, &http_code);

        /* Get HTTP status code. */
        /* printf("Got HTTP status code: %ld.\n", http_code); // DEBUG */
        if (http_code == 429L || http_code == 509L || http_code == 420L) {
                // TODO: If HTTP status code 429 or 420 or 509 is returned then the currently used
                // public ip service should be avoided for some time on next runs.
                printf("Warn: rate limiting active. Use avoid the current public ip address service for some time.");
                curl_easy_cleanup(curlsession);
                fclose(fpdownload);
                exit(EXIT_FAILURE);
        } else if (http_code >= 500L) {
                printf("Warn: used public ip address service has an error.\n");
        } else if (http_code == 410L) {
                printf("Warn: the used public ip address service has quit. Never use this url again.\n");
        } else if (http_code == 304) {
                printf("Info: http Not Modified, our ip address is the same.\n");
        } else if (http_code == 301L || http_code == 302L || http_code == 308L) {
                printf("Warn: public IP address service changed url.\n");
                printf("Incorrect url: %s\n", url);
        }

        /* Cleanup, should always happen. */
        curl_easy_cleanup(curlsession);
        /* Check the length of the downloaded file. */
        fseek(fpdownload, 0L, SEEK_END);
        unsigned long long int downloadedfilesize = ftell(fpdownload);
        /* printf("Downloaded file filesize: %llu bytes.\n", downloadedfilesize); // DEBUG */
        /* Close file pointer */
        fclose(fpdownload);
        if (downloadedfilesize == 0) {
                fprintf(stderr, "Error: downloaded empty file.\n");
                exit(EXIT_FAILURE);
        }
}


int main(void)
{
        char filepathipnow[] = "/tmp/ipnow.txt";
        char filepathipwas[] = "/tmp/ipwas.txt";
        char *url;
        url = get_url_ipservice();
        /* printf("url = %s\n", url); // DEBUG */
        download_file(url, filepathipnow);
        char *ipaddrstr;
        ipaddrstr = read_file_ipaddr(filepathipnow);
        if (is_valid_ipv4_addr(ipaddrstr) != 1) {
                free(ipaddrstr);
                fprintf(stderr, "Error: invalid IP(IPv4) address returned.\n");
                unlink(filepathipnow);
                exit(EXIT_FAILURE);
        }

        char *previpaddrstr;
        if (access(filepathipwas, F_OK) == -1 ) {
                printf("Info: first run. Confirm current ip result with other service.\n");
                sleep(1);
                char *url;
                url = get_url_ipservice();
                download_file(url, filepathipnow);
                previpaddrstr = read_file_ipaddr(filepathipnow);
                if (is_valid_ipv4_addr(previpaddrstr) != 1) {
                        free(previpaddrstr);
                        unlink(filepathipnow);
                        fprintf(stderr, "Error: invalid IP(IPv4) address returned.\n");
                        exit(EXIT_FAILURE);
                }

                if (strcmp(ipaddrstr, previpaddrstr) == 0) {
                        fprintf(stderr, "Alert: one of the ip address services could have lied to us.\n\
 Try getting public ip again on next run.\n");
                        exit(EXIT_FAILURE);
                }
        } else {
                previpaddrstr = read_file_ipaddr(filepathipwas);
        }

        if (strcmp(ipaddrstr, previpaddrstr) != 0) {
                printf("Change detected.\n");
                printf("Ip service original url = %s\n", url);
                sleep(1);
                url = get_url_ipservice();
                printf("Ip service confirm url = %s\n", url);
                download_file(url, filepathipnow);
                char *confirmipaddrstr;
                confirmipaddrstr = read_file_ipaddr(filepathipnow);
                if (is_valid_ipv4_addr(confirmipaddrstr) != 1) {
                        free(confirmipaddrstr);
                        fprintf(stderr, "Error: invalid IP(IPv4) address returned from confirm public ip service.\n");
                        unlink(filepathipnow);
                        exit(EXIT_FAILURE);
                }

                if (strcmp(confirmipaddrstr, previpaddrstr) == 0) {
                        fprintf(stderr, "Alert: the ip address service could have lied to us.\n\
 It's now unknown if public ip address has actually changed.\n");
                        exit(EXIT_FAILURE);
                }

                printf("Launch update_ip_dns.sh to update public ip in dns zones to: %s\n", ipaddrstr);
                char cmdlaunchscript[] = "/bin/sh ./update_ip_dns.sh ";
                strcat(cmdlaunchscript, ipaddrstr);
                /* printf("cmdlaunchscript = [ %s ]\n", cmdlaunchscript); // DEBUG */
                unsigned short exitcode = system(cmdlaunchscript);
                if (exitcode != 0) {
                        printf("Script returned error (exitcode %hu).\n", exitcode);
                }
        }

        /* Rename file */
        int ret = rename(filepathipnow, filepathipwas);
        if (ret != 0) {
                fprintf(stderr, "Error: unable to rename %s to %s.\n", filepathipnow, filepathipwas);
                exit(EXIT_FAILURE);
        }

        return EXIT_SUCCESS;
}

