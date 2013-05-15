/**
 * json2resultset
 * create ResultSet from json string.
 *
 * @param js json string
 * @param sil string item list
 */

#include <stdio.h>
#include <json/json.h>
#include "stringlist.h"
#include "resultset.h"

#include "json2resultset.h"


StringItemList* create_columnlist(json_object* root_obj)
{
    json_object *columns_obj;
    StringItemList* sil = NULL;

    columns_obj = json_object_object_get(root_obj, "columns");
    // printf("columns_obj=%s\n", json_object_to_json_string(columns_obj));
    if (json_object_get_type(columns_obj) == json_type_array) {
        // 配列
        json_object* obj;
        int i;
        int array_num = json_object_array_length(columns_obj);

        if ((sil = createStringItemList()) == NULL)
		return NULL;

        // array_list* list = json_object_get_array(columns_obj);

        for (i = 0; i < array_num; i++) {
		StringItem* si;
		obj = json_object_array_get_idx(columns_obj, i);
		if (json_object_get_type(obj) == json_type_string) {
			// printf("column(%d):%s\n", i, json_object_get_string(obj));
			if ((si = createStringItem(json_object_get_string(obj))) == NULL)
				return NULL;
			addStringItem(sil, si);
		}
        }
    }
    return sil;
}

ResultSet* json2resultset(char* js) {
	ResultSet* rs;
	json_object* json_obj;
    	json_object* data_obj;
	StringItemList* cl; // column name list

	/* create json object (JSON-C functoin) */
	json_obj = json_tokener_parse(js);
	if (json_obj == NULL) {
		return NULL;
	}
	if ((long) json_obj < 0) {
		return NULL;
	}

	/* create column name list */
	cl = create_columnlist(json_obj);

	/* create empty resultset */
	rs = createResultSet(cl);


	data_obj = json_object_object_get(json_obj, "data");
	// printf("data_obj=%s\n", json_object_to_json_string(data_obj));
	if (json_object_get_type(data_obj) == json_type_array) {
		// type is array
		int i, j; // i:record, j:column
		int column_num = get_column_num(json_obj);
		int array_num_i = json_object_array_length(data_obj);
		// array_list* record_list = json_object_get_array(data_obj);
		json_object* record;  // 1 record data

		for (i = 0; i < array_num_i; i++) {
			record = json_object_array_get_idx(data_obj, i);
			if (json_object_get_type(data_obj) == json_type_array) {
				StringItemList* sil;
				RowData* row_data;
				sil = createStringItemList();

				for (j = 0; j < column_num; j++) {
					json_object* obj; // 1 column data
					StringItem* si;
					
					obj = json_object_array_get_idx(record, j);
					if (obj == NULL) {
						// null
						si = createStringItem((char*) NULL);
						addStringItem(sil, si);
					} else if (json_object_get_type(obj) == json_type_string) {
						/// printf("data(%d):%s\n", j, json_object_get_string(obj));
						// create StringItem
						si = createStringItem((char*) json_object_get_string(obj));
						addStringItem(sil, si);
					} 
				}

				/* create RowData, and add ResultSet */
				row_data = createRowData(sil);
				addRowData(rs, row_data);
			}
			
		}
	}
	return rs;
}

int
get_column_num(json_object* root_obj) {
	json_object *columns_obj;

	columns_obj = json_object_object_get(root_obj, "columns");
	// printf("columns_obj=%s\n", json_object_to_json_string(columns_obj));
	if (json_object_get_type(columns_obj) == json_type_array) {
    		return json_object_array_length(columns_obj);
	}
	return -1; // not array
}

#ifdef JSON2RESULTSET
int
main(int argc, char** argv) {
	char* default_string = "{ \"columns\" : [ \"name\", \"area\",\"description\" ], \"data\" : [ [ \"nuko_yokohama\", \"Yokohama\",\"abc\" ], [ \"foo\", \"Nagoya\",\"efg\" ], [ \"bar\", \"Tokyo\",\"xyz\" ] ] }";	

	char* json_string;

	printf("argc=%d\n", argc);
	if (argc != 2) {
		json_string = default_string;
	} else {
		json_string = argv[1];
	}
	printf("main start, json_string=%s\n", json_string);

	ResultSet* rs;
	RowData* rd;
	int i;

	rs = json2resultset(json_string);
	if (rs == NULL) {
		fprintf(stderr, "json2resultset error, json_string=%s\n", json_string);
	}

	initResultSet(rs);
	while ((rd = getNextResultSet(rs)) != NULL) {
		for (i = 1; i <= rs->column_num; i++) {
			printf("value[%d]=%s\n", i, getValueRowData(rd, i));
		}
	}
	
	freeResultSet(rs);
}
#endif // JSON2RESULTSET
