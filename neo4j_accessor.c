#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "neo4j_fdw.h"
#include "neo4j_accessor.h"

char wr_buf[MAX_BUF+1];
int  wr_index;

static size_t write_data( void *buffer, size_t size, size_t nmemb, void *userp );

/*
 *  * Write data callback function (called within the context of 
 *   * curl_easy_perform.
 *    */
size_t write_data( void *buffer, size_t size, size_t nmemb, void *userp )
{
  int segsize = size * nmemb;

  /* Check to see if this data exceeds the size of our buffer. If so, 
 *    * set the user-defined context value and return 0 to indicate a
 *       * problem to curl.
 *          */
  if ( wr_index + segsize > MAX_BUF ) {
    *(int *)userp = 1;
    return 0;
  }

  /* Copy the data from the curl buffer into our buffer */
  memcpy( (void *)&wr_buf[wr_index], buffer, (size_t)segsize );

  /* Update the write index */
  wr_index += segsize;

  /* Null terminate the buffer */
  wr_buf[wr_index] = 0;

  /* Return the number of bytes received, indicating to curl that all is okay */
  return segsize;
}


/*
 *  * Simple curl application to read the index.html file from a Web site.
 *   */
char* neo4j_accessor( char* url, char* query )
{
  CURL *curl;
  CURLcode ret;
  int  wr_error;

  char* buf = NULL;
  char* jsonObj;
  struct curl_slist *headers;

  wr_error = 0;
  wr_index = 0;

  /* First step, init curl */
  curl = curl_easy_init();
  if (!curl) {
    // printf("couldn't init curl\n");
    return 0;
  }

  /* Tell curl the URL of the file we're going to retrieve */
  jsonObj = query;
  headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json;");
  headers = curl_slist_append(headers, "Accept: application/json;");
  headers = curl_slist_append(headers, "charsets: utf-8;");
  headers = curl_slist_append(headers, "User-Agent: curl/7.29.0;");
  // print_curl_slist(headers);
  curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );
  curl_easy_setopt( curl, CURLOPT_URL, url );
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(jsonObj));
  curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&wr_error );
  curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_data );

  /* Allow curl to perform the action */
  ret = curl_easy_perform( curl );

  // printf( "ret = %d (write_error = %d)\n", ret, wr_error );

  /* Emit the page if curl indicates that no errors occurred */
  if ( ret == 0 ) {
	buf = MALLOC(strlen(wr_buf) + 1);
	strcpy(buf, wr_buf);
  }

  curl_easy_cleanup( curl );
  return buf;
}

#ifdef NEO4J_ACCESSOR
static void print_curl_slist(struct curl_slist* s) ;

void print_curl_slist(struct curl_slist* s) {
  struct curl_slist* c;

  printf("s=%0x\n", s);
  printf("curl_slist:");
  for (c = s; c != NULL; c = c -> next) {
    printf("[%s]", c->data);
  }
}

int
main(int argc, char** argv) {
	char* default_url = "http://localhost:7474/db/data/cypher";
	char* default_query = "{\"query\":\"START n=node(*) RETURN n.name as name, n.gender? as gender \" }";
	char* url;
	char* query;
	char* result;

	if (argc == 3) {
		url = argv[1];
		query = argv[2];
	} else {
		url = default_url;
		query = default_query;
	}

	result = neo4j_accessor(url, query);
	if (result != NULL) {
		printf("neo4j_rest_accessor success.\nurl=%s\nquery=%s\n", url, query);
		printf("result = %s\n", result);
	} else {
		fprintf(stderr, "neo4j_rest_accessor error.\nurl=%s\nquery=%s\n", url, query);
	}
}
#endif // NEO4J_ACCESOR
