/**
 * json2resultset.h
 */

#ifndef _JSON2RESULTSET_
#define _JSON2RESULTSET_

#include "stringlist.h"
#include "resultset.h"
#include "json/json.h"

extern ResultSet* json2resultset(char* js) ;
extern int get_column_num(json_object* root_obj) ;
extern StringItemList* create_columnlist(json_object* root_obj);
extern StringItemList* json2columnlist(json_object* root_obj);

#endif // __JSON2RESULTSET_
