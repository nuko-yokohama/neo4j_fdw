/**
 * resultset.c
 *
 * ResultSet module
 */
#include <stdlib.h>
#include <stdio.h>

#include <json/json.h>

#include "stringlist.h"
#include "resultset.h"

/* create empty ResultSet */
ResultSet* createResultSet(StringItemList* sil) {
	ResultSet* rs = MALLOC(sizeof(ResultSet));
	if (rs == NULL) 
		return NULL;

	rs->column_num = sil->item_num;
	rs->columns = sil;
	rs->rows = 0;
	rs->first = NULL;
	rs->current= NULL;
	rs->last = NULL;

	return rs;
}

RowData* createRowData(StringItemList* sil) {
	RowData* rd = MALLOC(sizeof(RowData));
	if (rd == NULL) 
		return NULL;

	rd->data = sil;
	rd->next = NULL;
	return rd;
};

int addRowData(ResultSet* rs, RowData* rd) {
	if (rs->first == NULL) {
		rs->first = rd;
		rs->last = rd;
	} else {
		rs->last->next = rd;
		rs->last = rd;
	}
	rs->rows ++;
	return rs->rows;
}

void initResultSet(ResultSet* rs) {
	rs->current = NULL;
}

RowData* getFirstResultSet(ResultSet* rs) {
	rs->current = rs->first;
	return rs->current;
}

RowData* getNextResultSet(ResultSet* rs) {
	if (rs->current == NULL) {
		rs->current = rs->first;
	} else {
		if (rs->current == NULL) {
			return NULL;
		} else {
			rs->current = rs->current->next;
		}
	}
	return rs->current;
}

char* getValueRowData(RowData* rd, int index) {
	return getValueStringItemList(rd->data, index);
}

void freeRowData(RowData* rd) {
	freeStringItemList(rd->data);
	FREE(rd);
}

void freeResultSet(ResultSet* rs) {
	RowData* cur;
	RowData* next;
	freeStringItemList(rs->columns);
	for (cur = rs->first; cur != NULL; cur = next) {
		next = cur->next;
		freeRowData(cur);
	}
	FREE(rs);
}

void printResultSet(ResultSet* rs) {
	RowData* rd;
	printf("rs->column_num=%d\n",rs->column_num);
	printStringItemList(rs->columns);

	printf("rs->rows_num=%d\n",rs->rows);
	for (rd = rs->first; rd != NULL; rd = rd->next) {
		printStringItemList(rd->data);
	}
}

#ifdef UNIT_TEST_RESULTSET
int main(int argc, char** argv) {

        StringItemList* sil;
        StringItem* si;
	ResultSet* rs;
	int rows, cols;
	char tmp_data[128];



	sil  = createStringItemList();
	for (cols = 1; cols <= 3; cols++) {
		sprintf(tmp_data, "col_%02d", cols);
		si = createStringItem(tmp_data);
		addStringItem(sil, si);
	}
	rs = createResultSet(sil);

	for (rows = 1; rows <= 10; rows++) {
		StringItemList* sil;
		StringItem* si;
		RowData* rd;
		sil = createStringItemList();

		for (cols = 1; cols <= rs->column_num; cols++) {
			sprintf(tmp_data, "data_%02d_%02d", rows, cols);
			si = createStringItem(tmp_data);
			addStringItem(sil, si);
		}

		rd = createRowData(sil);
		addRowData(rs, rd);
	}

	printResultSet(rs);
	freeResultSet(rs);

}
#endif 
