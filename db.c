/***************************************************************************
 *    Copyright (C) 2018 D9ping
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
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sqlite3.h>

#define MAXLENURL          1023
#define MAXLENCONFIGSTR    255
#define MAXPRIORITY        9

/**
 * Create table ipservice with all ipservices to possible use.
 * @param verbosemode Print a message if ipservice table is successfully created.
 */
int create_table_ipservice(sqlite3 *db, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "CREATE TABLE IF NOT EXISTS `ipservice` ( \
 `nr` INT PRIMARY KEY NOT NULL, \
 `disabled` TINYINT NOT NULL DEFAULT 0, \
 `protocoltype` TINYINT NOT NULL, \
 `url` TEXT(1023) NOT NULL, \
 `priority` TINYINT(10) NOT NULL DEFAULT 1, \
 `lastErrorOn` NUMERIC );", -1, &stmt, NULL);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error creating ipservice table: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Table ipservice succesfully created.\n");
        }

        sqlite3_finalize(stmt);
        return retcode;
}


/**
 * Add an https/http or dns ip service to the ipservice database table.
 * if disabled is true then the ipservice has a persistent error and should not be used anymore.
 * @param urlnr        The ipservice number.
 * @param url          The url or address of the ipservice.
 * @param disabled     Is the ipservice disabled.
 * @param protocoltype The protocol type to use. 0 for dns(not yet implemented), protocoltype = 1 for http,
                       and protocoltype = 2 for https.
 * @param priority     The priority of the ipservice to use. From 1(most favourable) till 10(least favourable to use) at most.
 * @param verbosemode  Print a message if ipservice is succesfully added to database.
 */
int add_ipservice(sqlite3 *db, int urlnr, char * url, bool disabled, int protocoltype, int priority, bool verbosemode)
{
        if (protocoltype < 0 || protocoltype > 2) {
                exit(EXIT_FAILURE);
        }

        if (priority < 1 || priority > MAXPRIORITY) {
                exit(EXIT_FAILURE);

        }

        if (strlen(url) >= MAXLENURL) {
            printf("Url too long, maximum %d character allowed.\n", MAXLENURL);
            return 1;
        }

        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "INSERT INTO `ipservice` (`nr`, `disabled`, `protocoltype`, `url`, `priority`)\
 VALUES (?1, ?2, ?3, ?4, ?5);", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, urlnr);
        sqlite3_bind_int(stmt, 2, disabled);
        sqlite3_bind_int(stmt, 3, protocoltype);
        sqlite3_bind_text(stmt, 4, url, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, priority);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Added ipservice record.\n");
        }

        sqlite3_finalize(stmt);
        return retcode;
}

/**
 * Get the number of ipservices in the ipservice table.
 */
int get_count_all_ipservices(sqlite3 *db)
{
        int cntall = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT COUNT(`nr`) FROM `ipservice` LIMIT 1;",
                           1023,
                           &stmt,
                           NULL);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
                cntall = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return cntall;
}

/**
 * Get the number of available ipservices.
 * @param allowedprotocoltypes The allowed protocols to use.
 */
int get_count_available_ipservices(sqlite3 *db, int allowedprotocoltypes)
{
        int cntavailable = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT COUNT(`nr`) FROM `ipservice` WHERE `disabled` = 0 AND protocoltype >= ?1 LIMIT 1;",
                           -1,
                           &stmt,
                           NULL);
        sqlite3_bind_int(stmt, 1, allowedprotocoltypes);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
                cntavailable = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return cntavailable;
}

/**
 * Get the url for a ipservice number.
 * @param urlnr The ipservice number to get the url from.
 */
const char * get_url_ipservice(sqlite3 *db, int urlnr)
{
        const char *url;
        char *urlipservice;
        int sizeurlipservice = MAXLENURL * sizeof(char);
        urlipservice = malloc(sizeurlipservice);
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT `url` FROM `ipservice` WHERE `nr` = ?1 LIMIT 1;",
                           sizeurlipservice,
                           &stmt,
                           NULL);
        sqlite3_bind_int(stmt, 1, urlnr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
                url = sqlite3_column_text(stmt, 0);
        } else {
                url = "";
        }

        strcpy(urlipservice, url);
        sqlite3_finalize(stmt);
        return urlipservice;
}

/**
 * Disable an ipservice.
 * @param urlnr        The number of the ipservice to disable.
 * @param addtimestamp Add a timestamp on the time the ipservice was disabled on.
 */
int update_disabled_ipsevice(sqlite3 *db, int urlnr, bool addtimestamp)
{
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        if (addtimestamp) {
                int timestampnow = (int)time(NULL);
                sqlite3_prepare_v2(db,
                                    "UPDATE `ipservice` SET `disabled`= 1, `lasterroron`= ?1\
 WHERE `nr`= ?2 LIMIT 1;",
                                   -1,
                                   &stmt,
                                   NULL);
                sqlite3_bind_int(stmt, 1, timestampnow);
                sqlite3_bind_int(stmt, 2, urlnr);
        } else {
               sqlite3_prepare_v2(db,
                                  "UPDATE `ipservice` SET `disabled`= 1 WHERE `nr`= ?2 LIMIT 1;",
                                  -1,
                                  &stmt,
                                  NULL);
               sqlite3_bind_int(stmt, 2, urlnr);
        }

        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error updating ipservice: %s\n", sqlite3_errmsg(db));
        }

        sqlite3_finalize(stmt);
        return retcode;
}

/**
 * Get the priority of a ipservice.
 * @param urlnr The ipservice to get the priority from.
 */
const int get_priority_ipservice(sqlite3 *db, int urlnr)
{
        int priority = -1;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT `priority` FROM `ipservice` WHERE `nr` = ?1 LIMIT 1;",
                           255,
                           &stmt,
                           NULL);
        sqlite3_bind_int(stmt, 1, urlnr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
                priority = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return priority;
}

/**
 * Get a array with the numbers of ipservices requested.
 * @param urlnr
 * @param disabled
 * @param allowedprotocoltypes
 */
void get_urlnrs_ipservices(sqlite3 *db, int urlnrs[], int disabled, int allowedprotocoltypes)
{
        int i = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                            "SELECT `nr` FROM `ipservice` WHERE `disabled` = ?1 AND protocoltype >= ?2;",
                            -1,
                            &stmt,
                            NULL);
        sqlite3_bind_int(stmt, 1, disabled);
        sqlite3_bind_int(stmt, 2, allowedprotocoltypes);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                int nr;
                nr = sqlite3_column_int(stmt, 0);
                urlnrs[i] = nr;
                ++i;
        }

        sqlite3_finalize(stmt);
}

/**
 * Re-enable all the expired temporary disabled ipservice with a SQL query.
 * @param blockseconds The number of seconds the disabling
 */
int reenable_expired_disabled_ipservices(sqlite3 *db, int blockseconds)
{
        int retcode = 0;
        int timestampunblock = (int)time(NULL) - blockseconds;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                        "UPDATE `ipservice` SET `disabled` = 0 WHERE `disabled` = 1 AND `lasterroron` <= ?1;",
                        -1,
                        &stmt,
                        NULL);
        sqlite3_bind_int(stmt, 1, timestampunblock);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error on reenabling expired disabled ipservices: %s\n", sqlite3_errmsg(db));
        }

        sqlite3_finalize(stmt);
        return retcode;
}

/**
 * Create config table.
 * @param verbosemode Print a message if config table is created succesfully.
 */
int create_table_config(sqlite3 *db, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "CREATE TABLE IF NOT EXISTS `config` ( \
 `name` TEXT, \
 `valueint` INT, \
 `valuestr` TEXT(255) );", -1, &stmt, NULL);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error creating config table: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Table config succesfully created.\n");
        }

        sqlite3_finalize(stmt);
        return retcode;
}

/**
 * Get the config integer value with a certain name.
 * @param name The name to get the config integer value from.
 */
int get_config_value_int(sqlite3 *db, char * name)
{
        int value_int = -1;
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        retcode = sqlite3_prepare_v2(db,
                                     "SELECT `valueint` FROM `config` WHERE `name` = ?1 LIMIT 1;",
                                     -1,
                                     &stmt,
                                     NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                value_int = sqlite3_column_int(stmt, 0);
                break;
        }

        sqlite3_finalize(stmt);
        return value_int;
}

/**
 * Get the config string(char array) value.
 * @param name The name to get the string(char array) config value from.
 */
char * get_config_value_str(sqlite3 *db, char * name)
{
        int configstrlen = MAXLENCONFIGSTR * sizeof(char);
        char *configvaluestr;
        configvaluestr = malloc(configstrlen);
        const char * value_str;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT `valuestr` FROM `config` \
 WHERE `name` = ?1 LIMIT 1;",
                           -1,
                           &stmt,
                           NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                value_str = sqlite3_column_text(stmt, 0);
                break;
        }

        strcpy(configvaluestr, value_str);
        sqlite3_finalize(stmt);
        return configvaluestr;
}

/**
 * Add a new config value integer.
 * @param name        The lookup name of the config to add.
 * @param value       The integer configuration value.
 * @param verbosemode Print message if config added succesfully to database.
 */
int add_config_value_int(sqlite3 *db, char * name, int value, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "INSERT INTO `config` (`name`, `valueint`) \
 VALUES (?1, ?2);", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, value);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %d  saved.\n", name, value);
        }

        sqlite3_finalize(stmt);
        return retcode;
}

/**
 * Add a new config string value.
 * @param name        The lookup name of the config to add.
 * @param value       The string(char array) configuration value.
 * @param verbosemode Print message if config added succesfully to database.
 */
int add_config_value_str(sqlite3 *db, char *name, char * value, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "INSERT INTO `config` (`name`, `valuestr`) \
 VALUES (?1, ?2);", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %s  saved.\n", name, value);
        }

        sqlite3_finalize(stmt);
        return retcode;
}

/**
 * Change the integer config value.
 * @param name        The lookup name of the config to add.
 * @param valuenew    The new integer configuration value.
 * @param verbosemode Print message if config updated succesfully in database.
 */
int update_config_value_int(sqlite3 *db, char * name, int valuenew, bool verbosemode)
{
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "UPDATE `config` SET `valueint`= ?1\
 WHERE  `name`= ?2 LIMIT 1;",
                          -1,
                          &stmt,
                          NULL);
        sqlite3_bind_int(stmt, 1, valuenew);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %d  saved.\n", name, valuenew);
        }

        sqlite3_finalize(stmt);
        return retcode;
}

/**
 * Change the string config value.
 * @param name        The lookup name of the config to add.
 * @param valuenew    The new string configuration value.
 * @param verbosemode Print message if config updated succesfully in database.
 */
int update_config_value_str(sqlite3 *db, char * name, const char * valuenew, bool verbosemode)
{
        if (strlen(valuenew) > MAXLENCONFIGSTR) {
                return -1;
        }

        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "UPDATE `config` SET `valuestr`= ?1\
 WHERE `name`= ?2 LIMIT 1;",
                           -1,
                           &stmt,
                           NULL);
        sqlite3_bind_text(stmt, 1, valuenew, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %s  saved.\n", name, valuenew);
        }

        sqlite3_finalize(stmt);
        return retcode;
}

/**
 * Check if configuration exists in database config table.
 * @param name The lookup name of the config to check if it exists.
 */
bool is_config_exists(sqlite3 *db, char *name)
{
        int value_int = 0;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
                           "SELECT COUNT(`name`) FROM `config` \
 WHERE `name` = ?1 LIMIT 1;",
                           -1,
                           &stmt,
                           NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                value_int = sqlite3_column_int(stmt, 0);
                break;
        }

        sqlite3_finalize(stmt);
        if (value_int >= 1) {
            return true;
        } else {
            return false;
        }
}

