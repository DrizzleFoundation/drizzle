#ifndef HA_INNODB_PROTOTYPES_H
#define HA_INNODB_PROTOTYPES_H

#ifndef UNIV_HOTBACKUP

#include "univ.i" /* ulint, uint */
#include <mystrings/m_ctype.h> /* CHARSET_INFO */

/* Prototypes for global functions in ha_innodb.cc that are called by
InnoDB's C-code. */

/*************************************************************************
Wrapper around MySQL's copy_and_convert function, see it for
documentation. */

ulint
innobase_convert_string(
/*====================*/
	void*		to,
	ulint		to_length,
	CHARSET_INFO*	to_cs,
	const void*	from,
	ulint		from_length,
	CHARSET_INFO*	from_cs,
	uint*		errors);

/*********************************************************************
Display an SQL identifier. */

void
innobase_print_identifier(
/*======================*/
	FILE*		f,	/* in: output stream */
	trx_t*		trx,	/* in: transaction */
	ibool		table_id,/* in: TRUE=print a table name,
				FALSE=print other identifier */
	const char*	name,	/* in: name to print */
	ulint		namelen);/* in: length of name */

/**********************************************************************
Returns true if the thread is the replication thread on the slave
server. Used in srv_conc_enter_innodb() to determine if the thread
should be allowed to enter InnoDB - the replication thread is treated
differently than other threads. Also used in
srv_conc_force_exit_innodb(). */

ibool
session_is_replication_slave_thread(
/*============================*/
			/* out: true if session is the replication thread */
	void*	session);	/* in: thread handle (THD*) */

/**********************************************************************
Returns true if the transaction this thread is processing has edited
non-transactional tables. Used by the deadlock detector when deciding
which transaction to rollback in case of a deadlock - we try to avoid
rolling back transactions that have edited non-transactional tables. */

ibool
session_has_edited_nontrans_tables(
/*===========================*/
			/* out: true if non-transactional tables have
			been edited */
	void*	session);	/* in: thread handle (THD*) */

#endif
#endif
