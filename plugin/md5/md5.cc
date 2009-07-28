/* vim: expandtab:shiftwidth=2:tabstop=2:smarttab: 
   Copyright (C) 2006 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <drizzled/server_includes.h>
#include <drizzled/sql_udf.h>
#include <drizzled/item/func.h>
#include <drizzled/function/str/strfunc.h>

#if defined(HAVE_GNUTLS_OPENSSL)
# include <gnutls/openssl.h>
#else
# include <openssl/md5.h>
#endif /* HAVE_GNUTLS_OPENSSL */

#include <stdio.h>

using namespace std;

class Md5Function : public Item_str_func
{
public:
  Md5Function() : Item_str_func() {}
  String *val_str(String*);

  void fix_length_and_dec() 
  {
    max_length= 32;
    args[0]->collation.set(
      get_charset_by_csname(args[0]->collation.collation->csname,
                            MY_CS_BINSORT), DERIVATION_COERCIBLE);
  }

  const char *func_name() const 
  { 
    return "md5"; 
  }

  bool check_argument_count(int n) 
  { 
    return (n == 1); 
  }
};


String *Md5Function::val_str(String *str)
{
  assert(fixed == true);

  String *sptr= args[0]->val_str(str);
  if (sptr == NULL || str->alloc(32)) 
  {
    null_value= true;
    return 0;
  }

  null_value= false;

  unsigned char digest[16];
  str->set_charset(&my_charset_bin);
  MD5_CTX context;
  MD5_Init(&context);
  MD5_Update(&context, (unsigned char *) sptr->ptr(), sptr->length());
  MD5_Final(digest, &context);

  snprintf((char *) str->ptr(), 33,
    "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
    digest[0], digest[1], digest[2], digest[3],
    digest[4], digest[5], digest[6], digest[7],
    digest[8], digest[9], digest[10], digest[11],
    digest[12], digest[13], digest[14], digest[15]);
  str->length((uint32_t) 32);

  return str;
}


Create_function<Md5Function> md5udf(string("md5"));

static int initialize(PluginRegistry &registry)
{
  registry.add(&md5udf);
  return 0;
}

static int finalize(PluginRegistry &registry)
{
   registry.remove(&md5udf);
   return 0;
}

drizzle_declare_plugin(md5)
{
  "md5",
  "1.0",
  "Stewart Smith",
  "UDF for computing md5sum",
  PLUGIN_LICENSE_GPL,
  initialize, /* Plugin Init */
  finalize,   /* Plugin Deinit */
  NULL,   /* status variables */
  NULL,   /* system variables */
  NULL    /* config options */
}
drizzle_declare_plugin_end;