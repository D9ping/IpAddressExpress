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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sqlite3.h>


int create_table_ipservice(sqlite3 *db, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "CREATE TABLE IF NOT EXISTS `ipservice` ( \
 `nr` INT PRIMARY KEY NOT NULL, \
 `disabled` TINYINT NOT NULL DEFAULT 0, \
 `type` TINYINT NOT NULL, \
 `url` TEXT(1023) NOT NULL, \
 `lastErrorOn` NUMERIC );", -1, &stmt, NULL);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error creating ipservice table: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Table ipservice succesfully created.\n");
        }

        return retcode;
}


/**
        Add a ipservice to the ipservice database table.
        ipservice: 
        if disabled is true then the ipservice has a pernament error and should not be used.
        type = 0 for dns(not supported), type = 1 for http, type = 2 for https
 */
int add_ipservice(sqlite3 *db, int urlnr, char *url, bool disabled, int type, bool verbosemode)
{
        if (disabled < 0 || disabled > 1) {
                exit(EXIT_FAILURE);
        }

        if (type < 0 || type > 2) {
                exit(EXIT_FAILURE);
        }

        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "INSERT INTO `ipservice` (`nr`, `disabled`, `type`, `url`)\
 VALUES (?1, ?2, ?3, ?4);", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, urlnr);
        sqlite3_bind_int(stmt, 2, disabled);
        sqlite3_bind_int(stmt, 3, type);
        sqlite3_bind_text(stmt, 4, url, -1, SQLITE_STATIC);
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
        Get the number of ipservices in the ipservice table.
*/
int get_count_ipservices(sqlite3 *db)
{
        int items = 0;
        int rc = 0;
        sqlite3_stmt *stmt = NULL;
        rc = sqlite3_prepare_v2(db, 
                                "SELECT COUNT(`nr`) FROM `ipservice` LIMIT 1;",
                                1023,
                                &stmt,
                                NULL);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                items = sqlite3_column_int(stmt, 0);
                break;
        }

        sqlite3_finalize(stmt);
        return items;
}


/**
        Get the url for a ipservice number.
*/
const char * get_url_ipservice(sqlite3 *db, int urlnr)
{
        const char *url;
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        retcode = sqlite3_prepare_v2(db,
                                     "SELECT `url` FROM `ipservice` WHERE `nr` = ?1 LIMIT 1;", 
                                     2047,
                                     &stmt,
                                     NULL);
        sqlite3_bind_int(stmt, 1, urlnr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
                url = sqlite3_column_text(stmt, 0);
                break;
        }

        sqlite3_finalize(stmt);
        return url;
}


int update_ipsevice_temporary_disable(sqlite3 *db, int urlnr)
{
        int timestampnow = (int)time(NULL);
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        retcode = sqlite3_prepare_v2(db,
                                     "UPDATE `ipservice` SET `disabled`= 1, `lasterroron`= ?1\
 WHERE  `nr`= ?2 LIMIT 1;", 
                                     -1,
                                     &stmt,
                                     NULL);
        sqlite3_bind_int(stmt, 1, timestampnow);
        sqlite3_bind_int(stmt, 2, urlnr);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error updating ipservice: %s\n", sqlite3_errmsg(db));
        }

        sqlite3_finalize(stmt);
        return retcode;
}


/**
        Create config table.
 */
int create_table_config(sqlite3 *db, bool verbosemode)
{
        int retcode;
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "CREATE TABLE IF NOT EXISTS `config` ( \
 `name` TEXT, \
 `valueint` INT NOT NULL);", -1, &stmt, NULL);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error creating config table: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Table config succesfully created.\n");
        }

        return retcode;
}


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


int update_config_value_int(sqlite3 *db, char * name, int value, bool verbosemode)
{
        int retcode = 0;
        sqlite3_stmt *stmt = NULL;
        retcode = sqlite3_prepare_v2(db,
                                     "UPDATE `config` SET `valueint`= ?1\
 WHERE  `name`= ?2 LIMIT 1;", 
                                     -1,
                                     &stmt,
                                     NULL);
        sqlite3_bind_int(stmt, 1, value);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
        retcode = sqlite3_step(stmt);
        if (retcode != SQLITE_DONE) {
                fprintf(stderr, "Error storing config value: %s\n", sqlite3_errmsg(db));
        } else if (verbosemode) {
                fprintf(stdout, "Config: %s = %d  saved.\n", name, value);
        }

        sqlite3_finalize(stmt);
        return retcode;
}

