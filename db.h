#include <stdio.h>
#include <stdbool.h>
#include <sqlite3.h>

int create_table_ipservice(sqlite3 *db, bool verbosemode);

int add_ipservice(sqlite3 *db, int urlnr, char *url, bool disabled, int type, bool verbosemode);

int get_count_ipservices(sqlite3 *db);

const char * get_url_ipservice(sqlite3 *db, int urlnr);

int update_ipsevice_temporary_disable(sqlite3 *db, int urlnr);


int create_table_config(sqlite3 *db, bool verbosemode);

int get_config_value_int(sqlite3 *db, char *name);

int add_config_value_int(sqlite3 *db, char * name, int value, bool verbosemode);

int update_config_value_int(sqlite3 *db, char *name, int value, bool verbosemode);

