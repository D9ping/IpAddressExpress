#include <stdio.h>
#include <stdbool.h>
#include <sqlite3.h>

int create_table_ipservice(sqlite3 *db, bool verbosemode);

int add_ipservice(sqlite3 *db, int urlnr, char * url, bool disabled, int type, bool verbosemode);

int get_count_all_ipservices(sqlite3 *db);

int get_count_available_ipservices(sqlite3 *db, int allowedprotocoltypes);

const char * get_url_ipservice(sqlite3 *db, int urlnr);

int update_disabled_ipsevice(sqlite3 *db, int urlnr, bool addtimestamp);

void get_disabled_ipservices(sqlite3 *db, int urlnrs_avoid[], bool verbosemode);

void get_urlnrs_ipservices(sqlite3 *db, int urlnrs[], int disabled, int allowedprotocoltypes);

int create_table_config(sqlite3 *db, bool verbosemode);

int get_config_value_int(sqlite3 *db, char *name);

char * get_config_value_str(sqlite3 *db, char *name);

int add_config_value_int(sqlite3 *db, char *name, int value, bool verbosemode);

int add_config_value_str(sqlite3 *db, char *name, char * value, bool verbosemode);

int update_config_value_int(sqlite3 *db, char *name, int valuenew, bool verbosemode);

int update_config_value_str(sqlite3 *db, char *name, const char * valuenew, bool verbosemode);

bool is_config_exists(sqlite3 *db, char *name);

