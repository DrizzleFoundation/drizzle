/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2008-2009 Sun Microsystems
 *
 *  Authors:
 *
 *  Jay Pipes <joinfu@sun.com>
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

/**
 * @file
 *
 * Defines the implementation of the default transaction log.
 *
 * @see drizzled/plugin/transaction_replicator.h
 * @see drizzled/plugin/transaction_applier.h
 *
 * @details
 *
 * Currently, the log file uses this implementation:
 *
 * We have an atomic off_t called log_offset which keeps track of the 
 * offset into the log file for writing the next Transaction.
 *
 * We write Transaction message encapsulated in a 4-byte length header and a
 * 4-byte checksum trailer.
 *
 * When writing a Transaction to the log, we calculate the length of the 
 * Transaction to be written.  We then increment log_offset by the length
 * of the Transaction plus sizeof(uint32_t) plus sizeof(uint32_t) and store 
 * this new offset in a local off_t called cur_offset (see TransactionLog::apply().  
 * This compare and set is done in an atomic instruction.
 *
 * We then adjust the local off_t (cur_offset) back to the original
 * offset by subtracting the length and sizeof(uint32_t) and sizeof(uint32_t).
 *
 * We then first write a 64-bit length and then the serialized transaction/transaction
 * and optional checksum to our log file at our local cur_offset.
 *
 * ------------------------------------------------------------------
 * |<- 4 bytes ->|<- # Bytes of Transaction Message ->|<- 4 bytes ->|
 * ------------------------------------------------------------------
 * |   Length    |   Serialized Transaction Message   |   Checksum  |
 * ------------------------------------------------------------------
 *
 * @todo
 *
 * Possibly look at a scoreboard approach with multiple file segments.  For
 * right now, though, this is just a quick simple implementation to serve
 * as a skeleton and a springboard.
 *
 * Also, we can move to a ZeroCopyStream implementation instead of using the
 * string as a buffer in apply()
 */

#include "transaction_log.h"

#include <unistd.h>

#include <vector>
#include <string>

#include <drizzled/session.h>
#include <drizzled/set_var.h>
#include <drizzled/gettext.h>
#include <drizzled/hash/crc32.h>
#include <drizzled/message/transaction.pb.h>
#include <google/protobuf/io/coded_stream.h>

using namespace std;
using namespace drizzled;
using namespace google;

/** 
 * Transaction Log plugin system variable - Is the log enabled? Only used on init().  
 * The enable() and disable() methods of the TransactionLog class control online
 * disabling.
 */
static bool sysvar_transaction_log_enabled= false;
/** Transaction Log plugin system variable - The path to the log file used */
static char* sysvar_transaction_log_file= NULL;
/** 
 * Transaction Log plugin system variable - A debugging variable to assist 
 * in truncating the log file. 
 */
static bool sysvar_transaction_log_truncate_debug= false;
static const char DEFAULT_LOG_FILE_PATH[]= "transaction.log"; /* In datadir... */
/** 
 * Transaction Log plugin system variable - Should we write a CRC32 checksum for 
 * each written Transaction message?
 */
static bool sysvar_transaction_log_checksum_enabled= false;

TransactionLog::TransactionLog(const char *in_log_file_path, bool in_do_checksum)
  : 
    plugin::TransactionApplier(),
    state(OFFLINE),
    log_file_path(in_log_file_path)
{
  is_enabled= true; /* If constructed, the plugin is enabled until taken offline with disable() */
  is_active= false;
  do_checksum= in_do_checksum; /* Have to do here, not in initialization list b/c atomic<> */

  /* Setup our log file and determine the next write offset... */
  log_file= open(log_file_path, O_APPEND|O_CREAT|O_SYNC|O_WRONLY, S_IRWXU);
  if (log_file == -1)
  {
    errmsg_printf(ERRMSG_LVL_ERROR, _("Failed to open transaction log file %s.  Got error: %s\n"), 
                  log_file_path, 
                  strerror(errno));
    is_active= false;
    return;
  }

  /* 
   * The offset of the next write is the current position of the log
   * file, since it's opened in append mode...
   */
  log_offset= lseek(log_file, 0, SEEK_END);

  state= ONLINE;
  is_active= true;
}

TransactionLog::~TransactionLog()
{
  /* Clear up any resources we've consumed */
  if (isActive() && log_file != -1)
  {
    (void) close(log_file);
  }
}

bool TransactionLog::isActive()
{
  return is_enabled && is_active;
}

void TransactionLog::apply(const message::Transaction &to_apply)
{
  /* 
   * There is an issue on Solaris/SunStudio where if the std::string buffer is
   * NOT initialized with the below, the code produces an EFAULT when accessing
   * c_str() later on.  Stoopid, but true.
   */
  string buffer(""); /* Buffer we will write serialized transaction to */

  static const uint32_t HEADER_TRAILER_BYTES= sizeof(uint32_t) + /* 4-byte length header */
                                              sizeof(uint32_t); /* 4 byte checksum trailer */

  size_t length;
  ssize_t written;
  off_t cur_offset;

  to_apply.SerializeToString(&buffer);

  length= buffer.length(); 

  /*
   * Do an atomic increment on the offset of the log file position
   */
  cur_offset= log_offset.fetch_and_add(static_cast<off_t>((HEADER_TRAILER_BYTES + length)));

  /*
   * We adjust cur_offset back to the original log_offset before
   * the increment above...
   */
  cur_offset-= static_cast<off_t>((HEADER_TRAILER_BYTES + length));

  /* 
   * Quick safety...if an error occurs below, the log file will
   * not be active, therefore a caller could have been ready
   * to write...but the log is crashed.
   */
  if (unlikely(state == CRASHED))
    return;

  /* We always write in network byte order */
  uint8_t length_bytes[sizeof(uint32_t)];
  protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(length, length_bytes);

  /* Write the length header */
  do
  {
    written= pwrite(log_file, length_bytes, sizeof(uint32_t), cur_offset);
  }
  while (written == -1 && errno == EINTR); /* Just retry the write when interrupted by a signal... */

  if (unlikely(written != sizeof(uint32_t)))
  {
    errmsg_printf(ERRMSG_LVL_ERROR, 
                  _("Failed to write full size of transaction.  Tried to write %" PRId64
                    " bytes at offset %" PRId64 ", but only wrote %" PRId32 " bytes.  Error: %s\n"), 
                  static_cast<int64_t>(sizeof(uint32_t)), 
                  static_cast<int64_t>(cur_offset),
                  static_cast<int64_t>(written), 
                  strerror(errno));
    state= CRASHED;
    /* 
     * Reset the log's offset in case we want to produce a decent error message including
     * the original offset where an error occurred.
     */
    log_offset= cur_offset;
    is_active= false;
    return;
  }

  cur_offset+= static_cast<off_t>(written);

  /* 
   * Quick safety...if an error occurs above in another writer, the log 
   * file will be in a crashed state.
   */
  if (unlikely(state == CRASHED))
  {
    /* 
     * Reset the log's offset in case we want to produce a decent error message including
     * the original offset where an error occurred.
     */
    log_offset= cur_offset;
    return;
  }

  /* Write the transaction message itself */
  do
  {
    written= pwrite(log_file, buffer.c_str(), length, cur_offset);
  }
  while (written == -1 && errno == EINTR); /* Just retry the write when interrupted by a signal... */

  if (unlikely(written != static_cast<ssize_t>(length)))
  {
    errmsg_printf(ERRMSG_LVL_ERROR, 
                  _("Failed to write full serialized transaction.  Tried to write %" PRId64 
                    " bytes at offset %" PRId64 ", but only wrote %" PRId64 " bytes.  Error: %s\n"), 
                  static_cast<int64_t>(length), 
                  static_cast<int64_t>(cur_offset),
                  static_cast<int64_t>(written), 
                  strerror(errno));
    state= CRASHED;
    /* 
     * Reset the log's offset in case we want to produce a decent error message including
     * the original offset where an error occurred.
     */
    log_offset= cur_offset;
    is_active= false;
  }

  cur_offset+= static_cast<off_t>(written);

  /* 
   * Quick safety...if an error occurs above in another writer, the log 
   * file will be in a crashed state.
   */
  if (unlikely(state == CRASHED))
  {
    /* 
     * Reset the log's offset in case we want to produce a decent error message including
     * the original offset where an error occurred.
     */
    log_offset= cur_offset;
    return;
  }

  uint32_t checksum= 0;

  if (do_checksum)
  {
    checksum= drizzled::hash::crc32(buffer.c_str(), length);
  }

  /* We always write in network byte order */
  uint8_t checksum_bytes[sizeof(uint32_t)];
  protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(checksum, checksum_bytes);

  /* Write the checksum trailer */
  do
  {
    written= pwrite(log_file, checksum_bytes, sizeof(uint32_t), cur_offset);
  }
  while (written == -1 && errno == EINTR); /* Just retry the write when interrupted by a signal... */

  if (unlikely(written != static_cast<ssize_t>(sizeof(uint32_t))))
  {
    errmsg_printf(ERRMSG_LVL_ERROR, 
                  _("Failed to write full checksum of transaction.  Tried to write %" PRId64 
                    " bytes at offset %" PRId64 ", but only wrote %" PRId64 " bytes.  Error: %s\n"), 
                  static_cast<int64_t>(sizeof(uint32_t)), 
                  static_cast<int64_t>(cur_offset),
                  static_cast<int64_t>(written), 
                  strerror(errno));
    state= CRASHED;
    /* 
     * Reset the log's offset in case we want to produce a decent error message including
     * the original offset where an error occurred.
     */
    log_offset= cur_offset;
    is_active= false;
    return;
  }
}

void TransactionLog::truncate()
{
  bool orig_is_enabled= is_enabled;
  is_enabled= false;
  
  /* 
   * Wait a short amount of time before truncating.  This just prevents error messages
   * from being produced during a call to apply().  Setting is_enabled to false above
   * means that once the current caller to apply() is done, no other calls are made to
   * apply() before is_enabled is reset to its original state
   *
   * @note
   *
   * This is DEBUG code only!
   */
  usleep(500); /* Sleep for half a second */
  log_offset= (off_t) 0;
  int result;
  do
  {
    result= ftruncate(log_file, log_offset);
  }
  while (result == -1 && errno == EINTR);

  is_enabled= orig_is_enabled;
}

bool TransactionLog::findLogFilenameContainingTransactionId(const ReplicationServices::GlobalTransactionId&,
                                                        string &out_filename) const
{
  /* 
   * Currently, we simply return the single logfile name
   * Eventually, we'll have an index/hash with upper and
   * lower bounds to look up a log file with a transaction id
   */
  out_filename.assign(log_file_path);
  return true;
}

static TransactionLog *transaction_log= NULL; /* The singleton transaction log */

static int init(drizzled::plugin::Registry &registry)
{
  if (sysvar_transaction_log_enabled)
  {
    transaction_log= new TransactionLog(sysvar_transaction_log_file, 
                                sysvar_transaction_log_checksum_enabled);
    registry.add(transaction_log);
  }
  return 0;
}

static int deinit(drizzled::plugin::Registry &registry)
{
  if (transaction_log)
  {
    registry.remove(transaction_log);
    delete transaction_log;
  }
  return 0;
}

static void set_truncate_debug(Session *,
                               struct st_mysql_sys_var *, 
                               void *, 
                               const void *save)
{
  /* 
   * The const void * save comes directly from the check function, 
   * which should simply return the result from the set statement. 
   */
  if (transaction_log)
    if (*(bool *)save != false)
      transaction_log->truncate();
}

static DRIZZLE_SYSVAR_BOOL(enable,
                          sysvar_transaction_log_enabled,
                          PLUGIN_VAR_NOCMDARG,
                          N_("Enable transaction log"),
                          NULL, /* check func */
                          NULL, /* update func */
                          false /* default */);

static DRIZZLE_SYSVAR_BOOL(truncate_debug,
                          sysvar_transaction_log_truncate_debug,
                          PLUGIN_VAR_NOCMDARG,
                          N_("DEBUGGING - Truncate transaction log"),
                          NULL, /* check func */
                          set_truncate_debug, /* update func */
                          false /* default */);

static DRIZZLE_SYSVAR_STR(log_file,
                          sysvar_transaction_log_file,
                          PLUGIN_VAR_READONLY,
                          N_("Path to the file to use for transaction log."),
                          NULL, /* check func */
                          NULL, /* update func*/
                          DEFAULT_LOG_FILE_PATH /* default */);

static DRIZZLE_SYSVAR_BOOL(enable_checksum,
                          sysvar_transaction_log_checksum_enabled,
                          PLUGIN_VAR_NOCMDARG,
                          N_("Enable CRC32 Checksumming"),
                          NULL, /* check func */
                          NULL, /* update func */
                          false /* default */);

static struct st_mysql_sys_var* system_variables[]= {
  DRIZZLE_SYSVAR(enable),
  DRIZZLE_SYSVAR(truncate_debug),
  DRIZZLE_SYSVAR(log_file),
  DRIZZLE_SYSVAR(enable_checksum),
  NULL
};

drizzle_declare_plugin(transaction_log)
{
  "transaction_log",
  "0.1",
  "Jay Pipes",
  N_("Transaction Message Log"),
  PLUGIN_LICENSE_GPL,
  init, /* Plugin Init */
  deinit, /* Plugin Deinit */
  NULL, /* status variables */
  system_variables, /* system variables */
  NULL    /* config options */
}
drizzle_declare_plugin_end;
