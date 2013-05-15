/**
 * selectlist.c
 *
 * create select list
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "neo4j_fdw.h"
#include "stringlist.h"

StringItem* createStringItem(const char* name) {
	StringItem* si = MALLOC(sizeof(StringItem));
	if (si == NULL)
		return NULL;

	if (name == NULL) {
		si->name = NULL;
	} else {
		si->name = MALLOC(strlen(name) + 1);
		strcpy(si->name, name);
	}

	si->next = NULL;

	return si;
}

StringItemList* createStringItemList() {
	StringItemList* sil = MALLOC(sizeof(StringItemList));
	if (sil == NULL)
		return NULL;

	sil->item_num = 0;
	sil->first = NULL;
	sil->last = NULL;
	return sil;
}

int addStringItem(StringItemList* sil, StringItem* si) {
	if (sil->first == NULL) {
		sil->first = si;
		sil->last = si;
	} else {
		sil->last->next = si;
		sil->last = si;
	}
	sil->item_num ++;
	return sil->item_num;
}

char* getValueStringItemList(StringItemList* sil, int index) {
	StringItem* si;
	int i=1;
	if (index <= 0 || index > sil->item_num) {
		/* invalid index */
		return NULL;
	}
	for (si = sil->first; si != NULL; si = si->next, i++) {
		if (i == index) {
			return si->name;
		}
	}
	return NULL;
}

void freeStringItem(StringItem* si) {
	StringItem* cur;
	StringItem* next;
	for (cur = si; cur != NULL; cur = next) {
		if (cur->name != NULL) 
			FREE(cur->name);
		next = cur->next;
		FREE(cur);
	}
}

void freeStringItemList(StringItemList* sil) {
	if (sil == NULL) return;

	freeStringItem(sil->first);
	FREE(sil);
}

/**
 * printStringItemList()
 * (debug)
 */
void printStringItemList(StringItemList* sil) {
	StringItem* si;
	if (sil == NULL) {
		printf("sil is null\n");
		return;
	}
	if (sil->first == NULL) {
		printf("sil is empty\n");
	}

	printf("sil->item_num=%d\n", sil->item_num);
	for (si = sil->first; si != NULL; si = si->next) {
		printf("si->name=%s\n", si->name);
	}
}

#ifdef UNIT_TEST_STRINGLIST
int main(int argc,char** argv) {

	StringItemList* sil;
	StringItem* si;
	int i;

	sil  = createStringItemList();

	si = createStringItem("name_a");
	addStringItem(sil, si);
	si = createStringItem("name_b");
	addStringItem(sil, si);
	si = createStringItem("name_c");
	addStringItem(sil, si);

	printStringItemList(sil);

	for (i = 1; i <= sil->item_num; i++) {
		printf("name(%d)=%s\n", i, getValueStringItemList(sil, i));
	}

	freeStringItemList(sil);
}

#endif
