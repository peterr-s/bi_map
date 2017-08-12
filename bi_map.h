#ifndef BI_MAP_H
#define BI_MAP_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define BI_ERR_ALLOC 1
#define BI_W_NOTFOUND 2

#define DEFAULT_LOAD_FACTOR 0.75
#define DEFAULT_LEN 10

#define BI_NORMAL 0
#define BI_FAST 1
#define BI_DESTROY 2
#define BI_E2 4

/* stdbool.h is not required by C89 */
typedef unsigned char bool;
#define false 0
#define true !false

typedef struct node node;
struct node
{
	void* element_1;
	void* element_2;
	node* next;
};

typedef struct bi_map bi_map;
struct bi_map
{
	node** table;
	unsigned long int(* hash_fn_1)(void* key);
	unsigned long int(* hash_fn_2)(void* key);
	bool(* eq_fn_1)(void* p1, void* p2);
	bool(* eq_fn_2)(void* p1, void* p2);
	unsigned long int table_len;
	float load_factor;
	unsigned long int element_ct;
};

short int bi_map_init(bi_map* map, unsigned long int(* hash_fn_1)(void* key), unsigned long int(* hash_fn_2)(void* key), bool(* eq_fn_1)(void* p1, void* p2), bool(* eq_fn_2)(void* p1, void* p2), unsigned long int start_len, float load_factor);
void bi_map_destroy(bi_map* map, unsigned short int flags);

short int bi_map_put(bi_map* map, void* element_1, void* element_2, unsigned short int flags);
void* bi_map_get(bi_map* map, void* element, unsigned short int flags);
short int bi_map_drop(bi_map* map, void* element, unsigned short int flags);

void place_bidirectional_map_node_(node** table, node* n, unsigned long int idx_1, unsigned long int idx_2); /* internal use only */

unsigned long int default_hash(void* key);
unsigned long int string_hash(void* key);

bool default_eq(void* p1, void* p2);
bool string_eq(void* p1, void* p2);

#ifdef __cplusplus
}
#endif

#endif