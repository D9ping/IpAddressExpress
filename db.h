/***************************************************************************
 *    Copyright (C) 2018-2019 D9ping
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
#include <sqlite3.h>

int create_table_ipservice(sqlite3 *db, bool verbosemode);

int add_ipservice(sqlite3 *db, int urlnr, char * url, bool disabled, int protocoltype, int priority, bool verbosemode);

int get_count_all_ipservices(sqlite3 *db);

int get_count_available_ipservices(sqlite3 *db, int allowedprotocoltypes);

const char * get_url_ipservice(sqlite3 *db, int urlnr);

int update_disabled_ipsevice(sqlite3 *db, int urlnr, bool addtimestamp);

void get_disabled_ipservices(sqlite3 *db, int urlnrs_avoid[], bool verbosemode);

const int get_priority_ipservice(sqlite3 *db, int urlnr);

void get_urlnrs_ipservices(sqlite3 *db, int urlnrs[], int disabled, int allowedprotocoltypes);

int reenable_expired_disabled_ipservices(sqlite3 *db, int blockseconds);

int create_table_config(sqlite3 *db, bool verbosemode);

int get_config_value_int(sqlite3 *db, char *name);

char * get_config_value_str(sqlite3 *db, char *name);

int add_config_value_int(sqlite3 *db, char *name, int value, bool verbosemode);

int add_config_value_str(sqlite3 *db, char *name, char * value, bool verbosemode);

int update_config_value_int(sqlite3 *db, char *name, int valuenew, bool verbosemode);

int update_config_value_str(sqlite3 *db, char *name, const char * valuenew, bool verbosemode);

bool is_config_exists(sqlite3 *db, char *name);

