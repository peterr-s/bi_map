#include "bi_map.h"

/* initialize a bidirectional map structure */
short int bi_map_init(bi_map* map, unsigned long int(* hash_fn_1)(void* key), unsigned long int(* hash_fn_2)(void* key), bool(* eq_fn_1)(void* p1, void* p2), bool(* eq_fn_2)(void* p1, void* p2), unsigned long int start_len, float load_factor)
{
	map->table = calloc(start_len, sizeof(node*));
	if(!map->table)
		return BI_ERR_ALLOC;
	map->hash_fn_1 = hash_fn_1;
	map->hash_fn_2 = hash_fn_2;
	map->eq_fn_1 = eq_fn_1;
	map->eq_fn_2 = eq_fn_2;
	map->table_len = start_len;
	map->load_factor = load_factor;
	map->element_ct = 0;
	
	return 0;
}

/* add a pair to a bidirectional map, resizing and rehashing if necessary */
short int bi_map_put(bi_map* map, void* element_1, void* element_2, unsigned short int flags)
{
	node* n_node,
		* s_node;
	unsigned long int node_idx_1,
		node_idx_2,
		node_hash;
	
	/* perform resize and rehash if necessary */
	if(((float)(map->element_ct += 2)) / map->table_len > map->load_factor)
	{
		unsigned long int i,
			n_idx_1,
			n_idx_2;
		
		size_t n_len = map->table_len << 1;
		node** temp = calloc(n_len, sizeof(node*));
		if(!temp)
		{
			map->element_ct -= 2;
			return BI_ERR_ALLOC;
		}

		/* for each element in the table */
		for(i = 0; i < map->table_len; i ++)
		{
			/* traverse down the linked list */
			node* current,
				* next,
				* last;
			
			last = NULL; /* this is so that the chains already processed can be unlinked */
			
			/* guard against empty elements */
			last = current = map->table[i];
			while(current)
			{
				/* prepare lookahead pointer */
				next = current->next;
				
				/* rehash and copy each item */
				n_idx_1 = (map->hash_fn_1)(current->element_1) % n_len;
				n_idx_2 = (map->hash_fn_2)(current->element_2) % n_len;
				place_bidirectional_map_node_(temp, current, n_idx_1, n_idx_2);
				
				/* unlink chain */
				last->next = NULL;
				
				/* advance to next list element */
				last = current;
				current = next;
			}
		}
		
		free(map->table);
		map->table = temp;
		map->table_len = n_len;
	}
	
	/* check whether either item is already in map */
	node_hash = (map->hash_fn_1)(element_1);
	node_idx_1 = node_hash % map->table_len;
	s_node = map->table[node_idx_1];
	while(s_node)
	{
		if(flags & BI_FAST ? (hash_fn(s_node->element_1) == node_hash) : (eq_fn(s_node->element_1, element_1)))
		{
			map->element_ct -= 2;

			/* deallocate existing value if necessary*/
			if(flags & BI_DESTROY)
				free(s_node->element_2);
			
			/* update value and return if found */
			s_node->element_2 = element_2;
			return 0;
		}
		s_node = s_node->next;
	}
	
	node_hash = (map->hash_fn_2)(element_2);
	node_idx_2 = node_hash % map->table_len;
	s_node = map->table[node_idx_2];
	while(s_node)
	{
		if(flags & BI_FAST ? (hash_fn(s_node->element_2) == node_hash) : (eq_fn(s_node->element_2, element_2)))
		{
			map->element_ct -= 2;

			/* deallocate existing value if necessary*/
			if(flags & BI_DESTROY)
				free(s_node->element_1);
			
			/* update value and return if found */
			s_node->element_1 = element_1;
			return 0;
		}
		s_node = s_node->next;
	}
	
	/* create a new node to hold data */
	n_node = malloc(sizeof(node));
	if(!n_node)
		return BI_ERR_ALLOC;
	n_node->element_1 = element_1;
	n_node->element_2 = element_2;
	
	/* add new node to table */
	place_bidirectional_map_node_(map->table, n_node, node_idx_1, node_idx_2);
	
	return 0;
}

/* get the value associated with a key
 * returns NULL if key does not exist in map
 */
void* bi_map_get(bi_map* map, void* element, unsigned short int flags)
{
	node* current;
	unsigned long int hash;
	bool get_by_second;
	
	get_by_second = flags & BI_E2;
	
	hash = get_by_second ? (map->hash_fn_1)(element) : (map->hash_fn_2)(element);
	current = map->table[hash % map->table_len];
	while(current)
	{
		if(get_by_second) if(flags & BI_FAST ? (hash == (map->hash_fn_2)(current->element_2)) : ((map->eq_fn_2(element, current->element_2))))
			return current->element_1;
		else if(flags & BI_FAST ? (hash == (map->hash_fn_1)(current->element_1)) : ((map->eq_fn_1(element, current->element_1))))
			return current->element_2;
		
		current = current->next;
	}
	
	return NULL;
}

/* removes an item from the bidirectional map if present (else no-op) */
short int bi_map_drop(bi_map* map, void* element, unsigned short int flags)
{
	node* current,
		* parent = NULL;
	unsigned long int hash,
		idx,
		n_idx_1,
		n_idx_2;
	bool get_by_second;
	
	get_by_second = flags & BI_E2;
	
	hash = get_by_second ? (map->hash_fn_1)(element) : (map->hash_fn_2)(element);
	idx = hash % map->table_len;
	current = map->table[idx];
	while(current)
	{
		if(get_by_second ? (flags & BI_FAST ? (hash == (map->hash_fn_2)(current->element_2)) : ((map->eq_fn_2(element, current->element_2)))) : (flags & BI_FAST ? (hash == (map->hash_fn_1)(current->element_1)) : ((map->eq_fn_1(element, current->element_1)))))
		{
			/* redirect the node chain around it, then destroy */
			if(parent)
				parent->next = current->next;
			else
				map->table[idx] = current->next;
			if(flags & BI_DESTROY)
				free(get_by_second ? current->element_1 : current->element_2); /* key can be freed from calling code; would be too messy to add that here */
			free(current); /* nodes are always malloc'ed */
			
			/* perform resize and rehash if necessary */
			if((map->table_len > 10) && (map->element_ct -= 2) / (map->table_len << 1) < map->load_factor)
			{
				unsigned long int i;
				
				size_t n_len = map->table_len >> 1;
				node** temp = calloc(n_len, sizeof(node*));
				if(!temp)
				{
					map->element_ct += 2;
					return BI_ERR_ALLOC;
				}
				
				/* for each element in the table */
				for(i = 0; i < map->table_len; i ++)
				{
					/* traverse down the linked list */
					node* current,
						* next,
						* last;
					
					last = NULL; /* this is so that the chains already processed can be unlinked */
					
					/* guard against empty elements */
					last = current = map->table[i];
					while(current)
					{
						/* prepare lookahead pointer */
						next = current->next;
						
						/* rehash and copy each item */
						n_idx_1 = (map->hash_fn_1)(current->element_1) % n_len;
						n_idx_2 = (map->hash_fn_2)(current->element_2) % n_len;
						place_bidirectional_map_node_(temp, current, n_idx_1, n_idx_2);
						
						/* unlink chain */
						last->next = NULL;
						
						/* advance to next list element */
						last = current;
						current = next;
					}
				}
				
				free(map->table);
				map->table = temp;
				map->table_len = n_len;
			}
			
			/* stop looking since node was found */
			return 0;
		}
		
		parent = current;
		current = current->next;
	}
	
	return BI_W_NOTFOUND;
}

/* destroys a bidirectional map (does not touch pointed data) */
void bi_map_destroy(bi_map* map, unsigned short int flags)
{
	unsigned long int i;
	node* current,
		* next;
	
	/* goes down each list, deleting all nodes */
	for(i = 0; i < map->table_len; i++)
	{
		current = map->table[i];
		while(current)
		{
			next = current->next;
			if(flags & BI_DESTROY)
			{
				free(current->element_1);
				free(current->element_2);
			}
			free(current);
			current = next;
		}
	}
	
	/* deletes the table */
	free(map->table);
}

void place_bidirectional_map_node_(node** table, node* n, unsigned long int idx_1, unsigned long int idx_2)
{
	node** tail,
		* parent;
	
	/* traverse down list for first hash if necessary, placing item at end */
	tail = &(table[idx_1]);
	while((*tail)->next)
		tail = &((*tail)->next);
	(*tail)->next = n;
	
	parent = *tail;
	
	/* same process for second hash, with exit condition to avoid duplicate entries */
	tail = &(table[idx_2]);
	while((*tail)->next) if(*tail == parent) /* stop if this process leads back into the same chain */
		return;
	else
		tail = &((*tail)->next);
	(*tail)->next = n;
}

/* simple "hash" function (uses pointer as hash code) */
unsigned long int default_hash(void* key)
{
	return (unsigned long int) key;
}

/* simple string hash function */
unsigned long int string_hash(void* key)
{
	unsigned long int hash = 0;
	char* string = key;
	
	while(*string)
	{
		hash += *string; /* addition followed by multiplication makes collisions of scrambled strings with the same characters less likely */
		hash *= 7;
		string ++;
	}
	
	return hash;
}

/* simple equality checker (compares pointers) */
bool default_eq(void* p1, void* p2)
{
	return p1 == p2;
}

/* string equality checker (uses strcmp) */
bool string_eq(void* p1, void* p2)
{
	return !strcmp((char*)p1, (char*)p2);
}
