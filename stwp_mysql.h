#ifndef _stwp_mysql_h_
#define _stwp_mysql_h_
#include <mysql/mysql.h>

//#define     SERVER              "192.168.1.3"
#define     SERVER              "127.0.0.1"
#define     USER                "root"
#define     PASSWD              "123456"
//#define     PASSWD              ""
#define     DATABASE            "stwp"



typedef struct StwpMysql
{
#if 0
 typedef struct st_mysql_res {
  my_ulonglong row_count;
  unsigned int  field_count, current_field;
  MYSQL_FIELD   *fields;
  MYSQL_DATA    *data;
  MYSQL_ROWS    *data_cursor;
  MEM_ROOT      field_alloc;
  MYSQL_ROW     row;            /* If unbuffered read */
  MYSQL_ROW     current_row;    /* buffer to current row */
  unsigned long *lengths;       /* column lengths of current row */
  MYSQL         *handle;        /* for unbuffered reads */
  my_bool       eof;            /* Used my mysql_fetch_row */
} MYSQL_RES;
#endif 
    MYSQL       *conn;
    MYSQL_RES   *res;
    MYSQL_ROW   row;
    MYSQL_FIELD *fields;
}stwp_mysql_t;

extern stwp_mysql_t stwp_mysql;

#endif
