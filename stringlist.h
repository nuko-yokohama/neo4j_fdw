/**
 * stringlist.h
 *
 */

#ifndef _STRINGLIST_
#define _STRINGLIST_

/* stringItem */
typedef struct StringItem {
	char*	name;
	struct StringItem* next;
} StringItem;

/* StringItem List */
typedef struct StringItemList {
	int	item_num;
	struct StringItem* first;
	struct StringItem* last;
} StringItemList;

/* Functoin for StringItem, StringItemList */
extern StringItem* createStringItem(const char* name);
extern StringItemList* createStringItemList(void);
extern int addStringItem(StringItemList* sil, StringItem* si);
char* getValueStringItemList(StringItemList* sil, int index);
extern void freeStringItem(StringItem* si);
extern void freeStringItemList(StringItemList* sil);
extern void printStringItemList(StringItemList* sil);

#endif // _STRINGLIST_
