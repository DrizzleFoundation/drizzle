/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2008 Sun Microsystems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* The old structures from unireg */

#include <mysys/iocache.h>

class Table;
class Field;

typedef struct st_date_time_format {
  unsigned char positions[8];
  char  time_separator;			/* Separator between hour and minute */
  uint32_t flag;				/* For future */
  LEX_STRING format;
} DATE_TIME_FORMAT;


typedef struct st_keyfile_info {	/* used with ha_info() */
  unsigned char ref[MAX_REFLENGTH];		/* Pointer to current row */
  unsigned char dupp_ref[MAX_REFLENGTH];	/* Pointer to dupp row */
  uint32_t ref_length;			/* Length of ref (1-8) */
  uint32_t block_size;			/* index block size */
  File filenr;				/* (uniq) filenr for table */
  ha_rows records;			/* Records i datafilen */
  ha_rows deleted;			/* Deleted records */
  uint64_t data_file_length;		/* Length off data file */
  uint64_t max_data_file_length;	/* Length off data file */
  uint64_t index_file_length;
  uint64_t max_index_file_length;
  uint64_t delete_length;		/* Free bytes */
  uint64_t auto_increment_value;
  int errkey,sortkey;			/* Last errorkey and sorted by */
  time_t create_time;			/* When table was created */
  time_t check_time;
  time_t update_time;
  uint64_t mean_rec_length;		/* physical reclength */
} KEYFILE_INFO;


typedef struct st_key_part_info {	/* Info about a key part */
  Field *field;
  uint	offset;				/* offset in record (from 0) */
  uint	null_offset;			/* Offset to null_bit in record */
  /* Length of key part in bytes, excluding NULL flag and length bytes */
  uint16_t length;
  /*
    Number of bytes required to store the keypart value. This may be
    different from the "length" field as it also counts
     - possible NULL-flag byte (see HA_KEY_NULL_LENGTH) [if null_bit != 0,
       the first byte stored at offset is 1 if null, 0 if non-null; the
       actual value is stored from offset+1].
     - possible HA_KEY_BLOB_LENGTH bytes needed to store actual value length.
  */
  uint16_t store_length;
  uint16_t key_type;
  uint16_t fieldnr;			/* Fieldnum in UNIREG (1,2,3,...) */
  uint16_t key_part_flag;			/* 0 or HA_REVERSE_SORT */
  uint8_t type;
  uint8_t null_bit;			/* Position to null_bit */
} KEY_PART_INFO ;


typedef struct st_key {
  uint	key_length;			/* Tot length of key */
  ulong flags;                          /* dupp key and pack flags */
  uint	key_parts;			/* How many key_parts */
  uint32_t  extra_length;
  uint	usable_key_parts;		/* Should normally be = key_parts */
  uint32_t  block_size;
  enum  ha_key_alg algorithm;
  KEY_PART_INFO *key_part;
  char	*name;				/* Name of key */
  /*
    Array of AVG(#records with the same field value) for 1st ... Nth key part.
    0 means 'not known'.
    For temporary heap tables this member is NULL.
  */
  ulong *rec_per_key;
  Table *table;
  LEX_STRING comment;
} KEY;


struct st_join_table;

typedef struct st_reginfo {		/* Extra info about reg */
  struct st_join_table *join_tab;	/* Used by SELECT() */
  enum thr_lock_type lock_type;		/* How database is used */
  bool not_exists_optimize;
  bool impossible_range;
} REGINFO;


struct st_read_record;				/* For referense later */
class SQL_SELECT;
class Session;
class handler;
struct st_join_table;

typedef struct st_read_record {			/* Parameter to read_record */
  Table *table;			/* Head-form */
  handler *file;
  Table **forms;			/* head and ref forms */
  int (*read_record)(struct st_read_record *);
  Session *session;
  SQL_SELECT *select;
  uint32_t cache_records;
  uint32_t ref_length,struct_length,reclength,rec_cache_size,error_offset;
  uint32_t index;
  unsigned char *ref_pos;				/* pointer to form->refpos */
  unsigned char *record;
  unsigned char *rec_buf;                /* to read field values  after filesort */
  unsigned char	*cache,*cache_pos,*cache_end,*read_positions;
  IO_CACHE *io_cache;
  bool print_error, ignore_not_found_rows;
  struct st_join_table *do_insideout_scan;
} READ_RECORD;


typedef struct {
  uint32_t year;
  uint32_t month;
  uint32_t day;
  uint32_t hour;
  uint64_t minute,second,second_part;
  bool neg;
} INTERVAL;


typedef struct st_known_date_time_format {
  const char *format_name;
  const char *date_format;
  const char *datetime_format;
  const char *time_format;
} KNOWN_DATE_TIME_FORMAT;

enum SHOW_COMP_OPTION { SHOW_OPTION_YES, SHOW_OPTION_NO, SHOW_OPTION_DISABLED};

extern const char *show_comp_option_name[];

typedef int *(*update_var)(Session *, struct st_mysql_show_var *);

typedef struct	st_lex_user {
  LEX_STRING user, host, password;
} LEX_USER;

/*
  This structure specifies the maximum amount of resources which
  can be consumed by each account. Zero value of a member means
  there is no limit.
*/
typedef struct user_resources {
  /* Maximum number of queries/statements per hour. */
  uint32_t questions;
  /*
     Maximum number of updating statements per hour (which statements are
     updating is defined by sql_command_flags array).
  */
  uint32_t updates;
  /* Maximum number of connections established per hour. */
  uint32_t conn_per_hour;
  /* Maximum number of concurrent connections. */
  uint32_t user_conn;
  /*
     Values of this enum and specified_limits member are used by the
     parser to store which user limits were specified in GRANT statement.
  */
  enum {QUERIES_PER_HOUR= 1, UPDATES_PER_HOUR= 2, CONNECTIONS_PER_HOUR= 4,
        USER_CONNECTIONS= 8};
  uint32_t specified_limits;
} USER_RESOURCES;


/*
  This structure is used for counting resources consumed and for checking
  them against specified user limits.
*/
typedef struct  user_conn {
  /*
     Pointer to user+host key (pair separated by '\0') defining the entity
     for which resources are counted (By default it is user account thus
     priv_user/priv_host pair is used. If --old-style-user-limits option
     is enabled, resources are counted for each user+host separately).
  */
  char *user;
  /* Pointer to host part of the key. */
  char *host;
  /**
     The moment of time when per hour counters were reset last time
     (i.e. start of "hour" for conn_per_hour, updates, questions counters).
  */
  uint64_t reset_utime;
  /* Total length of the key. */
  uint32_t len;
  /* Current amount of concurrent connections for this account. */
  uint32_t connections;
  /*
     Current number of connections per hour, number of updating statements
     per hour and total number of statements per hour for this account.
  */
  uint32_t conn_per_hour, updates, questions;
  /* Maximum amount of resources which account is allowed to consume. */
  USER_RESOURCES user_resources;
} USER_CONN;

	/* Bits in form->update */
#define REG_MAKE_DUPP		1	/* Make a copy of record when read */
#define REG_NEW_RECORD		2	/* Write a new record if not found */
#define REG_UPDATE		4	/* Uppdate record */
#define REG_DELETE		8	/* Delete found record */
#define REG_PROG		16	/* User is updating database */
#define REG_CLEAR_AFTER_WRITE	32
#define REG_MAY_BE_UPDATED	64
#define REG_AUTO_UPDATE		64	/* Used in D-forms for scroll-tables */
#define REG_OVERWRITE		128
#define REG_SKIP_DUP		256

	/* Bits in form->status */
#define STATUS_NO_RECORD	(1+2)	/* Record isn't usably */
#define STATUS_GARBAGE		1
#define STATUS_NOT_FOUND	2	/* No record in database when needed */
#define STATUS_NO_PARENT	4	/* Parent record wasn't found */
#define STATUS_NOT_READ		8	/* Record isn't read */
#define STATUS_UPDATED		16	/* Record is updated by formula */
#define STATUS_NULL_ROW		32	/* table->null_row is set */
#define STATUS_DELETED		64

/*
  Such interval is "discrete": it is the set of
  { auto_inc_interval_min + k * increment,
    0 <= k <= (auto_inc_interval_values-1) }
  Where "increment" is maintained separately by the user of this class (and is
  currently only session->variables.auto_increment_increment).
  It mustn't derive from Sql_alloc, because SET INSERT_ID needs to
  allocate memory which must stay allocated for use by the next statement.
*/
class Discrete_interval {
private:
  uint64_t interval_min;
  uint64_t interval_values;
  uint64_t  interval_max;    // excluded bound. Redundant.
public:
  Discrete_interval *next;    // used when linked into Discrete_intervals_list
  void replace(uint64_t start, uint64_t val, uint64_t incr)
  {
    interval_min=    start;
    interval_values= val;
    interval_max=    (val == UINT64_MAX) ? val : start + val * incr;
  }
  Discrete_interval(uint64_t start, uint64_t val, uint64_t incr) :
    interval_min(start), interval_values(val),
    interval_max((val == UINT64_MAX) ? val : start + val * incr),
    next(NULL)
  {};
  Discrete_interval() :
    interval_min(0), interval_values(0),
    interval_max(0), next(NULL)
  {};
  uint64_t minimum() const { return interval_min;    };
  uint64_t values()  const { return interval_values; };
  uint64_t maximum() const { return interval_max;    };
  /*
    If appending [3,5] to [1,2], we merge both in [1,5] (they should have the
    same increment for that, user of the class has to ensure that). That is
    just a space optimization. Returns 0 if merge succeeded.
  */
  bool merge_if_contiguous(uint64_t start, uint64_t val, uint64_t incr)
  {
    if (interval_max == start)
    {
      if (val == UINT64_MAX)
      {
        interval_values=   interval_max= val;
      }
      else
      {
        interval_values+=  val;
        interval_max=      start + val * incr;
      }
      return 0;
    }
    return 1;
  };
};

/* List of Discrete_interval objects */
class Discrete_intervals_list {
private:
  Discrete_interval        *head;
  Discrete_interval        *tail;
  /*
    When many intervals are provided at the beginning of the execution of a
    statement (in a replication slave or SET INSERT_ID), "current" points to
    the interval being consumed by the thread now (so "current" goes from
    "head" to "tail" then to NULL).
  */
  Discrete_interval        *current;
  uint32_t                  elements; // number of elements

  /* helper function for copy construct and assignment operator */
  void copy_(const Discrete_intervals_list& from)
  {
    for (Discrete_interval *i= from.head; i; i= i->next)
    {
      Discrete_interval j= *i;
      append(&j);
    }
  }
public:
  Discrete_intervals_list() :
    head(NULL), tail(NULL),
    current(NULL), elements(0) {};
  Discrete_intervals_list(const Discrete_intervals_list& from) :
    head(NULL), tail(NULL),
    current(NULL), elements(0)
  {
    copy_(from);
  }
  Discrete_intervals_list& operator=(const Discrete_intervals_list& from)
  {
    empty();
    copy_(from);
    return *this;
  }
  void empty_no_free()
  {
    head= current= NULL;
    elements= 0;
  }
  void empty()
  {
    for (Discrete_interval *i= head; i;)
    {
      Discrete_interval *next= i->next;
      delete i;
      i= next;
    }
    empty_no_free();
  }

  const Discrete_interval* get_next()
  {
    Discrete_interval *tmp= current;
    if (current != NULL)
      current= current->next;
    return tmp;
  }
  ~Discrete_intervals_list() { empty(); };
  bool append(uint64_t start, uint64_t val, uint64_t incr);
  bool append(Discrete_interval *interval);
  uint64_t minimum()     const { return (head ? head->minimum() : 0); };
  uint64_t maximum()     const { return (head ? tail->maximum() : 0); };
  uint32_t      nb_elements() const { return elements; }
};
