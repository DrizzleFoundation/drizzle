/*
 * Copyright (c) 2010, Joseph Daly <skinny.moey@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *   * Neither the name of Joseph Daly nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>          
#include "stats_schema.h"
#include "scoreboard.h"
#include "identifiers.h"

using namespace drizzled;
using namespace plugin;
using namespace std;

static Identifiers identifiers; 

SessionStatementsTool::SessionStatementsTool(LoggingStats *in_logging_stats) :
  plugin::TableFunction("DATA_DICTIONARY", "SESSION_STATEMENTS_NEW")
{
  logging_stats= in_logging_stats;
  add_field("VARIABLE_NAME");
  add_field("VARIABLE_VALUE", TableFunction::NUMBER);
}

SessionStatementsTool::Generator::Generator(Field **arg, LoggingStats *logging_stats) :
  plugin::TableFunction::Generator(arg)
{
  count= 0;

  /* Set user_commands */
  Scoreboard *current_scoreboard= logging_stats->getCurrentScoreboard();

  Session *this_session= current_session;

  uint32_t bucket_number= current_scoreboard->getBucketNumber(this_session);

  vector<ScoreboardSlot* > *scoreboard_vector=
     current_scoreboard->getVectorOfScoreboardVectors()->at(bucket_number);

  vector<ScoreboardSlot *>::iterator scoreboard_vector_it= scoreboard_vector->begin();
  vector<ScoreboardSlot *>::iterator scoreboard_vector_end= scoreboard_vector->end();

  ScoreboardSlot *scoreboard_slot= NULL;
  for (vector<ScoreboardSlot *>::iterator it= scoreboard_vector->begin();
       it != scoreboard_vector->end(); ++it)
  {
    scoreboard_slot= *it;
    if (scoreboard_slot->getSessionId() == this_session->getSessionId())
    {
      break;
    }
  }

  user_commands= NULL;

  if (scoreboard_slot != NULL)
  {
    user_commands= scoreboard_slot->getUserCommands();
  }
}

bool SessionStatementsTool::Generator::populate()
{
  if (user_commands == NULL)
  {
    return false;
  } 

  uint32_t number_identifiers= user_commands->getCommandCount();

  if (count == number_identifiers)
  {
    return false;
  }

  push(UserCommands::IDENTIFIERS[count]);
  push(user_commands->getCount(count));

  ++count;
  return true;
}

GlobalStatementsTool::GlobalStatementsTool(LoggingStats *in_logging_stats) :
  plugin::TableFunction("DATA_DICTIONARY", "GLOBAL_STATEMENTS_NEW")
{   
  logging_stats= in_logging_stats;
  add_field("VARIABLE_NAME");
  add_field("VARIABLE_VALUE", TableFunction::NUMBER);
}

GlobalStatementsTool::Generator::Generator(Field **arg, LoggingStats *logging_stats) :
  plugin::TableFunction::Generator(arg)
{
  count= 0;
  global_stats= logging_stats->getCumulativeStats()->getGlobalStats();
}

bool GlobalStatementsTool::Generator::populate()
{
  uint32_t number_identifiers= global_stats->getUserCommands()->getCommandCount(); 
  if (count == number_identifiers)
  {
    return false;
  }

  push(UserCommands::IDENTIFIERS[count]);
  push(global_stats->getUserCommands()->getCount(count));

  ++count;
  return true;
}

CurrentCommandsTool::CurrentCommandsTool(LoggingStats *logging_stats) :
  plugin::TableFunction("DATA_DICTIONARY", "CURRENT_SQL_COMMANDS")
{
  outer_logging_stats= logging_stats;

  add_field("USER");
  add_field("IP");

  vector<const char* >::iterator command_identifiers_it= 
    identifiers.getCommandIdentifiers().begin();

  vector<const char* >::iterator command_identifiers_end=              
    identifiers.getCommandIdentifiers().end();

  for (; command_identifiers_it != command_identifiers_end; ++command_identifiers_it) 
  {
    add_field(*command_identifiers_it, TableFunction::NUMBER);
  }
}

CurrentCommandsTool::Generator::Generator(Field **arg, LoggingStats *logging_stats) :
  plugin::TableFunction::Generator(arg)
{
  inner_logging_stats= logging_stats;

  isEnabled= inner_logging_stats->isEnabled();

  if (isEnabled == false)
  {
    return;
  }

  current_scoreboard= logging_stats->getCurrentScoreboard();
  current_bucket= 0;

  vector_of_scoreboard_vectors_it= current_scoreboard->getVectorOfScoreboardVectors()->begin();
  vector_of_scoreboard_vectors_end= current_scoreboard->getVectorOfScoreboardVectors()->end();

  setVectorIteratorsAndLock(current_bucket);
}

void CurrentCommandsTool::Generator::setVectorIteratorsAndLock(uint32_t bucket_number)
{
  vector<ScoreboardSlot* > *scoreboard_vector= 
    current_scoreboard->getVectorOfScoreboardVectors()->at(bucket_number); 

  current_lock= current_scoreboard->getVectorOfScoreboardLocks()->at(bucket_number);

  scoreboard_vector_it= scoreboard_vector->begin();
  scoreboard_vector_end= scoreboard_vector->end();
  pthread_rwlock_rdlock(current_lock);
}

bool CurrentCommandsTool::Generator::populate()
{
  if (isEnabled == false)
  {
    return false;
  }

  while (vector_of_scoreboard_vectors_it != vector_of_scoreboard_vectors_end)
  {
    while (scoreboard_vector_it != scoreboard_vector_end)
    {
      ScoreboardSlot *scoreboard_slot= *scoreboard_vector_it; 
      if (scoreboard_slot->isInUse())
      {
        UserCommands *user_commands= scoreboard_slot->getUserCommands();
        push(scoreboard_slot->getUser());
        push(scoreboard_slot->getIp());

        int number_commands= user_commands->getCommandCount(); 

        for (int j= 0; j < number_commands; ++j)
        {
          push(user_commands->getCount(j));
        }

        ++scoreboard_vector_it;
        return true;
      }
      ++scoreboard_vector_it;
    }
    
    ++vector_of_scoreboard_vectors_it;
    pthread_rwlock_unlock(current_lock); 
    ++current_bucket;
    if (vector_of_scoreboard_vectors_it != vector_of_scoreboard_vectors_end)
    {
      setVectorIteratorsAndLock(current_bucket); 
    } 
  }

  return false;
}

CumulativeCommandsTool::CumulativeCommandsTool(LoggingStats *logging_stats) :
  plugin::TableFunction("DATA_DICTIONARY", "CUMULATIVE_SQL_COMMANDS")
{
  outer_logging_stats= logging_stats;

  add_field("USER");

  vector<const char* >::iterator command_identifiers_it=
    identifiers.getCommandIdentifiers().begin();

  vector<const char* >::iterator command_identifiers_end=
    identifiers.getCommandIdentifiers().end();


  for (; command_identifiers_it != command_identifiers_end; ++command_identifiers_it)
  {
    add_field(*command_identifiers_it, TableFunction::NUMBER);
  }
}

CumulativeCommandsTool::Generator::Generator(Field **arg, LoggingStats *logging_stats) :
  plugin::TableFunction::Generator(arg)
{
  inner_logging_stats= logging_stats;
  record_number= 0;

  if (inner_logging_stats->isEnabled())
  {
    last_valid_index= inner_logging_stats->getCumulativeStats()->getCumulativeStatsLastValidIndex();
  }
  else
  {
    last_valid_index= INVALID_INDEX; 
  }
}

bool CumulativeCommandsTool::Generator::populate()
{
  if ((record_number > last_valid_index) || (last_valid_index == INVALID_INDEX))
  {
    return false;
  }

  while (record_number <= last_valid_index)
  {
    ScoreboardSlot *cumulative_scoreboard_slot= 
      inner_logging_stats->getCumulativeStats()->getCumulativeStatsByUserVector()->at(record_number);

    if (cumulative_scoreboard_slot->isInUse())
    {
      UserCommands *user_commands= cumulative_scoreboard_slot->getUserCommands(); 
      push(cumulative_scoreboard_slot->getUser());

      int number_commands= user_commands->getCommandCount();

      for (int j= 0; j < number_commands; ++j)
      {
        push(user_commands->getCount(j));
      }
      ++record_number;
      return true;
    } 
    else 
    {
      ++record_number;
    }
  }

  return false;
}
